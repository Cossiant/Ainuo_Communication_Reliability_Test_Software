// NetworkWork.h
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 对齐 GPIBWork：sendStringWithDelay 添加 forceRead 参数

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
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs,
                             bool forceRead = false);

    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    void setHexDisplayMode(bool hexMode);

signals:
    void networkConnected();
    void networkDisconnected();
    void errorOccurred(const QString &errorMessage);
    void dataReceived(const QByteArray &data);
    void responseReceived(const QByteArray &data);
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);
    void recvCountChanged(int totalCount);
    void interCmdDelayFinished();

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onInterCmdDelay();

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    QTcpSocket  *m_tcpSocket     = nullptr;

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    QAtomicInt m_opened{0};
    QAtomicInt m_disconnecting{0};

    // ═══════════════════════════════════════════════
    //  精确延时 + 误差补偿
    // ═══════════════════════════════════════════════
    QTimer       *m_interCmdTimer        = nullptr;   // 1ms 轮询定时器
    QElapsedTimer m_preciseDelayTimer;                // 高精度计时
    int           m_targetDelayMs         = 0;         // 本次补偿后目标（ms）
    int           m_originalDelayMs       = 0;         // 本次原始请求（ms，日志用）
    int           m_timingCompensationMs  = 0;         // EMA 累积补偿（ms）
};

#endif // UNTITLED_NETWORKWORK_H
