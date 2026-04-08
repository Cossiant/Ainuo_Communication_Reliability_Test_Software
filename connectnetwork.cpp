#include "connectnetwork.h"

connectNetwork::connectNetwork(int port,QHostAddress serverIP)
{
    status = false;
    tcpClient = new QTcpSocket(this);
    connect(tcpClient,SIGNAL(readyRead()),this,SLOT(NetworkDataReceivedSlot()));
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(NetworkDisconnectedSlot()));
    connect(tcpClient,SIGNAL(connected()),this,SLOT(NetworkConnectedSlot()));
    tcpClient->connectToHost(serverIP,port);
    qDebug() << "初始化Network完成！" ;
    status = true;
}

void connectNetwork::NetworkConnectedSlot(){
    qDebug() << "网络链接成功！";
}

void connectNetwork::NetworkSendData(QString msg){
    if(msg.length()==0){
        return;
    }
    tcpClient->write(msg.toLatin1(),msg.length());
    contentListWidge->addItem("["+QTime::currentTime().toString("hh:mm:ss.zzz")+"] "+msg);
}

void connectNetwork::NetworkDataReceivedSlot(){
    while(tcpClient->bytesAvailable()>0){
        QByteArray dataprogram;
        dataprogram.resize(tcpClient->bytesAvailable());
        tcpClient->read(dataprogram.data(),dataprogram.size());
        QString msg = dataprogram.data();
        sendListWidge->addItem("["+QTime::currentTime().toString("hh:mm:ss.zzz")+"] "+msg.left(dataprogram.size()));
    }
}

void connectNetwork::NetworkDisconnectedSlot(){
    qDebug() << "已断开网络连接！";
}
