// SerialWork.h
// 方案三：命令间隔延时移入工作线程，精确定时

#ifndef UNTITLED_SERIALWORK_H
#define UNTITLED_SERIALWORK_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>
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

    // ★ 方案三：发送命令 + 在工作线程启动精确延时
    // expectedResponse: 期望返回值（传回主线程比对）
    // delayMs: 命令间隔（C 列），在工作线程用 PreciseTimer 计时
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs);

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

    // ★ 新增：工作线程内精确延时到期
    void interCmdDelayFinished();

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onBufferTimeout();
    void onInterCmdDelay();              // ★ 精确延时到期（工作线程内）

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    QSerialPort *m_serialPort  = nullptr;
    QTimer      *m_bufferTimer = nullptr;
    QTimer      *m_interCmdTimer = nullptr;   // ★ 命令间隔定时器（工作线程）

    QByteArray m_recvBuffer;
    bool       m_buffered = true;

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    QAtomicInt m_opened{0};
};

#endif // UNTITLED_SERIALWORK_H
