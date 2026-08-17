#include "CommIOModule/SerialPortChannel.h"
#include "CommIOModule/PerfClock.h"

#include <QCoreApplication>

namespace CommIO {

namespace {

CommIO::PerfEvent makePerfEvent(CommIO::PerfStage stage, const QByteArray& data)
{
    CommIO::PerfEvent event;
    event.channel = CommIO::ChannelType::SerialPort;
    event.stage   = stage;
    event.epochUs = CommIO::PerfClock::epochUs();
    event.data    = data;
    return event;
}

QString serialPortErrorReason(QSerialPort::SerialPortError error)
{
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "未找到串口设备（端口不存在、设备已拔出或驱动未就绪）");
    case QSerialPort::PermissionError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口访问被拒绝（可能被其他程序占用或权限不足）");
    case QSerialPort::OpenError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口打开失败（设备状态异常或重复打开）");
    case QSerialPort::NotOpenError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口尚未打开，当前操作无效");
    case QSerialPort::ParityError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口奇偶校验错误（请检查校验位配置）");
    case QSerialPort::FramingError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口帧格式错误（请检查波特率/数据位/停止位配置）");
    case QSerialPort::BreakConditionError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "检测到 Break 条件（总线电平异常或对端发送中断）");
    case QSerialPort::WriteError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口写入失败（发送阶段发生错误）");
    case QSerialPort::ReadError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口读取失败（接收阶段发生错误）");
    case QSerialPort::ResourceError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口资源错误（设备断开、驱动异常或系统资源不可用）");
    case QSerialPort::UnsupportedOperationError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口不支持当前操作");
    case QSerialPort::TimeoutError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口操作超时");
    case QSerialPort::UnknownError:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口发生未知错误");
    case QSerialPort::NoError:
    default:
        return QCoreApplication::translate("CommIO::SerialPortChannel", "串口状态正常");
    }
}

} // namespace

SerialPortChannel::SerialPortChannel(QObject* parent)
    : ICommChannel(parent)
{
    m_serialPort = new QSerialPort(this);
    qRegisterMetaType<QSerialPort::SerialPortError>();
    qRegisterMetaType<CommIO::ErrorType>();

    connect(m_serialPort, &QSerialPort::readyRead,
            this, &SerialPortChannel::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &SerialPortChannel::onSerialPortErrorOccurred);
}

SerialPortChannel::~SerialPortChannel()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

bool SerialPortChannel::open(const QVariantMap& config)
{
    if (m_serialPort->isOpen()) {
        emit errorOccurred(ErrorType::AlreadyOpen,
                           tr("串口已打开"));
        return false;
    }

    SerialPortConfig cfg = parseConfig(config);

    if (cfg.portName.isEmpty()) {
        emit errorOccurred(ErrorType::InvalidParameter,
                           tr("端口名称为空"));
        return false;
    }

    // 应用配置
    m_serialPort->setPortName(cfg.portName);
    m_serialPort->setBaudRate(cfg.baudRate);
    m_serialPort->setDataBits(cfg.dataBits);
    m_serialPort->setParity(cfg.parity);
    m_serialPort->setStopBits(cfg.stopBits);
    m_serialPort->setFlowControl(cfg.flowControl);

    if (cfg.readBufferSize > 0) {
        m_serialPort->setReadBufferSize(cfg.readBufferSize);
    }

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        // ErrorType errType = ErrorHelper::fromSerialPortError(m_serialPort->error());
        // emit errorOccurred(errType,
        //                    QStringLiteral("打开串口失败: %1").arg(m_serialPort->errorString()));
        return false;
    }

    // 打开成功后设置 DTR/RTS 信号
    m_serialPort->setDataTerminalReady(cfg.dataTerminalReady);
    m_serialPort->setRequestToSend(cfg.requestToSend);

    emit connectionStateChanged(true);
    return true;
}

void SerialPortChannel::close()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStateChanged(false);
    }
}

void SerialPortChannel::send(const QByteArray& data)
{
    if (!m_serialPort->isOpen()) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("串口未打开"));
        return;
    }

    emit perfEvent(makePerfEvent(CommIO::PerfStage::TxIoBegin, data));
    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten < 0) {
        emit errorOccurred(ErrorType::WriteError,
                           tr("串口写入失败: %1").arg(m_serialPort->errorString()));
    }
}

bool SerialPortChannel::isOpen() const
{
    return m_serialPort->isOpen();
}

SerialPortConfig SerialPortChannel::parseConfig(const QVariantMap& config) const
{
    SerialPortConfig cfg;

    cfg.portName = config.value("portName").toString();
    cfg.baudRate = config.value("baudRate", 9600).toInt();
    cfg.dataBits = static_cast<QSerialPort::DataBits>(
        config.value("dataBits", QSerialPort::Data8).toInt());
    cfg.parity = static_cast<QSerialPort::Parity>(
        config.value("parity", QSerialPort::NoParity).toInt());
    cfg.stopBits = static_cast<QSerialPort::StopBits>(
        config.value("stopBits", QSerialPort::OneStop).toInt());
    cfg.flowControl = static_cast<QSerialPort::FlowControl>(
        config.value("flowControl", QSerialPort::NoFlowControl).toInt());
    cfg.dataTerminalReady = config.value("dataTerminalReady", false).toBool();
    cfg.requestToSend = config.value("requestToSend", false).toBool();
    cfg.readTimeout = config.value("readTimeout", 1000).toInt();
    cfg.readBufferSize = config.value("readBufferSize", 0).toLongLong();

    return cfg;
}

void SerialPortChannel::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit perfEvent(makePerfEvent(CommIO::PerfStage::RxIoFirst, data));
        emit dataReceived(data);
    }
}

void SerialPortChannel::onSerialPortErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    ErrorType errType = ErrorHelper::fromSerialPortError(error);
    const QString reason = serialPortErrorReason(error);
    const QString nativeError = m_serialPort ? m_serialPort->errorString() : QString();

    QString message = tr("%1（错误码: %2）")
                          .arg(reason)
                          .arg(static_cast<int>(error));
    if (!nativeError.isEmpty()) {
        message += tr("，系统信息: %1").arg(nativeError);
    }

    emit errorOccurred(errType, message);

    // 设备被移除等致命错误，自动关闭并通知状态变化
    if (error == QSerialPort::ResourceError || error == QSerialPort::WriteError) {
        m_serialPort->close();
        emit connectionStateChanged(false);
    }
}

} // namespace CommIO
