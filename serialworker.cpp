#include "serialworker.h"
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent), m_serialPort(nullptr),
      m_useBufferMode(false)   // 默认不缓冲
{
    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(20);   // 20ms 超时
    connect(m_flushTimer, &QTimer::timeout,this, &SerialWorker::onFlushBuffer);
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
    if (receivedData.isEmpty()) return;

    if (m_useBufferMode) {
        // 缓冲模式：追加数据并重启定时器
        m_buffer.append(receivedData);
        m_flushTimer->start();
    } else {
        // 原始模式：直接过滤并发射
        QByteArray filtered = receivedData;
        filtered.replace("\r\n", "");
        emit displayReceivedData(filtered);
    }
}

void SerialWorker::setBufferMode(bool enabled)
{
    m_useBufferMode = enabled;
    // 切换模式时清空残留缓冲
    if (!enabled) {
        m_buffer.clear();
        m_flushTimer->stop();
    }
}
// 添加 onFlushBuffer() 定时器槽
void SerialWorker::onFlushBuffer()
{
    if (m_buffer.isEmpty()) return;

    // 合并缓冲，移除换行（保持与原始模式一致的处理）
    QByteArray merged = m_buffer;
    merged.replace("\r\n", "");
    merged.replace("\n", "");   // 确保彻底去除

    emit displayReceivedData(merged);
    m_buffer.clear();
}
