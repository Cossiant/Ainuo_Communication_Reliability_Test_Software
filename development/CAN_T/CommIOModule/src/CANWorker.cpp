#include "CommIOModule/CANWorker.h"
#include "CommIOModule/PerfClock.h"
#include "CommIOModule/ZLGCanLoader.h"

#include <QByteArray>
#include <cstring>

namespace CommIO {

namespace {

PerfEvent makePerfEvent(PerfStage stage, const QByteArray &data)
{
    PerfEvent event;
    event.channel = ChannelType::CAN;
    event.stage = stage;
    event.epochUs = PerfClock::epochUs();
    event.data = data;
    return event;
}

QString readChannelError(quintptr channelHandle, const QString &context)
{
    ZCAN_CHANNEL_ERR_INFO errInfo;
    std::memset(&errInfo, 0, sizeof(errInfo));
    if (ZLGCanDriver::readChannelErrInfo(
            reinterpret_cast<CHANNEL_HANDLE>(channelHandle), &errInfo) != 0) {
        return QStringLiteral("%1 (Code: 0x%2)")
                .arg(context)
                .arg(errInfo.error_code, 8, 16, QChar('0'));
    }
    return context;
}

// 应用统一帧格式：1 字节标志 + 4 字节大端 ID + 数据
struct DecodedFrame {
    quint32 canIdRaw = 0;
    QByteArray data;
    bool isFd = false;
    bool brs = false;
    bool valid = false;
};

DecodedFrame decodeFrame(const QByteArray &data)
{
    DecodedFrame frame;
    if (data.size() < 5) {
        return frame;
    }

    const unsigned char flags = static_cast<unsigned char>(data.at(0));
    frame.canIdRaw = (static_cast<unsigned char>(data.at(1)) << 24)
                   | (static_cast<unsigned char>(data.at(2)) << 16)
                   | (static_cast<unsigned char>(data.at(3)) << 8)
                   |  static_cast<unsigned char>(data.at(4));
    frame.data = data.mid(5);
    frame.isFd = (flags & 0x01) != 0;
    frame.brs = (flags & 0x02) != 0;
    frame.valid = true;
    return frame;
}

} // namespace

CANWorker::CANWorker(QObject *parent)
    : QObject(parent)
{
}

CANWorker::~CANWorker()
{
    if (m_isOpen) {
        doClose();
    }
}

void CANWorker::doOpen(const CommIO::CANConfig &config)
{
    if (m_isOpen) {
        emit errorOccurred(ErrorType::AlreadyOpen, tr("CAN 设备已打开"));
        emit opened(false, 0, 0, 0);
        return;
    }
    if (!ZLGCanDriver::isLoaded()) {
        emit errorOccurred(ErrorType::DriverNotInstalled,
                           tr("未找到可用的 64 位 zlgcan.dll"));
        emit opened(false, 0, 0, 0);
        return;
    }

    const DEVICE_HANDLE device = ZLGCanDriver::openDevice(
        static_cast<UINT>(config.deviceType),
        static_cast<UINT>(config.deviceIndex), 0);
    if (device == INVALID_DEVICE_HANDLE) {
        emit errorOccurred(ErrorType::ConnectionFailed, tr("打开 CAN 设备失败"));
        emit opened(false, 0, 0, 0);
        return;
    }

    const int canIndex = config.canIndex;

    // 通过设备属性路径配置波特率和 CAN FD 标准
    if (config.canType == TYPE_CANFD) {
        const QByteArray standard = QByteArray::number(config.canfdStandard);
        ZLGCanDriver::setValue(device, QString("%1/canfd_standard").arg(canIndex).toLatin1().constData(),
                               standard.constData());
        const QByteArray abit = QByteArray::number(config.abitBaudRate);
        ZLGCanDriver::setValue(device, QString("%1/canfd_abit_baud_rate").arg(canIndex).toLatin1().constData(),
                               abit.constData());
        const QByteArray dbit = QByteArray::number(config.dbitBaudRate);
        ZLGCanDriver::setValue(device, QString("%1/canfd_dbit_baud_rate").arg(canIndex).toLatin1().constData(),
                               dbit.constData());
    } else {
        const QByteArray baud = QByteArray::number(config.abitBaudRate);
        ZLGCanDriver::setValue(device, QString("%1/baud_rate").arg(canIndex).toLatin1().constData(),
                               baud.constData());
    }

    if (config.terminalResistance) {
        const QByteArray value("1");
        ZLGCanDriver::setValue(device, QString("%1/initenal_resistance").arg(canIndex).toLatin1().constData(),
                               value.constData());
    }

    ZCAN_CHANNEL_INIT_CONFIG initConfig;
    std::memset(&initConfig, 0, sizeof(initConfig));
    initConfig.can_type = static_cast<UINT>(config.canType);
    if (config.canType == TYPE_CANFD) {
        initConfig.canfd.acc_code = config.accCode;
        initConfig.canfd.acc_mask = config.accMask;
        initConfig.canfd.filter = static_cast<BYTE>(config.filter);
        initConfig.canfd.mode = static_cast<BYTE>(config.mode);
    } else {
        initConfig.can.acc_code = config.accCode;
        initConfig.can.acc_mask = config.accMask;
        initConfig.can.filter = static_cast<BYTE>(config.filter);
        initConfig.can.mode = static_cast<BYTE>(config.mode);
    }

    const CHANNEL_HANDLE channel = ZLGCanDriver::initCAN(device, static_cast<UINT>(canIndex), &initConfig);
    if (channel == INVALID_CHANNEL_HANDLE) {
        emit errorOccurred(ErrorType::ConfigError, tr("初始化 CAN 通道失败"));
        ZLGCanDriver::closeDevice(device);
        emit opened(false, 0, 0, 0);
        return;
    }

    ZLGCanDriver::clearBuffer(channel);
    if (ZLGCanDriver::startCAN(channel) == 0) {
        emit errorOccurred(ErrorType::CommandFailed,
                           readChannelError(reinterpret_cast<quintptr>(channel),
                                            tr("启动 CAN 通道失败")));
        ZLGCanDriver::closeDevice(device);
        emit opened(false, 0, 0, 0);
        return;
    }

    m_deviceHandle = reinterpret_cast<quintptr>(device);
    m_channelHandle = reinterpret_cast<quintptr>(channel);
    m_canIndex = canIndex;
    m_isOpen = true;

    emit opened(true, m_deviceHandle, m_canIndex, config.receiveWaitTime);
}

void CANWorker::doSend(const QByteArray &data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError, tr("CAN 设备未打开"));
        return;
    }

    const DecodedFrame frame = decodeFrame(data);
    if (!frame.valid || frame.data.size() > (frame.isFd ? 64 : 8)) {
        emit errorOccurred(ErrorType::InvalidParameter, tr("CAN 帧数据无效"));
        return;
    }

    emit perfEvent(makePerfEvent(PerfStage::TxIoBegin, frame.data));

    UINT sendCount = 0;
    if (frame.isFd) {
        ZCAN_TransmitFD_Data tx;
        std::memset(&tx, 0, sizeof(tx));
        tx.frame.can_id = frame.canIdRaw;
        tx.frame.len = static_cast<BYTE>(frame.data.size());
        tx.frame.flags = frame.brs ? CANFD_BRS : 0;
        std::memcpy(tx.frame.data, frame.data.constData(),
                    static_cast<size_t>(frame.data.size()));
        tx.transmit_type = 0;
        sendCount = ZLGCanDriver::transmitFD(
            reinterpret_cast<CHANNEL_HANDLE>(m_channelHandle), &tx, 1);
    } else {
        ZCAN_Transmit_Data tx;
        std::memset(&tx, 0, sizeof(tx));
        tx.frame.can_id = frame.canIdRaw;
        tx.frame.can_dlc = static_cast<BYTE>(frame.data.size());
        std::memcpy(tx.frame.data, frame.data.constData(),
                    static_cast<size_t>(frame.data.size()));
        tx.transmit_type = 0;
        sendCount = ZLGCanDriver::transmit(
            reinterpret_cast<CHANNEL_HANDLE>(m_channelHandle), &tx, 1);
    }

    if (sendCount != 1) {
        emit errorOccurred(ErrorType::WriteError,
                           readChannelError(m_channelHandle, tr("CAN 发送失败")));
    }
}

void CANWorker::doClose()
{
    if (!m_isOpen) {
        return;
    }

    if (m_channelHandle) {
        ZLGCanDriver::resetCAN(reinterpret_cast<CHANNEL_HANDLE>(m_channelHandle));
    }
    if (m_deviceHandle) {
        ZLGCanDriver::closeDevice(reinterpret_cast<DEVICE_HANDLE>(m_deviceHandle));
    }

    m_channelHandle = 0;
    m_deviceHandle = 0;
    m_isOpen = false;
    emit closed();
}

} // namespace CommIO
