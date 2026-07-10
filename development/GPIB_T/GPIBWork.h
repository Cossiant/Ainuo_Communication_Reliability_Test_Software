// GPIBWork.h
// GPIB Worker：在独立线程中执行阻塞 VISA 操作
// 对齐 SerialWork / NetworkWork 架构
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿

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
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs,
                             bool forceRead);

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);

signals:
    void gpibOpened();
    void gpibClosed();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);

    // ★ 工作线程内精确延时到期
    void interCmdDelayFinished();

private slots:
    void onInterCmdDelay();              // ★ 精确延时到期（工作线程内）

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

    QAtomicInt m_opened{0};

    // ═══════════════════════════════════════════════
    //  精确延时 + 误差补偿（对齐 SerialWork / NetworkWork）
    // ═══════════════════════════════════════════════
    QTimer       *m_interCmdTimer        = nullptr;   // 1ms 轮询定时器
    QElapsedTimer m_preciseDelayTimer;                // 高精度计时
    int           m_targetDelayMs         = 0;         // 本次补偿后目标（ms）
    int           m_originalDelayMs       = 0;         // 本次原始请求（ms，日志用）
    int           m_timingCompensationMs  = 0;         // EMA 累积补偿（ms）
};

#endif // UNTITLED_GPIBWORK_H
