#include "CANWork.h"

#include "CANFrameUtils.h"
#include "CommIOModule/CANChannel.h"
#include "CommIOModule/CommTypes.h"

#include <QDateTime>
#include <QDebug>
#include <QVariantMap>
#include <QThread>

CANWork::CANWork(QObject *parent)
    : QObject(parent)
{
    // CommIO::CANChannel 自带独立的发送线程和接收线程
    m_channel = new CommIO::CANChannel(this);

    connect(m_channel, &CommIO::CANChannel::dataReceived,
            this, &CANWork::onChannelData);
    connect(m_channel, &CommIO::CANChannel::connectionStateChanged,
            this, &CANWork::onChannelStateChanged);
    connect(m_channel, &CommIO::CANChannel::errorOccurred,
            this, [this](CommIO::ErrorType type, const QString &message) {
        emit errorOccurred(QStringLiteral("%1：%2")
                               .arg(CommIO::ErrorHelper::errorTypeToString(type), message));
    });

    m_holdTimer = new QTimer(this);
    m_holdTimer->setTimerType(Qt::PreciseTimer);
    m_holdTimer->setInterval(1);
    m_holdTimer->setSingleShot(false);
    connect(m_holdTimer, &QTimer::timeout, this, &CANWork::onHoldTick);

    m_txClock.start();
    m_txClockWallAnchorMs = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "CANWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")";
}

CANWork::~CANWork()
{
    m_holdTimer->stop();
    if (m_channel) {
        m_channel->close();
    }
}

bool CANWork::isOpen() const
{
    return m_opened.loadRelaxed() != 0;
}

int CANWork::totalRecvCount() const
{
    return m_totalRecv;
}

void CANWork::openCANPort(int deviceType,
                          int deviceIndex,
                          int canIndex,
                          int canType,
                          int abitBaudRate,
                          int dbitBaudRate,
                          int canfdStandard,
                          int terminalResistance,
                          quint32 accCode,
                          quint32 accMask,
                          int filter,
                          int mode)
{
    QVariantMap config;
    config["deviceType"] = deviceType;
    config["deviceIndex"] = deviceIndex;
    config["canIndex"] = canIndex;
    config["canType"] = canType;
    config["abitBaudRate"] = abitBaudRate;
    config["dbitBaudRate"] = dbitBaudRate;
    config["canfdStandard"] = canfdStandard;
    config["terminalResistance"] = terminalResistance;
    config["accCode"] = accCode;
    config["accMask"] = accMask;
    config["filter"] = filter;
    config["mode"] = mode;
    config["receiveWaitTime"] = 100;

    if (!m_channel->open(config)) {
        emit errorOccurred(QStringLiteral("CAN 打开请求失败"));
    }
}

void CANWork::closeCANPort()
{
    cancelPendingCommands();
    if (m_channel) {
        m_channel->close();
    }
}

void CANWork::sendFrameWithDelay(const QByteArray &frame,
                                 const QByteArray &expectedResponse,
                                 int delayMs,
                                 bool forceRead,
                                 int generation)
{
    if (!isOpen() || frame.size() < 5) {
        return;
    }

    m_pending.frame = frame;
    m_pending.expected = expectedResponse;
    m_pending.delayMs = delayMs;
    m_pending.forceRead = forceRead;
    m_pending.generation = generation;
    m_hasPending = true;

    const qint64 arrivalElapsedMs = m_txClock.elapsed();
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

void CANWork::onHoldTick()
{
    if (!m_hasPending) {
        m_holdTimer->stop();
        return;
    }

    qint64 elapsedMs = m_txClock.elapsed();
    if (elapsedMs < m_holdTargetMs - 1) {
        return;
    }

    while (m_txClock.elapsed() < m_holdTargetMs) {
        // 最后 1ms 忙等，精准命中截止时刻
    }

    writePendingAtDeadline();
}

void CANWork::writePendingAtDeadline()
{
    if (!m_hasPending || !isOpen() || !m_channel) {
        return;
    }

    PendingFrame s = m_pending;
    m_hasPending = false;
    m_holdTimer->stop();

    m_expectedResponse = s.expected;
    m_currentGeneration = s.generation;

    m_channel->send(s.frame);
    const qint64 actualWriteElapsedMs = m_txClock.elapsed();

    const QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    emit sendLogLine(QStringLiteral("[%1] TX → %2").arg(timeStr, formatFrame(s.frame)));

    const qint64 intervalMs = (m_lastWriteElapsedMs >= 0)
            ? actualWriteElapsedMs - m_lastWriteElapsedMs
            : -1;
    const qint64 writeErrorMs = actualWriteElapsedMs - m_expectedWriteElapsedMs;
    m_lastWriteElapsedMs = actualWriteElapsedMs;
    m_prevTxAnchorElapsedMs = actualWriteElapsedMs;

    qDebug() << "[TxTiming][CAN]"
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

    const bool shouldRead = s.forceRead || !m_expectedResponse.isEmpty();
    if (!shouldRead) {
        emit responseReceived(QByteArray());
    }
}

void CANWork::cancelPendingCommands()
{
    m_hasPending = false;
    m_holdTimer->stop();
    m_expectedResponse.clear();
}

void CANWork::resetTimingCompensation()
{
    m_timingCompensationMs = 0;
    m_prevTxAnchorElapsedMs = -1;
    m_lastWriteElapsedMs = -1;
}

void CANWork::setExpectedResponse(const QByteArray &expected)
{
    m_expectedResponse = expected;
}

void CANWork::resetRecvCount()
{
    m_totalRecv = 0;
    emit recvCountChanged(0);
}

void CANWork::onChannelData(const QByteArray &data)
{
    const QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    emit recvLogLine(QStringLiteral("[%1] RX ← %2").arg(timeStr, formatFrame(data)));

    ++m_totalRecv;
    emit recvCountChanged(m_totalRecv);
    emit dataReceived(data);
    emit responseReceived(data);
}

void CANWork::onChannelStateChanged(bool connected)
{
    if (connected) {
        m_opened.storeRelaxed(1);
        m_prevTxAnchorElapsedMs = -1;
        m_lastWriteElapsedMs = -1;
        emit canOpened();
    } else {
        cancelPendingCommands();
        m_opened.storeRelaxed(0);
        emit canClosed();
    }
}

QString CANWork::formatFrame(const QByteArray &frame) const
{
    return CANFrameUtils::formatFrame(frame);
}
