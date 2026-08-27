#pragma once

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>

namespace CommIO {
class CANChannel;
}

// CAN 工作对象：运行在独立线程，负责 ZLGCAN 打开/关闭、发送、接收，
// 以及与 SerialWork / NetworkWork 等模块对齐的精确延时调度。
class CANWork : public QObject
{
    Q_OBJECT

public:
    explicit CANWork(QObject *parent = nullptr);
    ~CANWork();

    bool isOpen() const;
    int  totalRecvCount() const;
    int  timingCompensationMs() const { return m_timingCompensationMs; }

public slots:
    void openCANPort(int deviceType,
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
                     int mode);
    void closeCANPort();

    // frame 使用 CANFrameUtils::encodeFrame() 的统一格式：4 字节大端 ID + 数据
    void sendFrameWithDelay(const QByteArray &frame,
                            const QByteArray &expectedResponse,
                            int delayMs,
                            bool forceRead,
                            int generation);

    void cancelPendingCommands();
    void resetTimingCompensation();
    void setExpectedResponse(const QByteArray &expected);
    void resetRecvCount();

signals:
    void canOpened();
    void canClosed();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &frame);
    void responseReceived(const QByteArray &frame);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);
    void commandWritten(int generation);

private slots:
    void onHoldTick();
    void onChannelData(const QByteArray &data);
    void onChannelStateChanged(bool connected);

private:
    void writePendingAtDeadline();
    QString formatFrame(const QByteArray &frame) const;

    CommIO::CANChannel *m_channel = nullptr;

    QByteArray m_expectedResponse;
    QAtomicInt m_opened{0};
    int m_totalRecv = 0;

    // 精确发送时间轴与绝对截止时刻
    QElapsedTimer m_txClock;
    qint64 m_txClockWallAnchorMs = 0;
    qint64 m_lastWriteElapsedMs = -1;
    qint64 m_expectedWriteElapsedMs = 0;
    qint64 m_prevTxAnchorElapsedMs = -1;

    struct PendingFrame {
        QByteArray frame;
        QByteArray expected;
        int        delayMs = 0;
        bool       forceRead = false;
        int        generation = 0;
    };
    PendingFrame m_pending;
    bool m_hasPending = false;
    qint64 m_holdTargetMs = 0;
    QTimer *m_holdTimer = nullptr;

    int m_timingCompensationMs = 0;
    int m_currentGeneration = 0;
};
