// NetworkWork.cpp
// 方案三实现（TCP 客户端版）
// ★ 可选禁用 Nagle 算法（由勾选框控制）
// ★ 修复：连接错误时使用 deleteLater 避免闪退
// ★ 修复：disconnectFromHost 增加重入保护

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
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setSingleShot(true);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);
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
// ═══════════════════════════════════════════════════════════════
void NetworkWork::connectToHost(const QString &ipAddress,
                                quint16 port,
                                bool disableNagle)
{
    if (m_tcpSocket) {
        disconnectFromHost();
    }

    m_tcpSocket = new QTcpSocket(this);

    if (disableNagle) {
        m_tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }

    connect(m_tcpSocket, &QTcpSocket::connected,
            this, &NetworkWork::onConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected,
            this, &NetworkWork::onDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead,
            this, &NetworkWork::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred,
            this, &NetworkWork::onSocketError);

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
//  ★ 修复：增加重入保护，避免竞态导致闪退
// ═══════════════════════════════════════════════════════════════
void NetworkWork::disconnectFromHost()
{
    // ★ 重入保护：如果已经在断开流程中，直接返回
    if (m_disconnecting.testAndSetRelaxed(0, 1) == false) {
        qDebug() << "NetworkWork: disconnectFromHost 已在执行中，跳过重复调用";
        return;
    }

    m_interCmdTimer->stop();

    if (m_tcpSocket) {
        // ★ 先取出指针并置空，防止 onSocketError 等回调中重复操作
        QTcpSocket* sock = m_tcpSocket;
        m_tcpSocket = nullptr;

        // 断开所有信号连接，防止后续 socket 事件触发回调
        disconnect(sock, nullptr, this, nullptr);

        if (sock->state() != QAbstractSocket::UnconnectedState) {
            sock->abort();  // ★ 立即中止，避免 waitForDisconnected 阻塞
        }

        // ★ 使用 deleteLater 安全删除，避免在信号处理栈中直接 delete
        sock->deleteLater();
    }

    bool wasOpen = (m_opened.fetchAndStoreRelaxed(0) != 0);

    // ★ 只有之前是已连接状态才发断开信号（连接失败时不发重复信号）
    if (wasOpen) {
        emit networkDisconnected();
    }

    m_disconnecting.storeRelaxed(0);

    qDebug() << "NetworkWork: TCP 已断开"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  被动断开回调
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onDisconnected()
{
    m_interCmdTimer->stop();

    // ★ 如果正在主动断开中，不重复处理
    if (m_disconnecting.loadRelaxed() != 0)
        return;

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

    m_expectedResponse = expectedResponse;

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

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

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
//  ★ 修复：
//    1. 使用 deleteLater 替代直接 delete，避免在信号回调栈中删除 socket
//    2. 先保存错误信息，再清理 socket
//    3. 防止 onSocketError 重入
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onSocketError(QAbstractSocket::SocketError error)
{
    // ★ 如果 socket 已经被清理（可能被超时或其他路径抢先），直接返回
    if (!m_tcpSocket)
        return;

    // ★ 如果正在断开中，不重复处理
    if (m_disconnecting.loadRelaxed() != 0)
        return;

    // ★ 先提取错误信息（在清理 socket 之前）
    QString msg;
    bool fatal = false;

    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        msg = "连接被拒绝，请检查目标 IP 和端口";
        fatal = true;
        break;
    case QAbstractSocket::RemoteHostClosedError:
        msg = "远程主机关闭了连接";
        fatal = true;
        break;
    case QAbstractSocket::HostNotFoundError:
        msg = "找不到主机，请检查 IP 地址";
        fatal = true;
        break;
    case QAbstractSocket::SocketTimeoutError:
        msg = "Socket 操作超时";
        fatal = true;
        break;
    case QAbstractSocket::NetworkError:
        msg = "网络错误，连接中断";
        fatal = true;
        break;
    default:
        // ★ 在 socket 还存在时保存错误信息
        msg = m_tcpSocket->errorString();
        break;
    }

    // ★ 致命错误：安全清理 socket
    if (fatal) {
        // 先取出 socket 指针并置空，防止重复操作
        QTcpSocket* sock = m_tcpSocket;
        m_tcpSocket = nullptr;

        m_interCmdTimer->stop();

        // 断开所有信号
        disconnect(sock, nullptr, this, nullptr);

        // 立即中止连接
        if (sock->state() != QAbstractSocket::UnconnectedState) {
            sock->abort();
        }

        // ★ 使用 deleteLater 安全删除
        sock->deleteLater();

        m_opened.storeRelaxed(0);

        // ★ 连接失败时也发 networkDisconnected，让 UI 恢复初始状态
        emit networkDisconnected();
    }

    // ★ 最后发射错误信号（此时 socket 已安全清理或保留）
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
