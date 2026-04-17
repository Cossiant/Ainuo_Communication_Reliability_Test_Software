#ifndef CONNECTNETWORK_H
#define CONNECTNETWORK_H

#include <QTcpSocket>
#include <QObject>
#include <QHostAddress>
#include <QListWidget>
#include <QByteArray>

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);

    // 对外接口
    QListWidget *contentListWidget;   // 接收显示列表（可选）
    QListWidget *sendListWidget;      // 发送显示列表（可选）

    void sendNetworkData(const QByteArray &data);

private:
    QTcpSocket *m_tcpClient;
    bool m_connectedStatus;

signals:
    void displayReceivedData(const QByteArray &data);   // 接收到的数据（原始字节）
    void displaySentData(const QByteArray &data);       // 发送的数据（原始字节）

public slots:
    void onNetworkConnected(int port, QHostAddress serverIP);
    void onNetworkDisconnected();
    void onDataReceived();
};

#endif // CONNECTNETWORK_H
