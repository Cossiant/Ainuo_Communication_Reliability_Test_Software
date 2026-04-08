#ifndef CONNECTNETWORK_H
#define CONNECTNETWORK_H

#include <QTcpSocket>
#include <QObject>
#include <QHostAddress>
#include <QListWidget>
#include <QTime>

class connectNetwork:public QObject
{
    Q_OBJECT
public:
    connectNetwork(int port,QHostAddress serverIP);   
    //对外只需要输入port和serverip就可以了
    QListWidget *contentListWidge;
    QListWidget *sendListWidge;
    void NetworkSendData(QString msg);
private:
    QTcpSocket *tcpClient;
    bool status;
public slots:
    void NetworkConnectedSlot();
    void NetworkDisconnectedSlot();
    void NetworkDataReceivedSlot();
};

#endif // CONNECTNETWORK_H
