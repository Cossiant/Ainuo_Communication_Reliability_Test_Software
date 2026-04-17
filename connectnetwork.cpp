#include "connectnetwork.h"
#include <QDebug>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent), m_tcpClient(nullptr), m_connectedStatus(false)
{
    contentListWidget = nullptr;
    sendListWidget = nullptr;
}

void NetworkClient::onNetworkConnected(int port, QHostAddress serverIP)
{
    m_tcpClient = new QTcpSocket(this);
    connect(m_tcpClient, &QTcpSocket::readyRead, this, &NetworkClient::onDataReceived);
    connect(m_tcpClient, &QTcpSocket::disconnected, this, &NetworkClient::onNetworkDisconnected);
    m_tcpClient->connectToHost(serverIP, port);
    qDebug() << "正在连接网络服务器...";
    if (m_tcpClient->waitForConnected(3000)) {
        m_connectedStatus = true;
        qDebug() << "网络连接成功！";
    } else {
        qDebug() << "网络连接失败：" << m_tcpClient->errorString();
    }
}

void NetworkClient::sendNetworkData(const QByteArray &data)
{
    if (data.isEmpty()) return;
    m_tcpClient->write(data);
    emit displaySentData(data);
}

void NetworkClient::onDataReceived()
{
    if (!m_tcpClient) return;
    QByteArray datagram = m_tcpClient->readAll();
    if (!datagram.isEmpty()) emit displayReceivedData(datagram);
}

void NetworkClient::onNetworkDisconnected()
{
    m_connectedStatus = false;
    qDebug() << "已断开网络连接！";
    if (m_tcpClient) {
        m_tcpClient->deleteLater();
        m_tcpClient = nullptr;
    }
}
