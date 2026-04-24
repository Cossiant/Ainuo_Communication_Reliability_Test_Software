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

    bool m_disableNagle = false;   // 是否禁用 Nagle 算法

private:
    QTcpSocket *m_tcpClient;
    bool m_connectedStatus;

signals:
    void displayReceivedData(const QByteArray &data);   // 接收到的数据（原始字节）
    void displaySentData(const QByteArray &data);       // 发送的数据（原始字节）
    void connectionError(const QString &errorMessage);  //网络连接失败发送提示
    void disConnectWithServer();                        //被电源断开网络连接
    void disConnectServer();                            //通知GUI线程结束网络连接（执行断开链接的按钮操作）
    void successConnectServer();                        //通知GUI网络连接成功

public slots:
    void onNetworkConnected(int port, QHostAddress serverIP);
    void onNetworkDisconnected();
    void onDataReceived();
};

#endif // CONNECTNETWORK_H
