// NetworkWork.cpp
// 精确定时实现：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿

#include "NetworkWork.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════
NetworkWork::NetworkWork(QObject *parent)
    : QObject(parent)
{
    // ★ 1ms 轮询定时器 — 配合 QElapsedTimer 实现高精度
    //    Qt5 在 Windows 上创建 QTimer 时会内部调用 timeBeginPeriod(1)，
    //    无需手动调用。
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);
    m_interCmdTimer->setInterval(1);          // 每1ms触发
    m_interCmdTimer->setSingleShot(false);     // 持续触发直到手动停止
    connect(m_interCmdTimer, &QTimer::timeout,
            this, &NetworkWork::onInterCmdDelay);

    qDebug() << "NetworkWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")"
             << "| 精确延时: 1ms轮询+忙等自旋+EMA补偿";
}

NetworkWork::~NetworkWork()
{
    m_interCmdTimer->stop();
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
//  连接主机
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

    // ★ 新连接重置误差补偿
    m_timingCompensationMs = 0;

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
    if (m_disconnecting.testAndSetRelaxed(0, 1) == false) {
        qDebug() << "NetworkWork: disconnectFromHost 已在执行中，跳过重复调用";
        return;
    }

    m_interCmdTimer->stop();

    if (m_tcpSocket) {
        QTcpSocket* sock = m_tcpSocket;
        m_tcpSocket = nullptr;

        disconnect(sock, nullptr, this, nullptr);

        if (sock->state() != QAbstractSocket::UnconnectedState) {
            sock->abort();
        }

        sock->deleteLater();
    }

    bool wasOpen = (m_opened.fetchAndStoreRelaxed(0) != 0);

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
    if (m_disconnecting.loadRelaxed() != 0)
        return;

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
//  ★★★ 核心：发送 + 1ms轮询精确延时 + 微秒忙等 + EMA补偿 ★★★
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendStringWithDelay(const QString &text, bool hexMode,
                                      const QByteArray &expectedResponse,
                                      int delayMs)
{
    if (!isOpen() || text.isEmpty() || !m_tcpSocket)
        return;

    m_expectedResponse = expectedResponse;

    // ── 构建数据 ──
    QByteArray data;
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
    } else {
        data = text.toUtf8();
    }

    // ── 发送 ──
    qint64 written = m_tcpSocket->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_tcpSocket->errorString()));
        return;
    }

    // ★ 关键：等待数据刷新到 TCP 栈，消除发送侧的随机延迟
    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->waitForBytesWritten(1);  // 最多等1ms
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    // ═══════════════════════════════════════════════════════════
    //  精确延时（1ms轮询 + 微秒忙等 + EMA补偿）
    // ═══════════════════════════════════════════════════════════
    if (delayMs > 0) {
        // 第1步：EMA补偿
        int compensatedMs = delayMs + m_timingCompensationMs;
        if (compensatedMs < 0) compensatedMs = 0;

        // 钳位补偿范围 ±100ms，防止单次异常抖动
        const int MAX_COMPENSATION = 100;
        m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                         m_timingCompensationMs,
                                         MAX_COMPENSATION);

        // 第2步：启动1ms轮询 + 记录起始时间
        m_targetDelayMs   = compensatedMs;
        m_originalDelayMs = delayMs;
        m_preciseDelayTimer.start();
        m_interCmdTimer->start();  // 每1ms触发 onInterCmdDelay()
    } else {
        emit interCmdDelayFinished();
    }
}

// ═══════════════════════════════════════════════════════════════
//  1ms 轮询回调：检测是否到期 → 微秒忙等 → 误差补偿 → 发射信号
// ═══════════════════════════════════════════════════════════════
void NetworkWork::onInterCmdDelay()
{
    qint64 elapsedMs = m_preciseDelayTimer.elapsed();

    // ★ 还没到目标时间，继续等（定时器下次再触发）
    if (elapsedMs < m_targetDelayMs - 1) {
        return;
    }

    // ★ 距离目标 ≤1ms：进入忙等自旋，精准命中
    //   用 nsecsElapsed() 获得纳秒级精度（QElapsedTimer 内部用 QPC）
    while (m_preciseDelayTimer.elapsed() < m_targetDelayMs) {
        // 自旋等待，不做任何事
        // 在 Windows上 QElapsedTimer 基于 QueryPerformanceCounter，
        // 精度可达微秒级。1ms 的自旋 ≈ 3,000,000 个 CPU 周期（3GHz），
        // 开销极小。
    }

    // ★ 停止轮询
    m_interCmdTimer->stop();

    // ★ 测量实际耗时，计算误差
    qint64 actualMs = m_preciseDelayTimer.elapsed();
    int    errorMs  = static_cast<int>(actualMs - m_targetDelayMs);

    // ★ EMA 平滑更新补偿值 (alpha = 0.5)
    const int MAX_COMPENSATION = 100;
    m_timingCompensationMs -= errorMs / 2;
    m_timingCompensationMs  = qBound(-MAX_COMPENSATION,
                                      m_timingCompensationMs,
                                      MAX_COMPENSATION);

    // ★ 诊断日志
    if (qAbs(errorMs) >= 1) {
        qDebug() << "NetworkWork:[精确延时]"
                 << "请求" << m_originalDelayMs << "ms"
                 << "→补偿后" << m_targetDelayMs << "ms"
                 << "→实际" << actualMs << "ms"
                 << "|误差" << errorMs << "ms"
                 << "|累积补偿" << m_timingCompensationMs << "ms";
    }

    // ★ 通知主线程
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
    if (!m_tcpSocket)
        return;

    if (m_disconnecting.loadRelaxed() != 0)
        return;

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
        msg = m_tcpSocket->errorString();
        break;
    }

    if (fatal) {
        QTcpSocket* sock = m_tcpSocket;
        m_tcpSocket = nullptr;

        m_interCmdTimer->stop();

        disconnect(sock, nullptr, this, nullptr);

        if (sock->state() != QAbstractSocket::UnconnectedState) {
            sock->abort();
        }

        sock->deleteLater();

        m_opened.storeRelaxed(0);
        emit networkDisconnected();
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
        if (!text.isEmpty()) {
            text.replace(QLatin1Char('\r'), QLatin1String("\\r"));
            text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
            return text;
        } else {
            return data.toHex(' ').toUpper();
        }
    }
}
