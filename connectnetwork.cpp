#include "connectnetwork.h"

NetworkClient::NetworkClient()
{
}

void NetworkClient::onNetworkConnected(int port, QHostAddress serverIP){
    m_tcpClient = new QTcpSocket(this);
    connect(m_tcpClient, SIGNAL(readyRead()), this, SLOT(onDataReceived()));
    connect(m_tcpClient, SIGNAL(disconnected()), this, SLOT(onNetworkDisconnected()));
    m_tcpClient->connectToHost(serverIP, port);
    qDebug() << "网络链接成功！";
}

void NetworkClient::sendNetworkData(QString msg, int delayMS){
    if(msg.isEmpty()) return;
    m_tcpClient->write(msg.toLatin1(), msg.length());
    emit displaySentData("[" + QTime::currentTime().toString("hh:mm:ss.zzz") + "] " + msg);
}

void NetworkClient::onDataReceived(){
    while(m_tcpClient->bytesAvailable() > 0){
        QByteArray datagram;
        datagram.resize(m_tcpClient->bytesAvailable());
        m_tcpClient->read(datagram.data(), datagram.size());
        QString msg = datagram.data();
        emit displayReceivedData("[" + QTime::currentTime().toString("hh:mm:ss.zzz") + "] " + msg.left(datagram.size()));
    }
}

void NetworkClient::onNetworkDisconnected(){
    qDebug() << "已断开网络连接！";
}
