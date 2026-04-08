#ifndef CONNECTNETWORK_H
#define CONNECTNETWORK_H

#include <QTcpSocket>
#include <QObject>
#include <QHostAddress>
#include <QListWidget>
#include <QTime>
#include <QThread>

class connectNetwork:public QObject
{
    Q_OBJECT
public:
    connectNetwork();
    //对外只需要输入port和serverip就可以了
    QListWidget *contentListWidge;
    QListWidget *sendListWidge;
    void NetworkSendData(QString msg,int delayMS);
private:
    QTcpSocket *tcpClient;
    bool status;
signals:
    void DisplaConnectData(QString msg);                                          //接收到的数据显示信号量
    void DisplaSendData(QString msg);                                             //发送过去的数据显示信号量
public slots:
    void NetworkConnectedSlot(int port,QHostAddress serverIP);
    void NetworkDisconnectedSlot();
    void NetworkDataReceivedSlot();
};

#endif // CONNECTNETWORK_H
