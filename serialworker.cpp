#include "serialworker.h"
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent), m_serialPort(nullptr)
{
}

void SerialWorker::onSerialStart(const QString &portName, qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits,
                                 QSerialPort::FlowControl flowControl)
{
    qDebug() << "开始初始化串口";
    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(flowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "串口打开成功！";
        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
    } else {
        qDebug() << "无法打开串口：" << m_serialPort->errorString();
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
    }
}

void SerialWorker::onSerialStop()
{
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
            qDebug() << "串口已关闭";
        }
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
    }
    emit serialClosed();
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->write(data);
        emit displaySentData(data);
    } else {
        qWarning() << "串口未打开，无法发送数据";
    }
}

void SerialWorker::handleReadyRead()
{
    if (!m_serialPort) return;
    QByteArray receivedData = m_serialPort->readAll();
    if (!receivedData.isEmpty()) {
        emit displayReceivedData(receivedData);
    }
}
