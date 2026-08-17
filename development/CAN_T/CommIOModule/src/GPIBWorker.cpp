#include "CommIOModule/GPIBWorker.h"
#include "CommIOModule/PerfClock.h"

#include <QCoreApplication>
#include <visa.h>

namespace CommIO {

namespace {

CommIO::PerfEvent makePerfEvent(CommIO::PerfStage stage, const QByteArray& data)
{
    CommIO::PerfEvent event;
    event.channel = CommIO::ChannelType::GPIB;
    event.stage   = stage;
    event.epochUs = CommIO::PerfClock::epochUs();
    event.data    = data;
    return event;
}

QString visaStatusHex(ViStatus status)
{
    return QStringLiteral("0x%1")
        .arg(static_cast<quint32>(status), 8, 16, QChar('0'))
        .toUpper();
}

QString viOpenFailureReason(ViStatus status)
{
// #ifdef VI_ERROR_INTF_NUM_NCONFIG
//     if (status == VI_ERROR_INTF_NUM_NCONFIG) {
//         return QStringLiteral("接口号未配置。请确认 GPIB 板卡号有效，并在 NI-MAX 中完成该接口配置。");
//     }
// #endif
    if (status == VI_ERROR_INTF_NUM_NCONFIG) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "资源名称无效。请检查板卡号配置是否正确，或设备连线是否稳定。");
    }
//#ifdef VI_ERROR_RSRC_NFOUND
    if (status == VI_ERROR_RSRC_NFOUND) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "未找到目标资源。请确认 GPIB 板卡号、仪器主地址、设备上电状态，以及 NI-MAX 中资源可见。");
    }
// #endif
// #ifdef VI_ERROR_RSRC_BUSY
    if (status == VI_ERROR_RSRC_BUSY) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "目标资源正忙。设备可能正在被 NI-MAX 或其他程序占用，请先释放后重试。");
    }
// #endif
// #ifdef VI_ERROR_RSRC_LOCKED
    if (status == VI_ERROR_RSRC_LOCKED) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "目标资源被锁定。请关闭占用该资源的进程后重试。");
    }
// #endif
// #ifdef VI_ERROR_INV_RSRC_NAME
    if (status == VI_ERROR_INV_RSRC_NAME) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "资源名称无效。请检查板卡号配置是否正确，或设备连线是否稳定。");
    }
//#endif
//#ifdef VI_ERROR_INV_ACC_MODE
    if (status == VI_ERROR_INV_ACC_MODE) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "访问模式无效。请检查 VISA 打开参数与驱动环境。");
    }
//#endif
//#ifdef VI_ERROR_ALLOC
    if (status == VI_ERROR_ALLOC) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "系统资源分配失败。请关闭部分程序后重试。");
    }
//#endif
//#ifdef VI_ERROR_TMO
    if (status == VI_ERROR_TMO) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "打开资源超时。请检查设备连线、地址与仪器响应状态。");
    }
//#endif
//#ifdef VI_ERROR_LIBRARY_NFOUND
    if (status == VI_ERROR_LIBRARY_NFOUND) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "未找到 VISA 运行库。请确认 NI-VISA 已正确安装。");
    }
//#endif
//#ifdef VI_ERROR_SYSTEM_ERROR
    if (status == VI_ERROR_SYSTEM_ERROR) {
        return QCoreApplication::translate("CommIO::GPIBWorker", "系统层错误。建议重启 NI 相关服务或重启系统后重试。");
    }
//#endif

    return QCoreApplication::translate("CommIO::GPIBWorker", "VISA 打开失败（未匹配到明确原因）。建议在 NI-MAX 中执行通信测试定位问题或重启本软件。");
}

} // namespace

GPIBWorker::GPIBWorker(QObject* parent)
    : QObject(parent)
{
}

GPIBWorker::~GPIBWorker()
{
    if (m_isOpen) {
        doClose();
    }
}

void GPIBWorker::doOpen(const CommIO::GPIBConfig& config)
{
    if (m_isOpen) {
        emit errorOccurred(ErrorType::AlreadyOpen,
                           tr("GPIB 会话已存在"));
        emit opened(false);
        return;
    }

    m_timeout = config.timeout;

    // 打开资源管理器
    ViStatus status = viOpenDefaultRM(&m_resourceManager);
    if (!checkVISAStatus(status, QStringLiteral("viOpenDefaultRM"))) {
        emit opened(false);
        return;
    }

    // 构建 VISA 资源名称
    QString resourceName = QStringLiteral("GPIB%1::%2::INSTR")
                               .arg(config.boardIndex)
                               .arg(config.primaryAddress);

    // 打开仪器会话
    status = viOpen(m_resourceManager,
                    resourceName.toLatin1().constData(),
                    VI_NULL, VI_NULL,
                    &m_instrument);
    if (!checkVISAStatus(status, QStringLiteral("viOpen(%1)").arg(resourceName))) {
        viClose(m_resourceManager);
        m_resourceManager = 0;
        emit opened(false);
        return;
    }
    int res = 0;
    // 设置超时
    res =viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE,
                   static_cast<ViAttrState>(config.timeout));

    // 配置 EOS 模式
    res =viSetAttribute(m_instrument, VI_ATTR_TERMCHAR_EN,
                   config.termCharEnabled ? VI_TRUE : VI_FALSE);
    res =viSetAttribute(m_instrument, VI_ATTR_TERMCHAR,
                   static_cast<ViAttrState>(config.termChar));
    res =viSetAttribute(m_instrument, VI_ATTR_SEND_END_EN,
                   config.sendEndEnabled ? VI_TRUE : VI_FALSE);

    m_isOpen = true;
    emit opened(true);
}

void GPIBWorker::doSend(const QByteArray& data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("GPIB 会话未打开"));
        return;
    }

    ViUInt32 retCount = 0;
    emit perfEvent(makePerfEvent(CommIO::PerfStage::TxIoBegin, data));
    ViStatus status = viWrite(m_instrument,
                              reinterpret_cast<ViBuf>(const_cast<char*>(data.constData())),
                              static_cast<ViUInt32>(data.size()),
                              &retCount);
    if (!checkVISAStatus(status, QStringLiteral("viWrite"))) {
        return;
    }
    // 只写，不读取响应
}

void GPIBWorker::doQuery(const QByteArray& data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("GPIB 会话未打开"));
        return;
    }

    ViUInt32 retCount = 0;
    emit perfEvent(makePerfEvent(CommIO::PerfStage::TxIoBegin, data));
    ViStatus status = viWrite(m_instrument,
                              reinterpret_cast<ViBuf>(const_cast<char*>(data.constData())),
                              static_cast<ViUInt32>(data.size()),
                              &retCount);
    if (!checkVISAStatus(status, QStringLiteral("viWrite"))) {
        return;
    }
    // 写入成功后读取响应
    readResponse();
}

void GPIBWorker::doClose()
{
    if (!m_isOpen) {
        return;
    }

    if (m_instrument) {
        viClose(m_instrument);
        m_instrument = 0;
    }
    if (m_resourceManager) {
        viClose(m_resourceManager);
        m_resourceManager = 0;
    }

    m_isOpen = false;
    emit closed();
}

void GPIBWorker::readResponse()
{
    const int bufferSize = 4096;
    QByteArray buffer(bufferSize, '\0');
    ViUInt32 retCount = 0;

    ViStatus status = viRead(m_instrument,
                             reinterpret_cast<ViBuf>(buffer.data()),
                             static_cast<ViUInt32>(bufferSize),
                             &retCount);

    // VI_SUCCESS_TERM_CHAR 和 VI_SUCCESS_MAX_CNT 也算成功
    if (status >= 0) {
        buffer.resize(static_cast<int>(retCount));
        emit perfEvent(makePerfEvent(CommIO::PerfStage::RxIoFirst, buffer));
        emit dataReady(buffer);
    } else {
        checkVISAStatus(status, QStringLiteral("viRead"));
    }
}

bool GPIBWorker::checkVISAStatus(ViStatus status, const QString& operation)
{
    if (status >= 0) {
        return true; // VISA 成功状态码为非负值
    }

    ErrorType errType = ErrorHelper::fromVISAStatus(status);

    // 打开资源失败时，优先给出“面向用户”的原因提示，不直接暴露 viStatusDesc 原文。
    if (operation.startsWith(QStringLiteral("viOpen"))) {
        QString message = tr("%1失败: %2 错误码(%3)")
                              .arg(operation)
                              .arg(viOpenFailureReason(status))
                              .arg(visaStatusHex(status));
        emit errorOccurred(errType, message);

        if (errType == ErrorType::DeviceDisconnected) {
            m_isOpen = false;
        }
        return false;
    }

    QString reason = ErrorHelper::errorTypeToString(errType);

    QString message = tr("%1失败：%2 (Code: %3)")
                          .arg(operation)
                          .arg(reason)
                          .arg(visaStatusHex(status));

    emit errorOccurred(errType, message);

    // 连接丢失时自动清理
    if (errType == ErrorType::DeviceDisconnected) {
        m_isOpen = false;
    }

    return false;
}

} // namespace CommIO
