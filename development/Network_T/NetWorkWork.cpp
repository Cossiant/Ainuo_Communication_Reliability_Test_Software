// NetworkWork.cpp
// 方案三实现（TCP 客户端版）
// ★ 可选禁用 Nagle 算法（由勾选框控制）

#include "NetworkWork.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════
NetworkWork::NetworkWork(QObject *parent)
    : QObject(parent)
{
    // ★ 命令间隔精确延时定时器（工作线程内）
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setSingleShot(true);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);   // ★ 1ms 精度
    connect(m_interCmdTimer, &QTimer::timeout,
            this, &NetworkWork::onInterCmdDelay);

    qDebug() << "NetworkWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")";
}

NetworkWork::~NetworkWork()
{
    disconnectFromHost();
    qDebug() << "NetworkWork: 已销毁";
}

// ═══════════════════════════════════════════════════════════════
//  查询 / 设置 接口
// ═══════════════════════════════════════════════════════════════
bool NetworkWork::isOpen() const
{
    return m_opened.loadRelaxed() != 0;
}

int NetworkWork::totalRecvCount() const
{
    return m_totalRecv;
}

void NetworkWork::resetRecvCount()
{
    m_totalRecv = 0;
    emit recvCountChanged(0);
}

void NetworkWork::setExpectedResponse(const QByteArray &expected)
{
    m_expectedResponse = expected;
}

QByteArray NetworkWork::expectedResponse() const
{
    return m_expectedResponse;
}

void NetworkWork::setHexDisplayMode(bool hexMode)
{
    m_hexDisplay = hexMode;
}

// ═══════════════════════════════════════════════════════════════
//  连接主机（TCP 客户端，异步连接）
//  ★ disableNagle：勾选时才禁用 Nagle 算法
// ═══════════════════════════════════════════════════════════════
void NetworkWork::connectToHost(const QString &ipAddress,
                                quint16 port,
                                bool disableNagle)
{
    if (m_tcpSocket) {
        disconnectFromHost();
    }

    m_tcpSocket = new QTcpSocket(this);

    // ★ 根据勾选框决定是否禁用 Nagle 算法
    if (disableNagle) {
        m_tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }

    // ★ 异步连接信号
    connect(m_tcpSocket, &QTcpSocket::connected,
            this, &NetworkWork::onConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected,
            this, &NetworkWork::onDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead,
            this, &NetworkWork::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred,
            this, &NetworkWork::onSocketError);

    // ★ 异步发起连接
    m_tcpSocket->connectToHost(ipAddress, port);

    qDebug() << "NetworkWork: 正在连接" << ipAddress << ":" << port
             << (disableNagle ? "(Nagle 已禁用)" : "(Nagle 正常)")
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  连接成功回调
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onConnected()
{
    m_opened.storeRelaxed(1);
    emit networkConnected();

    qDebug() << "NetworkWork: TCP 已连接"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  断开连接
// ═══════════════════════════════════════════════════════════════
void NetworkWork::disconnectFromHost()
{
    m_interCmdTimer->stop();   // ★ 停止命令间隔定时器

    if (m_tcpSocket) {
        disconnect(m_tcpSocket, nullptr, this, nullptr);

        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->disconnectFromHost();

            // 短暂等待断开（在工作线程内不会阻塞 UI）
            if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
                m_tcpSocket->waitForDisconnected(1000);
            }
        }

        delete m_tcpSocket;
        m_tcpSocket = nullptr;
    }

    m_opened.storeRelaxed(0);
    emit networkDisconnected();

    qDebug() << "NetworkWork: TCP 已断开"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  被动断开回调
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onDisconnected()
{
    m_interCmdTimer->stop();

    m_opened.storeRelaxed(0);
    emit networkDisconnected();

    qDebug() << "NetworkWork: TCP 被动断开"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  发送原始字节
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendData(const QByteArray &data)
{
    if (!isOpen() || data.isEmpty() || !m_tcpSocket)
        return;

    qint64 written = m_tcpSocket->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_tcpSocket->errorString()));
        return;
    }
    if (written < data.size()) {
        qDebug() << "NetworkWork: 部分发送" << written << "/" << data.size();
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));
}

// ═══════════════════════════════════════════════════════════════
//  发送字符串
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendString(const QString &text, bool hexMode)
{
    if (!isOpen() || text.isEmpty())
        return;

    QByteArray data;
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
    } else {
        data = text.toUtf8();
    }

    sendData(data);
}

// ═══════════════════════════════════════════════════════════════
//  ★ 方案三核心：发送命令 + 工作线程内精确延时
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendStringWithDelay(const QString &text, bool hexMode,
                                      const QByteArray &expectedResponse,
                                      int delayMs)
{
    if (!isOpen() || text.isEmpty() || !m_tcpSocket)
        return;

    // 1. 设置期望返回值
    m_expectedResponse = expectedResponse;

    // 2. 编码并发送
    QByteArray data;
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
    } else {
        data = text.toUtf8();
    }

    qint64 written = m_tcpSocket->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_tcpSocket->errorString()));
        return;
    }

    // 3. 发送日志
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    // 4. ★ 在工作线程内启动精确延时
    if (delayMs > 0) {
        m_interCmdTimer->start(delayMs);
    } else {
        emit interCmdDelayFinished();
    }
}

// ═══════════════════════════════════════════════════════════════
//  ★ 精确延时到期（工作线程内触发）
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onInterCmdDelay()
{
    emit interCmdDelayFinished();
}

// ═══════════════════════════════════════════════════════════════
//  处理 readyRead（网口直接透传，不合并缓冲）
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onReadyRead()
{
    if (!m_tcpSocket)
        return;

    QByteArray chunk = m_tcpSocket->readAll();
    emitData(chunk);
}

// ═══════════════════════════════════════════════════════════════
//  处理 Socket 错误
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    if (!m_tcpSocket)
        return;

    QString msg;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        msg = "连接被拒绝，请检查目标 IP 和端口";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        msg = "远程主机关闭了连接";
        disconnectFromHost();
        return;
    case QAbstractSocket::HostNotFoundError:
        msg = "找不到主机，请检查 IP 地址";
        break;
    case QAbstractSocket::SocketTimeoutError:
        msg = "Socket 操作超时";
        break;
    case QAbstractSocket::NetworkError:
        msg = "网络错误，连接中断";
        disconnectFromHost();
        return;
    default:
        msg = m_tcpSocket->errorString();
        break;
    }

    emit errorOccurred(msg);
}

// ═══════════════════════════════════════════════════════════════
//  内部：统一的数据输出入口
// ═══════════════════════════════════════════════════════════════
void NetworkWork::emitData(const QByteArray &data)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit recvLogLine(QString("[%1] RX ← %2").arg(timeStr, display));

    m_totalRecv++;
    emit recvCountChanged(m_totalRecv);

    emit dataReceived(data);
    emit responseReceived(data);
}

// ═══════════════════════════════════════════════════════════════
//  内部：格式化字节数组
// ═══════════════════════════════════════════════════════════════
QString NetworkWork::formatByteArray(const QByteArray &data) const
{
    if (m_hexDisplay) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty())
            return text;
        else
            return data.toHex(' ').toUpper();
    }
}
