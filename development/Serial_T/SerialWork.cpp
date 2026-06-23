// SerialWork.cpp
// 方案三实现

#include "SerialWork.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════
SerialWork::SerialWork(QObject *parent)
    : QObject(parent)
{
    // 缓冲区合并定时器
    m_bufferTimer = new QTimer(this);
    m_bufferTimer->setSingleShot(true);
    connect(m_bufferTimer, &QTimer::timeout,
            this, &SerialWork::onBufferTimeout);

    // ★ 命令间隔精确延时定时器（工作线程内）
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setSingleShot(true);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);   // ★ 1ms 精度
    connect(m_interCmdTimer, &QTimer::timeout,
            this, &SerialWork::onInterCmdDelay);

    qDebug() << "SerialWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")";
}

SerialWork::~SerialWork()
{
    closeSerialPort();
    qDebug() << "SerialWork: 已销毁";
}

// ═══════════════════════════════════════════════════════════════
//  查询 / 设置 接口
// ═══════════════════════════════════════════════════════════════
bool SerialWork::isOpen() const
{
    return m_opened.loadRelaxed() != 0;
}

int SerialWork::totalRecvCount() const
{
    return m_totalRecv;
}

void SerialWork::resetRecvCount()
{
    m_totalRecv = 0;
    emit recvCountChanged(0);
}

void SerialWork::setExpectedResponse(const QByteArray &expected)
{
    m_expectedResponse = expected;
}

QByteArray SerialWork::expectedResponse() const
{
    return m_expectedResponse;
}

void SerialWork::setHexDisplayMode(bool hexMode)
{
    m_hexDisplay = hexMode;
}

// ═══════════════════════════════════════════════════════════════
//  打开串口
// ═══════════════════════════════════════════════════════════════
void SerialWork::openSerialPort(const QString &portName,
                                int baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits,
                                bool buffered)
{
    if (m_serialPort) {
        closeSerialPort();
    }

    m_buffered = buffered;

    m_serialPort = new QSerialPort(this);

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString err = m_serialPort->errorString();
        emit errorOccurred(QString("无法打开 %1: %2").arg(portName, err));

        delete m_serialPort;
        m_serialPort = nullptr;
        return;
    }

    connect(m_serialPort, &QSerialPort::readyRead,
            this, &SerialWork::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &SerialWork::onSerialError);

    m_opened.storeRelaxed(1);
    emit serialOpened();

    qDebug() << "SerialWork: 串口已打开" << portName << baudRate
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  关闭串口
// ═══════════════════════════════════════════════════════════════
void SerialWork::closeSerialPort()
{
    m_interCmdTimer->stop();   // ★ 停止命令间隔定时器

    if (m_serialPort) {
        disconnect(m_serialPort, nullptr, this, nullptr);

        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    m_bufferTimer->stop();
    m_recvBuffer.clear();

    m_opened.storeRelaxed(0);
    emit serialClosed();

    qDebug() << "SerialWork: 串口已关闭"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  发送原始字节
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendData(const QByteArray &data)
{
    if (!isOpen() || data.isEmpty())
        return;

    qint64 written = m_serialPort->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_serialPort->errorString()));
        return;
    }
    if (written < data.size()) {
        qDebug() << "SerialWork: 部分发送" << written << "/" << data.size();
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));
}

// ═══════════════════════════════════════════════════════════════
//  发送字符串
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendString(const QString &text, bool hexMode)
{
    if (!isOpen() || text.isEmpty())
        return;

    QByteArray data;
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
    } else {
        data = text.toUtf8();
    }

    sendData(data);
}

// ═══════════════════════════════════════════════════════════════
//  ★ 方案三核心：发送命令 + 工作线程内精确延时
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendStringWithDelay(const QString &text, bool hexMode,
                                     const QByteArray &expectedResponse,
                                     int delayMs)
{
    if (!isOpen() || text.isEmpty())
        return;

    // 1. 设置期望返回值
    m_expectedResponse = expectedResponse;

    // 2. 编码并发送
    QByteArray data;
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
    } else {
        data = text.toUtf8();
    }

    qint64 written = m_serialPort->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_serialPort->errorString()));
        return;
    }

    // 3. 发送日志
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    // 4. ★ 在工作线程内启动精确延时（消除主线程定时器抖动 + 跨线程排队）
    if (delayMs > 0) {
        m_interCmdTimer->start(delayMs);
    } else {
        // 无延时，立刻通知
        emit interCmdDelayFinished();
    }
}

// ═══════════════════════════════════════════════════════════════
//  ★ 精确延时到期（工作线程内触发）
// ═══════════════════════════════════════════════════════════════
void SerialWork::onInterCmdDelay()
{
    // 从工作线程发射信号 → 主线程通过 QueuedConnection 接收
    emit interCmdDelayFinished();
}

// ═══════════════════════════════════════════════════════════════
//  处理 readyRead
// ═══════════════════════════════════════════════════════════════
void SerialWork::onReadyRead()
{
    if (!m_serialPort)
        return;

    QByteArray chunk = m_serialPort->readAll();

    if (m_buffered) {
        m_recvBuffer.append(chunk);
        m_bufferTimer->start(20);
    } else {
        emitData(chunk);
    }
}

// ═══════════════════════════════════════════════════════════════
//  缓冲区超时
// ═══════════════════════════════════════════════════════════════
void SerialWork::onBufferTimeout()
{
    if (!m_recvBuffer.isEmpty()) {
        emitData(m_recvBuffer);
        m_recvBuffer.clear();
    }
}

// ═══════════════════════════════════════════════════════════════
//  处理串口错误
// ═══════════════════════════════════════════════════════════════
void SerialWork::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    QString msg;
    switch (error) {
    case QSerialPort::ResourceError:
        msg = "串口设备被移除或断开";
        closeSerialPort();
        break;
    case QSerialPort::TimeoutError:
        msg = "串口操作超时";
        break;
    case QSerialPort::ReadError:
        msg = "串口读取错误";
        break;
    case QSerialPort::WriteError:
        msg = "串口写入错误";
        break;
    default:
        msg = m_serialPort ? m_serialPort->errorString()
                           : "未知串口错误";
        break;
    }

    emit errorOccurred(msg);
}

// ═══════════════════════════════════════════════════════════════
//  内部：统一的数据输出入口
// ═══════════════════════════════════════════════════════════════
void SerialWork::emitData(const QByteArray &data)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit recvLogLine(QString("[%1] RX ← %2").arg(timeStr, display));

    m_totalRecv++;
    emit recvCountChanged(m_totalRecv);

    emit dataReceived(data);
    emit responseReceived(data);
}

// ═══════════════════════════════════════════════════════════════
//  内部：格式化字节数组
// ═══════════════════════════════════════════════════════════════
QString SerialWork::formatByteArray(const QByteArray &data) const
{
    if (m_hexDisplay) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty())
            return text;
        else
            return data.toHex(' ').toUpper();
    }
}
