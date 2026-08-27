// SerialWork.h
// ★ sendStringWithDelay 添加 forceRead 参数
// ★ 新增：发送后缀功能
// ★ 新增：可配置缓冲区超时时间
// ★ 新增：代际标记防止信号串扰

#ifndef UNTITLED_SERIALWORK_H
#define UNTITLED_SERIALWORK_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>

class SerialWork : public QObject
{
    Q_OBJECT

public:
    explicit SerialWork(QObject *parent = nullptr);
    ~SerialWork();

    // ─── 查询接口（线程安全） ───
    bool isOpen() const;
    int  totalRecvCount() const;
    QByteArray expectedResponse() const;
    int  timingCompensationMs() const { return m_timingCompensationMs; }

public slots:
    void openSerialPort(const QString &portName,
                        int baudRate,
                        QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity,
                        QSerialPort::StopBits stopBits,
                        bool buffered = true);
    void closeSerialPort();
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

    // ★ 阶段二：取消尚未写入的预投递命令（停止发送/关串口时调用）
    void cancelPendingCommands();

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);
    void setSuffixMode(int mode);   // ★ 新增：设置发送后缀模式
    void setBufferTimeout(int ms);  // ★ 新增：设置缓冲区合并超时时间
    void resetTimingCompensation(); // ★ 新增：重置误差补偿

signals:
    void serialOpened();
    void serialClosed();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);

    // ★ 工作线程内精确延时到期，携带代际标记
    void interCmdDelayFinished(int generation);    // ★ 修改：携带代际

    // ★ 阶段二：命令在 worker 线程真正写入串口的时刻（GUI 据此启动响应超时）
    void commandWritten(int generation);

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onBufferTimeout();
    void onInterCmdDelay();              // ★ 精确延时到期（工作线程内）
    void onHoldTick();                   // ★ 预投递命令按绝对截止时刻触发写入

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    // ★ 新增：统一构建发送数据（unescape → 去尾 → 加后缀）
    QByteArray buildSendData(const QString &text, bool hexMode) const;

    QSerialPort *m_serialPort   = nullptr;
    QTimer      *m_bufferTimer  = nullptr;
    QTimer      *m_interCmdTimer = nullptr;   // ★ 命令间隔定时器（工作线程，1ms轮询）
    QTimer      *m_holdTimer    = nullptr;    // ★ 预投递命令的截止等待定时器

    QByteArray m_recvBuffer;
    bool       m_buffered = true;

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    int        m_suffixMode = 0;   // ★ 新增：发送后缀模式 0=None, 1=CR, 2=LF, 3=CRLF

    int        m_bufferTimeoutMs = 20;  // ★ 新增：缓冲区合并超时（默认20ms）

    QAtomicInt m_opened{0};

    // ═══════════════════════════════════════════════
    //  精确延时 + 误差补偿（对齐 NetworkWork 等模块）
    // ═══════════════════════════════════════════════
    QElapsedTimer m_preciseDelayTimer;                // 高精度计时
    int           m_targetDelayMs        = 0;         // 本次补偿后目标（ms）
    int           m_originalDelayMs      = 0;         // 本次原始请求（ms，日志用）
    int           m_timingCompensationMs = 0;         // EMA 累积补偿（ms）

    // ★ 发送时间戳日志（阶段一：测量期望发送 / 实际 write / 两次 write 间隔）
    QElapsedTimer m_txClock;                            // 高精度发送时间轴
    qint64        m_txClockWallAnchorMs    = 0;         // 时间轴零点对应的墙钟时刻（ms）
    qint64        m_lastWriteElapsedMs     = -1;        // 上一条实际 write 时刻（-1 表示无）
    qint64        m_expectedWriteElapsedMs = 0;         // 本条指令的期望 write 时刻

    // ★ 阶段二：预投递命令 + 绝对发送锚点（消除 GUI↔worker 的截止时刻往返）
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
    qint64      m_prevTxAnchorElapsedMs = -1;        // 上一条 write / 粘包伪 write 的时刻

    // ★ 代际标记：防止旧延迟信号污染新命令
    int           m_currentGeneration    = 0;         // 当前正在处理的命令代际

    void writePendingAtDeadline();                    // 到达截止时刻后执行实际 write
};

#endif // UNTITLED_SERIALWORK_H
