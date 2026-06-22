#include "SerialThread.h"
#include <QDebug>

#include "SerialWork.h"

SerialThread::SerialThread(QObject *parent)
    : QObject(parent)
{
}

SerialThread::~SerialThread()
{
    if (m_serial) {
        if (m_serial->isOpen()) {
            m_serial->close();
        }
        delete m_serial;
        m_serial = nullptr;
    }
}

// ═══════════════════════════════════════════════════════════════
//  打开串口（在工作线程中执行）
// ═══════════════════════════════════════════════════════════════
void SerialThread::onSerialStart(QString portName,
                                 qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits,
                                 QSerialPort::FlowControl flowControl)
{
    // ──── 如果已存在，先清理 ────
    if (m_serial) {
        if (m_serial->isOpen()) m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }

    m_serial = new QSerialPort(this);
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(dataBits);
    m_serial->setParity(parity);
    m_serial->setStopBits(stopBits);
    m_serial->setFlowControl(flowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QString err = QString("无法打开串口 %1: %2")
                          .arg(portName, m_serial->errorString());
        emit errorOccurred(err);
        emit serialClosed();          // 打开失败也要通知上层清理
        return;
    }

    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialThread::onReadyRead);

    emit serialOpened();

    qDebug() << "SerialThread: 串口已打开" << portName << baudRate;
}

// ═══════════════════════════════════════════════════════════════
//  关闭串口（在工作线程中执行）
// ═══════════════════════════════════════════════════════════════
void SerialThread::onSerialStop()
{
    qDebug() << "SerialThread: 关闭串口";

    if (m_serial) {
        if (m_serial->isOpen()) {
            m_serial->close();
        }
        m_serial->deleteLater();
        m_serial = nullptr;
    }

    m_buffer.clear();
    emit serialClosed();
}

// ═══════════════════════════════════════════════════════════════
//  缓冲区模式开关
// ═══════════════════════════════════════════════════════════════
void SerialThread::setBufferMode(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_bufferMode = enabled;
    if (!enabled) {
        m_buffer.clear();
    }
}

// ═══════════════════════════════════════════════════════════════
//  发送数据（在工作线程中执行）
// ═══════════════════════════════════════════════════════════════
void SerialThread::sendData(const QByteArray &data)
{
    if (!m_serial || !m_serial->isOpen()) {
        emit errorOccurred("串口未打开，无法发送数据");
        return;
    }
    m_serial->write(data);
}

// ═══════════════════════════════════════════════════════════════
//  接收数据
// ═══════════════════════════════════════════════════════════════
void SerialThread::onReadyRead()
{
    if (!m_serial) return;

    QByteArray data = m_serial->readAll();

    QMutexLocker locker(&m_mutex);
    if (m_bufferMode) {
        m_buffer.append(data);
        // 缓冲区模式：不发射信号，等待外部批量读取
    } else {
        locker.unlock();
        emit dataReceived(data);
    }
}
