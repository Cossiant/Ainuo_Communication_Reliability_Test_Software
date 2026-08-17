#include "CommIOModule/TCPChannel.h"
#include "CommIOModule/PerfClock.h"

#include <QHostAddress>
#include <QNetworkProxy>

namespace CommIO {

namespace {

CommIO::PerfEvent makePerfEvent(CommIO::PerfStage stage, const QByteArray& data)
{
    CommIO::PerfEvent event;
    event.channel = CommIO::ChannelType::TCP;
    event.stage   = stage;
    event.epochUs = CommIO::PerfClock::epochUs();
    event.data    = data;
    return event;
}

} // namespace

TCPChannel::TCPChannel(QObject* parent)
    : ICommChannel(parent)
{
    m_socket = new QTcpSocket(this);
    m_socket->setProxy(QNetworkProxy::NoProxy);  // 工业设备直连，禁用系统代理
    m_connectTimer = new QTimer(this);
    m_connectTimer->setSingleShot(true);
    m_connectTimer->setTimerType(Qt::PreciseTimer);
    qRegisterMetaType<CommIO::ErrorType>();
    qRegisterMetaType<QAbstractSocket::SocketError>();

    connect(m_socket, &QTcpSocket::connected,
            this, &TCPChannel::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TCPChannel::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TCPChannel::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &TCPChannel::onSocketErrorOccurred);
    connect(m_connectTimer, &QTimer::timeout,
            this, &TCPChannel::onConnectTimeout);
}

TCPChannel::~TCPChannel()
{
    close();
}

bool TCPChannel::open(const QVariantMap& config)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        emit errorOccurred(ErrorType::AlreadyOpen,
                           tr("TCP 连接已存在"));
        return false;
    }

    TCPConfig cfg = parseConfig(config);

    if (cfg.host.isEmpty()) {
        emit errorOccurred(ErrorType::InvalidParameter,
                           tr("主机地址为空"));
        return false;
    }

    QHostAddress hostAddress;
    if (!hostAddress.setAddress(cfg.host)
        || hostAddress.isNull()
        || hostAddress == QHostAddress::Any
        || hostAddress == QHostAddress::AnyIPv6
        || hostAddress.isBroadcast()) {
        emit errorOccurred(ErrorType::InvalidAddress,
                           tr("IP 地址格式无效: %1，请输入合法的 IPv4 地址").arg(cfg.host));
        return false;
    }

    if (cfg.port == 0) {
        emit errorOccurred(ErrorType::InvalidParameter,
                           tr("端口号无效"));
        return false;
    }

    // 应用 Socket 选项
    if (cfg.keepAlive) {
        m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    }
    if (cfg.lowDelay) {
        m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }
    if (cfg.readBufferSize > 0) {
        m_socket->setReadBufferSize(cfg.readBufferSize);
    }

    // 发起异步连接
    m_isConnecting = true;
    m_socket->connectToHost(hostAddress, cfg.port);

    // 启动连接超时定时器
    m_connectTimer->start(cfg.connectTimeout);

    return true; // 异步连接，true 仅表示发起成功
}

void TCPChannel::close()
{
    m_connectTimer->stop();
    m_isConnecting = false;

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        // 等待断开完成（最多 1 秒）
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
}

void TCPChannel::send(const QByteArray& data)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("TCP 未连接"));
        return;
    }

    emit perfEvent(makePerfEvent(CommIO::PerfStage::TxIoBegin, data));
    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten < 0) {
        emit errorOccurred(ErrorType::WriteError,
                           tr("TCP 写入失败: %1").arg(m_socket->errorString()));
    }
}

bool TCPChannel::isOpen() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

TCPConfig TCPChannel::parseConfig(const QVariantMap& config) const
{
    TCPConfig cfg;

    cfg.host = config.value("host").toString().trimmed();
    cfg.port = static_cast<quint16>(config.value("port", 0).toUInt());
    cfg.connectTimeout = config.value("connectTimeout", 3000).toInt();
    cfg.readTimeout = config.value("readTimeout", 1000).toInt();
    cfg.keepAlive = config.value("keepAlive", false).toBool();
    cfg.lowDelay = config.value("lowDelay", false).toBool();
    cfg.readBufferSize = config.value("readBufferSize", 0).toLongLong();

    return cfg;
}

void TCPChannel::onConnected()
{
    m_connectTimer->stop();
    m_isConnecting = false;
    emit connectionStateChanged(true);
}

void TCPChannel::onDisconnected()
{
    m_connectTimer->stop();
    m_isConnecting = false;
    emit connectionStateChanged(false);
}

void TCPChannel::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        emit perfEvent(makePerfEvent(CommIO::PerfStage::RxIoFirst, data));
        emit dataReceived(data);
    }
}

void TCPChannel::onSocketErrorOccurred(QAbstractSocket::SocketError error)
{
    // 连接过程中的错误，停止超时定时器
    if (m_isConnecting) {
        m_connectTimer->stop();
        m_isConnecting = false;
    }

    // RemoteHostClosedError 在 disconnected 信号中已处理状态变化
    ErrorType errType = ErrorHelper::fromSocketError(error);
    emit errorOccurred(errType, m_socket->errorString());
}

void TCPChannel::onConnectTimeout()
{
    if (!m_isConnecting) {
        return;
    }
    m_isConnecting = false;

    // 中止连接尝试
    m_socket->abort();
    emit errorOccurred(ErrorType::TimeoutError,
                       tr("TCP 连接超时,请检查IP或端口是否正确配置"));
}

} // namespace CommIO
