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
    void NetworkSendData(QString msg);
    QListWidget *ListWidge;
private:
    QTcpSocket *tcpClient;
    bool status;
public slots:
    void NetworkConnectedSlot();
    void NetworkDisconnectedSlot();
    void NetworkDataReceivedSlot();
};

#endif // CONNECTNETWORK_H
