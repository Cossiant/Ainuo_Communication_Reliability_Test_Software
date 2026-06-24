// NetworkWork.h
// 方案三：命令间隔延时移入工作线程，精确定时（TCP 客户端版）
// 网口无需合并缓冲，直接透传；支持可选禁用 Nagle 算法

#ifndef UNTITLED_NETWORKWORK_H
#define UNTITLED_NETWORKWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QAtomicInt>

class NetworkWork : public QObject
{
    Q_OBJECT

public:
    explicit NetworkWork(QObject *parent = nullptr);
    ~NetworkWork();

    // ─── 查询接口（线程安全） ───
    bool isOpen() const;
    int  totalRecvCount() const;
    QByteArray expectedResponse() const;

public slots:
    // ═══════════════════════════════════════════════════
    //  所有可能从主线程调用的写操作必须是 slot
    // ═══════════════════════════════════════════════════
    void connectToHost(const QString &ipAddress,
                       quint16 port,
                       bool disableNagle = false);
    void disconnectFromHost();
    void sendData(const QByteArray &data);
    void sendString(const QString &text, bool hexMode);

    // ★ 方案三：发送命令 + 在工作线程启动精确延时
    void sendStringWithDelay(const QString &text, bool hexMode,
                             const QByteArray &expectedResponse,
                             int delayMs);

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

    // ★ 工作线程内精确延时到期
    void interCmdDelayFinished();

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onInterCmdDelay();              // ★ 精确延时到期（工作线程内）

private:
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);

    QTcpSocket  *m_tcpSocket     = nullptr;
    QTimer      *m_interCmdTimer = nullptr;   // ★ 命令间隔定时器（工作线程）

    int        m_totalRecv = 0;
    QByteArray m_expectedResponse;
    bool       m_hexDisplay = false;

    QAtomicInt m_opened{0};

    // ★ 防止 disconnectFromHost 重入（同一线程内无需 atomic，但用 atomic 更安全）
    QAtomicInt m_disconnecting{0};
};

#endif // UNTITLED_NETWORKWORK_H
