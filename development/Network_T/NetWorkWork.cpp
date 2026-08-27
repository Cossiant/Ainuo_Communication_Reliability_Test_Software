// NetworkWork.cpp
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 计时起点移到 write 之前 + forceRead 判断
// ★ 新增：发送后缀功能
// ★ 新增：代际标记防止信号串扰

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

    // ★ 阶段二：预投递命令的截止等待定时器
    m_holdTimer = new QTimer(this);
    m_holdTimer->setTimerType(Qt::PreciseTimer);
    m_holdTimer->setInterval(1);
    m_holdTimer->setSingleShot(false);
    connect(m_holdTimer, &QTimer::timeout,
            this, &NetworkWork::onHoldTick);

    // ★ 发送时间戳时间轴（阶段一：误差测量日志）
    m_txClock.start();
    m_txClockWallAnchorMs = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "NetworkWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")"
             << "| 精确延时: 1ms轮询+忙等自旋+EMA补偿";
}

NetworkWork::~NetworkWork()
{
    m_interCmdTimer->stop();
    m_holdTimer->stop();
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

// ★ 新增：设置发送后缀模式
void NetworkWork::setSuffixMode(int mode)
{
    m_suffixMode = mode;
    qDebug() << "NetworkWork: 后缀模式 =" << mode
             << (mode == 0 ? "None" : mode == 1 ? "CR" : mode == 2 ? "LF" : "CRLF");
}

// ★ 新增：重置误差补偿（每次批量发送开始时调用）
void NetworkWork::resetTimingCompensation()
{
    m_timingCompensationMs = 0;
    m_prevTxAnchorElapsedMs = -1;   // ★ 新批次重新锚定第一条命令的发送时刻
    qDebug() << "NetworkWork: 误差补偿已重置";
}

// ★ 新增：统一构建发送数据
QByteArray NetworkWork::buildSendData(const QString &text, bool hexMode) const
{
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        return QByteArray::fromHex(hex.toLatin1());
    }

    // Step 1: 将用户输入的 \r \n 转义还原为真实控制字符
    QString unescaped = text;
    unescaped.replace(QLatin1String("\\r"), QLatin1String("\r"));
    unescaped.replace(QLatin1String("\\n"), QLatin1String("\n"));

    QByteArray data = unescaped.toUtf8();

    // Step 2: 去掉末尾已有的 \r \n（避免与后缀重复）
    while (!data.isEmpty()) {
        char last = data.at(data.size() - 1);
        if (last == '\r' || last == '\n')
            data.chop(1);
        else
            break;
    }

    // Step 3: 追加用户选择的后缀
    //   0 = None, 1 = CR (\r), 2 = LF (\n), 3 = CRLF (\r\n)
    switch (m_suffixMode) {
        case 1:   data.append('\r');          break;   // CR
        case 2:   data.append('\n');          break;   // LF
        case 3:   data.append("\r\n");        break;   // CRLF
        case 0:                                break;   // None
        default:                               break;
    }

    return data;
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
    m_lastWriteElapsedMs = -1;   // 新会话重新锚定“两次 write 间隔”日志
    m_prevTxAnchorElapsedMs = -1;   // 新会话重置绝对发送锚点
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

    cancelPendingCommands();   // ★ 停止间隔/截止定时器并丢弃预投递命令

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

    cancelPendingCommands();
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
//  发送字符串（★ 改用 buildSendData 统一构建）
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendString(const QString &text, bool hexMode)
{
    if (!isOpen() || text.isEmpty())
        return;

    QByteArray data = buildSendData(text, hexMode);   // ★ 使用统一构建方法

    sendData(data);
}

// ═══════════════════════════════════════════════════════════════
//  ★★★ 核心：发送 + 1ms轮询精确延时 + 微秒忙等 + EMA补偿 ★★★
//  ★ 计时起点在 write 之前 + forceRead 控制读取
//  ★ generation：代际标记，到期时原样传回供 NetworkExcel 校验
// ═══════════════════════════════════════════════════════════════
void NetworkWork::sendStringWithDelay(const QString &text, bool hexMode,
                                      const QByteArray &expectedResponse,
                                      int delayMs,
                                      bool forceRead,
                                      int generation)
{
    if (!isOpen() || text.isEmpty() || !m_tcpSocket)
        return;

    // ★ 阶段二：预投递 + 按住到“上一条 write + 本条延时”的绝对截止时刻
    m_pendingSend.text       = text;
    m_pendingSend.hexMode    = hexMode;
    m_pendingSend.expected   = expectedResponse;
    m_pendingSend.delayMs    = delayMs;
    m_pendingSend.forceRead  = forceRead;
    m_pendingSend.generation = generation;
    m_hasPendingSend = true;

    qint64 arrivalElapsedMs = m_txClock.elapsed();
    m_holdTargetMs = (m_prevTxAnchorElapsedMs >= 0)
            ? m_prevTxAnchorElapsedMs + delayMs
            : arrivalElapsedMs;
    m_expectedWriteElapsedMs = m_holdTargetMs;

    if (m_holdTargetMs <= arrivalElapsedMs) {
        writePendingAtDeadline();
    } else {
        m_holdTimer->start();
    }
}

void NetworkWork::onHoldTick()
{
    if (!m_hasPendingSend) {
        m_holdTimer->stop();
        return;
    }

    qint64 elapsedMs = m_txClock.elapsed();
    if (elapsedMs < m_holdTargetMs - 1)
        return;

    while (m_txClock.elapsed() < m_holdTargetMs) {
        // 最后 1ms 忙等自旋，精准命中截止时刻
    }

    writePendingAtDeadline();
}

void NetworkWork::writePendingAtDeadline()
{
    if (!m_hasPendingSend || !isOpen() || !m_tcpSocket)
        return;

    PendingSend s = m_pendingSend;
    m_hasPendingSend = false;
    m_holdTimer->stop();

    m_expectedResponse = s.expected;
    m_currentGeneration = s.generation;

    QByteArray data = buildSendData(s.text, s.hexMode);

    qint64 written = m_tcpSocket->write(data);
    qint64 actualWriteElapsedMs = m_txClock.elapsed();
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_tcpSocket->errorString()));
        emit commandWritten(s.generation);
        return;
    }

    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->waitForBytesWritten(1);
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    qint64 intervalMs = (m_lastWriteElapsedMs >= 0)
            ? actualWriteElapsedMs - m_lastWriteElapsedMs
            : -1;
    qint64 writeErrorMs = actualWriteElapsedMs - m_expectedWriteElapsedMs;
    m_lastWriteElapsedMs = actualWriteElapsedMs;
    m_prevTxAnchorElapsedMs = actualWriteElapsedMs;

    qDebug() << "[TxTiming][Network]"
             << "gen=" << s.generation
             << "delay=" << s.delayMs << "ms"
             << "expectedSend="
             << QDateTime::fromMSecsSinceEpoch(m_txClockWallAnchorMs + m_expectedWriteElapsedMs)
                    .toString("HH:mm:ss.zzz")
             << "actualWrite="
             << QDateTime::fromMSecsSinceEpoch(m_txClockWallAnchorMs + actualWriteElapsedMs)
                    .toString("HH:mm:ss.zzz")
             << "writeError=" << writeErrorMs << "ms"
             << "interval=" << intervalMs << "ms";

    emit commandWritten(s.generation);

    bool shouldRead = s.forceRead || !m_expectedResponse.isEmpty();
    if (!shouldRead) {
        emit responseReceived(QByteArray());
    }
}

void NetworkWork::cancelPendingCommands()
{
    m_hasPendingSend = false;
    m_holdTimer->stop();
    m_interCmdTimer->stop();
    m_expectedResponse.clear();
}

// ═══════════════════════════════════════════════════════════════
//  1ms 轮询回调：检测是否到期 → 微秒忙等 → 误差补偿 → 发射信号
// ═══════════════════════════════════════════════════════════════
void NetworkWork::startDelayOnly(int delayMs, int generation)
{
    // 粘包分包分支复用：仅执行精确延时，到期原样传回代际
    m_currentGeneration = generation;

    if (delayMs <= 0) {
        // ★ 粘包伪 write：把当前时刻作为下一条命令的发送锚点
        m_prevTxAnchorElapsedMs = m_txClock.elapsed();
        emit interCmdDelayFinished(m_currentGeneration);
        return;
    }

    m_originalDelayMs = delayMs;
    m_preciseDelayTimer.start();

    int compensatedMs = delayMs + m_timingCompensationMs;
    if (compensatedMs < 0) compensatedMs = 0;

    const int MAX_COMPENSATION = 100;
    m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                     m_timingCompensationMs,
                                     MAX_COMPENSATION);

    m_targetDelayMs = compensatedMs;
    m_interCmdTimer->start();
}

void NetworkWork::onInterCmdDelay()
{
    qint64 elapsedMs = m_preciseDelayTimer.elapsed();

    // ★ 还没到目标时间，继续等（定时器下次再触发）
    if (elapsedMs < m_targetDelayMs - 1) {
        return;
    }

    // ★ 距离目标 ≤1ms：进入忙等自旋，精准命中
    while (m_preciseDelayTimer.elapsed() < m_targetDelayMs) {
        // 自旋等待，不做任何事
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
                 << "gen" << m_currentGeneration
                 << "请求" << m_originalDelayMs << "ms"
                 << "→补偿后" << m_targetDelayMs << "ms"
                 << "→实际" << actualMs << "ms"
                 << "|误差" << errorMs << "ms"
                 << "|累积补偿" << m_timingCompensationMs << "ms";
    }

    // ★ 粘包“仅延时”结束：更新下一条命令的发送锚点，再通知主线程
    m_prevTxAnchorElapsedMs = m_txClock.elapsed();
    emit interCmdDelayFinished(m_currentGeneration);
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

        cancelPendingCommands();

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
