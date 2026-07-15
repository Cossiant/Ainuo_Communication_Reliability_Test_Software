// SerialWork.h
// ★ 对齐 GPIBWork：sendStringWithDelay 添加 forceRead 参数

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
    // ═══════════════════════════════════════════════════
    //  所有可能从主线程调用的写操作必须是 slot
    // ═══════════════════════════════════════════════════
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
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs,
                             bool forceRead = false);

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);

signals:
    void serialOpened();
    void serialClosed();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);

    // ★ 工作线程内精确延时到期
    void interCmdDelayFinished();

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onBufferTimeout();
    void onInterCmdDelay();              // ★ 精确延时到期（工作线程内）

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    QSerialPort *m_serialPort   = nullptr;
    QTimer      *m_bufferTimer  = nullptr;
    QTimer      *m_interCmdTimer = nullptr;   // ★ 命令间隔定时器（工作线程，1ms轮询）

    QByteArray m_recvBuffer;
    bool       m_buffered = true;

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    QAtomicInt m_opened{0};

    // ═══════════════════════════════════════════════
    //  精确延时 + 误差补偿（对齐 GPIBWork / NetworkWork）
    // ═══════════════════════════════════════════════
    QElapsedTimer m_preciseDelayTimer;                // 高精度计时
    int           m_targetDelayMs        = 0;         // 本次补偿后目标（ms）
    int           m_originalDelayMs      = 0;         // 本次原始请求（ms，日志用）
    int           m_timingCompensationMs = 0;         // EMA 累积补偿（ms）
};

#endif // UNTITLED_SERIALWORK_H
