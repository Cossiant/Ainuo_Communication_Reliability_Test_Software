// GPIBWork.h
// GPIB Worker：在独立线程中执行阻塞 VISA 操作
// 对齐 SerialWork / NetworkWork 架构
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 新增：代际标记防止信号串扰

#ifndef UNTITLED_GPIBWORK_H
#define UNTITLED_GPIBWORK_H

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>

// NI-VISA 类型前向声明（避免在头文件中全局包含 visa.h）
typedef unsigned long ViSession;
typedef long          ViStatus;

// 发送后缀模式
enum class GPIBSuffix {
    None = 0,
    CR   = 1,   // 追加 \r
    LF   = 2,   // 追加 \n
    CRLF = 3    // 追加 \r\n
};

class GPIBWork : public QObject
{
    Q_OBJECT

public:
    explicit GPIBWork(QObject *parent = nullptr);
    ~GPIBWork();

    // ─── 查询接口（线程安全） ───
    bool isOpen() const;
    int  totalRecvCount() const;
    QByteArray expectedResponse() const;
    int  timingCompensationMs() const { return m_timingCompensationMs; }

public slots:
    // ═══════════════════════════════════════════════════
    //  所有可能从主线程调用的写操作必须是 slot
    // ═══════════════════════════════════════════════════
    void openGPIBPort(int boardIndex,
                      int primaryAddress,
                      int secondaryAddress,
                      int timeoutMs,
                      bool termCharEnabled,
                      char termChar,
                      bool sendEndEnabled);
    void closeGPIBPort();
    void sendString(const QString &text, bool hexMode);

    // ★ 发送命令 + 在工作线程启动精确延时（1ms轮询+忙等+EMA补偿）
    // forceRead: true=必须viRead（捕获模式），false=仅在expectedResponse非空时读取
    // generation: 代际标记，用于防止旧延迟信号污染新命令
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs,
                             bool forceRead,
                             int responseTimeoutMs,
                             int generation = 0);      // ★ 新增参数

    // ★ 阶段二：取消尚未写入的预投递命令（停止发送/关闭设备时调用）
    void cancelPendingCommands();

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);

    void setSuffixMode(int mode);   // 设置发送后缀模式
    void resetTimingCompensation(); // ★ 新增：重置误差补偿

signals:
    void gpibOpened();
    void gpibClosed();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);

    // ★ 工作线程内精确延时到期，携带代际标记
    void interCmdDelayFinished(int generation);       // ★ 修改：携带代际
    void commandWritten(int generation);              // ★ 阶段二：实际 viWrite 完成时刻

private slots:
    void onHoldTick();                   // ★ 预投递命令按绝对截止时刻触发写入

private:
    // ─── VISA 操作 ───
    bool doVISAWrite(const QByteArray &data);
    QByteArray doVISARead(int timeoutMs);
    bool checkVISAStatus(ViStatus status, const QString &operation);
    QString visaStatusHex(ViStatus status);
    QString viOpenFailureReason(ViStatus status);

    // ─── 格式化 ───
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    // ─── VISA 句柄 ───
    ViSession m_resourceManager = 0;
    ViSession m_instrument      = 0;

    // ─── GPIB 配置 ───
    int  m_timeoutMs       = 3000;
    char m_termChar        = '\n';
    bool m_termCharEnabled = true;
    bool m_sendEndEnabled  = true;

    // ─── 计数与状态 ───
    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    GPIBSuffix m_suffixMode = GPIBSuffix::None;   // 发送后缀
    QByteArray buildSendData(const QString &text, bool hexMode) const;  // 构建发送数据

    QAtomicInt m_opened{0};

    QTimer       *m_holdTimer            = nullptr;   // 预投递命令的截止等待定时器
    int           m_timingCompensationMs  = 0;         // 保留接口：绝对截止模型下通常为 0

    // ★ 发送时间戳日志（测量期望发送 / 实际 write / 两次 write 间隔）
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
        int        readTimeoutMs = 0;
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

#endif // UNTITLED_GPIBWORK_H
