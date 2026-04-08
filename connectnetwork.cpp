#include "connectnetwork.h"

connectNetwork::connectNetwork()
{
}

void connectNetwork::NetworkConnectedSlot(int port,QHostAddress serverIP){
    tcpClient = new QTcpSocket(this);
    connect(tcpClient,SIGNAL(readyRead()),this,SLOT(NetworkDataReceivedSlot()));
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(NetworkDisconnectedSlot()));
    tcpClient->connectToHost(serverIP,port);
    qDebug() << "网络链接成功！";
}

void connectNetwork::NetworkSendData(QString msg,int delayMS){
    if(msg.isEmpty()) return;
    tcpClient->write(msg.toLatin1(),msg.length());
    DisplaSendData("["+QTime::currentTime().toString("hh:mm:ss.zzz")+"] "+msg);
    if(delayMS > 0) {
        QThread::msleep(delayMS);  // 包含头文件 <QThread>
    }
}

void connectNetwork::NetworkDataReceivedSlot(){
    while(tcpClient->bytesAvailable()>0){
        QByteArray dataprogram;
        dataprogram.resize(tcpClient->bytesAvailable());
        tcpClient->read(dataprogram.data(),dataprogram.size());
        QString msg = dataprogram.data();
        DisplaConnectData("["+QTime::currentTime().toString("hh:mm:ss.zzz")+"] "+msg.left(dataprogram.size()));
    }
}

void connectNetwork::NetworkDisconnectedSlot(){
    qDebug() << "已断开网络连接！";
}
