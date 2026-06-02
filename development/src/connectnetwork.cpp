//
// Created by Cossiant on 2026/6/2.
//

#include "../include/connectnetwork.h"
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

        // 根据标志禁用 Nagle 算法
        if (m_disableNagle) {
            m_tcpClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);
            qDebug() << "已禁用 Nagle 算法 (TCP_NODELAY)";
        }

        emit successConnectServer();
    } else {
        qDebug() << "网络连接失败：" << m_tcpClient->errorString();
        emit connectionError(m_tcpClient->errorString());
        // 清理未连接成功的套接字，避免残留
        m_tcpClient->deleteLater();
        m_tcpClient = nullptr;
        //随后执行网络线程关闭(这里逻辑与点击关闭按钮一致)
        emit disConnectServer();
    }
}

//发送软件数据到电源
void NetworkClient::sendNetworkData(const QByteArray &data)
{
    if (data.isEmpty()) return;
    m_tcpClient->write(data);
    emit displaySentData(data);
}

//收到网络发到软件的数据
void NetworkClient::onDataReceived()
{
    if (!m_tcpClient) return;
    QByteArray datagram = m_tcpClient->readAll();
    if (!datagram.isEmpty()) {
        // 移除所有的 "\r\n"
        QByteArray filteredData = datagram;
        filteredData.replace("\r\n", "");
        emit displayReceivedData(filteredData);
    }
}

//如果网络链接被电源断开
void NetworkClient::onNetworkDisconnected()
{
    m_connectedStatus = false;
    qDebug() << "已被电源断开网络连接！";
    if (m_tcpClient) {
        m_tcpClient->deleteLater();
        m_tcpClient = nullptr;
    }
    //随后执行网络线程关闭(这里逻辑与点击关闭按钮一致)
    //首先弹窗提示
    emit disConnectWithServer();
    //其次自动断开连接
    emit disConnectServer();
}
