#ifndef CONNECTNETWORK_H
#define CONNECTNETWORK_H

#include <QTcpSocket>
#include <QObject>
#include <QHostAddress>
#include <QListWidget>
#include <QTime>
#include <QThread>

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    NetworkClient();
    //对外只需要输入port和serverip就可以了
    QListWidget *contentListWidget;
    QListWidget *sendListWidget;
    void sendNetworkData(QString msg, int delayMS);
private:
    QTcpSocket *m_tcpClient;
    bool m_connectedStatus;
signals:
    void displayReceivedData(QString msg);       //接收到的数据显示信号量
    void displaySentData(QString msg);           //发送过去的数据显示信号量
public slots:
    void onNetworkConnected(int port, QHostAddress serverIP);
    void onNetworkDisconnected();
    void onDataReceived();
};

#endif // CONNECTNETWORK_H
