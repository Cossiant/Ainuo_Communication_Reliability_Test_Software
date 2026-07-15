// SerialWork.cpp
// ★ 升级：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 新增：发送后缀功能

#include "SerialWork.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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

    // ★ 1ms 轮询定时器 — 配合 QElapsedTimer 实现高精度
    //    Qt5 在 Windows 上创建 QTimer 时会内部调用 timeBeginPeriod(1)，
    //    无需手动调用。
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);
    m_interCmdTimer->setInterval(1);          // 每1ms触发
    m_interCmdTimer->setSingleShot(false);     // 持续触发直到手动停止
    connect(m_interCmdTimer, &QTimer::timeout,
            this, &SerialWork::onInterCmdDelay);

    qDebug() << "SerialWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")"
             << "| 精确延时: 1ms轮询+忙等自旋+EMA补偿";
}

SerialWork::~SerialWork()
{
    m_interCmdTimer->stop();
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
// 设置发送后缀模式
void SerialWork::setSuffixMode(int mode)
{
    m_suffixMode = mode;
    qDebug() << "SerialWork: 后缀模式 =" << mode
             << (mode == 0 ? "None" : mode == 1 ? "CR" : mode == 2 ? "LF" : "CRLF");
}

// 统一构建发送数据（对齐 GPIBWork::buildSendData）
QByteArray SerialWork::buildSendData(const QString &text, bool hexMode) const
{
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        return QByteArray::fromHex(hex.toLatin1());
    }

    // Step 1: 将用户输入的 \r \n 转义还原为真实控制字符
    QString unescaped = text;
    unescaped.replace(QLatin1String("\\r"), QLatin1String("\r"));
    unescaped.replace(QLatin1String("\\n"), QLatin1String("\n"));

    QByteArray data = unescaped.toUtf8();

    // Step 2: 去掉末尾已有的 \r \n（避免与后缀重复）
    while (!data.isEmpty()) {
        char last = data.at(data.size() - 1);
        if (last == '\r' || last == '\n')
            data.chop(1);
        else
            break;
    }

    // Step 3: 追加用户选择的后缀
    //   0 = None, 1 = CR (\r), 2 = LF (\n), 3 = CRLF (\r\n)
    switch (m_suffixMode) {
        case 1:   data.append('\r');          break;   // CR
        case 2:   data.append('\n');          break;   // LF
        case 3:   data.append("\r\n");        break;   // CRLF
        case 0:                                break;   // None
        default:                               break;
    }

    return data;
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

    // ★ 新连接重置误差补偿
    m_timingCompensationMs = 0;

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
    if (!isOpen() || data.isEmpty() || !m_serialPort)
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

    QByteArray data = buildSendData(text, hexMode);   // ★ 使用统一构建方法

    sendData(data);
}
// ═══════════════════════════════════════════════════════════════
//  ★★★ 核心：发送 + 1ms轮询精确延时 + 微秒忙等 + EMA补偿 ★★★
//  ★ 对齐 GPIBWork：计时起点在 write 之前 + forceRead 控制读取
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendStringWithDelay(const QString &text, bool hexMode,
                                     const QByteArray &expectedResponse,
                                     int delayMs,
                                     bool forceRead)
{
    if (!isOpen() || text.isEmpty() || !m_serialPort)
        return;

    m_expectedResponse = expectedResponse;

    // ── 构建数据（★ 使用统一构建方法）──
    QByteArray data = buildSendData(text, hexMode);

    // ★ 对齐 GPIB：在 write 之前启动高精度计时
    //    delayMs > 0 时才启动，delayMs == 0 时跳过整个延时逻辑
    if (delayMs > 0) {
        m_originalDelayMs = delayMs;
        m_preciseDelayTimer.start();
    }

    // ── 发送 ──
    qint64 written = m_serialPort->write(data);
    if (written == -1) {
        emit errorOccurred(QString("发送失败: %1").arg(m_serialPort->errorString()));

        // ★ 发送失败但照样启动延时（保持与 GPIB 行为一致）
        if (delayMs > 0) {
            int compensatedMs = delayMs + m_timingCompensationMs;
            if (compensatedMs < 0) compensatedMs = 0;

            const int MAX_COMPENSATION = 100;
            m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                             m_timingCompensationMs,
                                             MAX_COMPENSATION);

            m_targetDelayMs   = compensatedMs;
            m_interCmdTimer->start();
        } else {
            emit interCmdDelayFinished();
        }
        return;
    }

    // ★ 等待数据刷新到串口驱动，消除发送侧的随机延迟
    if (m_serialPort->isOpen()) {
        m_serialPort->waitForBytesWritten(1);
    }

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    // ★ 对齐 GPIB：forceRead 控制是否需要等待设备回复
    bool shouldRead = forceRead || !m_expectedResponse.isEmpty();

    if (!shouldRead) {
        emit responseReceived(QByteArray());
    }

    // ═══════════════════════════════════════════════════════════
    //  精确延时（对齐 GPIB：计时起点在 write 之前）
    // ═══════════════════════════════════════════════════════════
    if (delayMs > 0) {
        qint64 alreadyElapsed = m_preciseDelayTimer.elapsed();

        if (alreadyElapsed >= delayMs) {
            emit interCmdDelayFinished();
        } else {
            int remainingMs = static_cast<int>(delayMs - alreadyElapsed);
            int compensatedMs = remainingMs + m_timingCompensationMs;
            if (compensatedMs < 0) compensatedMs = 0;

            const int MAX_COMPENSATION = 100;
            m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                             m_timingCompensationMs,
                                             MAX_COMPENSATION);

            m_targetDelayMs = static_cast<int>(alreadyElapsed + compensatedMs);
            m_interCmdTimer->start();
        }
    } else {
        emit interCmdDelayFinished();
    }
}
// ═══════════════════════════════════════════════════════════════
//  1ms 轮询回调：检测是否到期 → 微秒忙等 → 误差补偿 → 发射信号
// ═══════════════════════════════════════════════════════════════
void SerialWork::onInterCmdDelay()
{
    qint64 elapsedMs = m_preciseDelayTimer.elapsed();

    // ★ 还没到目标时间，继续等（定时器下次再触发）
    if (elapsedMs < m_targetDelayMs - 1) {
        return;
    }

    // ★ 距离目标 ≤1ms：进入忙等自旋，精准命中
    while (m_preciseDelayTimer.elapsed() < m_targetDelayMs) {
        // 自旋等待
    }

    // ★ 停止轮询
    m_interCmdTimer->stop();

    // ★ 测量实际耗时，计算误差
    qint64 actualMs = m_preciseDelayTimer.elapsed();
    int    errorMs  = static_cast<int>(actualMs - m_targetDelayMs);

    // ★ EMA 平滑更新补偿值 (alpha = 0.5)
    const int MAX_COMPENSATION = 100;
    m_timingCompensationMs -= errorMs / 2;
    m_timingCompensationMs  = qBound(-MAX_COMPENSATION,
                                      m_timingCompensationMs,
                                      MAX_COMPENSATION);

    // ★ 诊断日志
    if (qAbs(errorMs) >= 1) {
        qDebug() << "SerialWork:[精确延时]"
                 << "请求" << m_originalDelayMs << "ms"
                 << "→补偿后" << m_targetDelayMs << "ms"
                 << "→实际" << actualMs << "ms"
                 << "|误差" << errorMs << "ms"
                 << "|累积补偿" << m_timingCompensationMs << "ms";
    }

    // ★ 通知主线程
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
        if (!text.isEmpty()) {
            // ★ 将控制字符转义为可见字符串
            text.replace(QLatin1Char('\r'), QLatin1String("\\r"));
            text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
            return text;
        } else {
            return data.toHex(' ').toUpper();
        }
    }
}

