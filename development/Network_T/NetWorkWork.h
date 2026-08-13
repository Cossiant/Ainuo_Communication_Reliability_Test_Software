// NetWorkWork.h
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 对齐 GPIBWork：sendStringWithDelay 添加 forceRead 参数
// ★ 新增：发送后缀功能
// ★ 新增：代际标记防止信号串扰

#ifndef UNTITLED_NETWORKWORK_H
#define UNTITLED_NETWORKWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>

class NetworkWork : public QObject
{
    Q_OBJECT

public:
    explicit NetworkWork(QObject *parent = nullptr);
    ~NetworkWork();

    bool isOpen() const;
    int  totalRecvCount() const;
    QByteArray expectedResponse() const;
    int  timingCompensationMs() const { return m_timingCompensationMs; }

public slots:
    void connectToHost(const QString &ipAddress,
                       quint16 port,
                       bool disableNagle = false);
    void disconnectFromHost();
    void sendData(const QByteArray &data);
    void sendString(const QString &text, bool hexMode);

    // ★ forceRead: true=必须等待设备回复（捕获模式），
    //              false=仅在 expectedResponse 非空时等待回复
    // ★ generation: 代际标记，用于防止旧延迟信号污染新命令
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs,
                             bool forceRead = false,
                             int generation = 0);      // ★ 新增参数

    // 粘包分包分支复用的“仅延时”入口：由 worker 线程执行 1ms 轮询 + 忙等精确延时
    void startDelayOnly(int delayMs, int generation);

    // ★ 阶段二：取消尚未写入的预投递命令（停止发送/断开连接时调用）
    void cancelPendingCommands();

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);
    void setSuffixMode(int mode);
    void resetTimingCompensation(); // ★ 新增：重置误差补偿

signals:
    void networkConnected();
    void networkDisconnected();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);
    void interCmdDelayFinished(int generation);       // ★ 修改：携带代际
    void commandWritten(int generation);              // ★ 阶段二：实际 write 时刻

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onInterCmdDelay();
    void onHoldTick();                                // ★ 阶段二：截止时刻触发写入

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);
    QByteArray buildSendData(const QString &text, bool hexMode) const;

    QTcpSocket  *m_tcpSocket     = nullptr;

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;
    int        m_suffixMode = 0;

    QAtomicInt m_opened{0};
    QAtomicInt m_disconnecting{0};

    QTimer       *m_interCmdTimer        = nullptr;
    QTimer       *m_holdTimer            = nullptr;   // ★ 预投递命令的截止等待定时器
    QElapsedTimer m_preciseDelayTimer;
    int           m_targetDelayMs         = 0;
    int           m_originalDelayMs       = 0;
    int           m_timingCompensationMs  = 0;

    // ★ 发送时间戳日志（阶段一：测量期望发送 / 实际 write / 两次 write 间隔）
    QElapsedTimer m_txClock;
    qint64        m_txClockWallAnchorMs    = 0;
    qint64        m_lastWriteElapsedMs     = -1;
    qint64        m_expectedWriteElapsedMs = 0;

    // ★ 阶段二：预投递命令 + 绝对发送锚点
    struct PendingSend {
        QString    text;
        bool       hexMode   = false;
        QByteArray expected;
        int        delayMs   = 0;
        bool       forceRead = false;
        int        generation = 0;
    };
    PendingSend m_pendingSend;
    bool        m_hasPendingSend        = false;
    qint64      m_holdTargetMs          = 0;
    qint64      m_prevTxAnchorElapsedMs = -1;

    // ★ 代际标记：防止旧延迟信号污染新命令
    int           m_currentGeneration    = 0;

    void writePendingAtDeadline();
};

#endif // UNTITLED_NETWORKWORK_H
