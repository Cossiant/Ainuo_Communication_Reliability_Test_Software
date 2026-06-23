//
// Created by Cossiant on 2026/6/22.
//

#include "SerialWork.h"
#include "SerialPage.h"
#include "ElaWindow.h"
#include "../Other_T/LED.h"

#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

static const int MAX_LOG_LINES = 200;

SerialWork::SerialWork(SerialPage *serialPage, QObject *parent)
    : QObject(parent),
      m_serialPage(serialPage),
      m_mainWindow(serialPage->m_mainWindow)
{
    connect(m_serialPage->m_openSerialButton,  &ElaPushButton::clicked,
            this, &SerialWork::onOpenSerial);
    connect(m_serialPage->m_closeSerialButton, &ElaPushButton::clicked,
            this, &SerialWork::onCloseSerial);

    connect(m_serialPage->m_singleSendBtn, &ElaPushButton::clicked, [this]() {
        QString text = m_serialPage->m_singleSendInput->text();
        this->sendString(text);
    });

    m_bufferTimer = new QTimer(this);
    m_bufferTimer->setSingleShot(true);
    connect(m_bufferTimer, &QTimer::timeout, this, &SerialWork::onBufferTimeout);

    qDebug() << "串口初始化完成！";
}

SerialWork::~SerialWork()
{
    if (m_serialPort) {
        disconnect(m_serialPort, &QSerialPort::readyRead,
                   this, &SerialWork::onReadyRead);
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }
}

bool SerialWork::isOpen() const
{
    return m_serialPort && m_serialPort->isOpen();
}

void SerialWork::setExpectedResponse(const QByteArray &expected)
{
    m_expectedResponse = expected;
}

// ═══════════════════════════════════════════════════════════════
//  发送字符串（自动判断 HEX / ASCII）
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendString(const QString &text)
{
    if (!isOpen() || text.isEmpty())
        return;

    QByteArray data;
    QString displayText;

    if (m_serialPage->m_serialHexSendCheckBox->isChecked()) {
        QString hex = text;
        hex.remove(' ');
        data = QByteArray::fromHex(hex.toLatin1());
        displayText = data.toHex(' ').toUpper();
    } else {
        data = text.toUtf8();
        displayText = text;
    }

    sendBytes(data, displayText);
}

// ═══════════════════════════════════════════════════════════════
//  发送原始字节
// ═══════════════════════════════════════════════════════════════
void SerialWork::sendBytes(const QByteArray &data, const QString &displayText)
{
    if (!isOpen() || data.isEmpty())
        return;

    qint64 written = m_serialPort->write(data);
    if (written == -1) {
        qDebug() << "SerialWork: 发送失败:" << m_serialPort->errorString();
        return;
    }
    if (written < data.size()) {
        qDebug() << "SerialWork: 部分发送" << written << "/" << data.size();
    }

    QString text = displayText.isEmpty()
                       ? data.toHex(' ').toUpper()
                       : displayText;
    logSend(text);
}

// ═══════════════════════════════════════════════════════════════
//  打开串口
// ═══════════════════════════════════════════════════════════════
void SerialWork::onOpenSerial()
{
    qDebug() << "打开串口按钮被点击！";

    QString portName = m_serialPage->m_serialPortComboBox->currentText();
    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(m_mainWindow, "警告", "请选择有效的串口端口！");
        return;
    }

    qint32 baudRate = m_serialPage->m_baudRateComboBox->currentText().toInt();

    QSerialPort::DataBits dataBits;
    QString db = m_serialPage->m_dataBitsComboBox->currentText();
    if (db == "5")      dataBits = QSerialPort::Data5;
    else if (db == "6") dataBits = QSerialPort::Data6;
    else if (db == "7") dataBits = QSerialPort::Data7;
    else                dataBits = QSerialPort::Data8;

    QSerialPort::StopBits stopBits;
    QString sb = m_serialPage->m_stopBitsComboBox->currentText();
    if (sb == "1.5") stopBits = QSerialPort::OneAndHalfStop;
    else if (sb == "2") stopBits = QSerialPort::TwoStop;
    else                stopBits = QSerialPort::OneStop;

    QSerialPort::Parity parity;
    QString pa = m_serialPage->m_parityComboBox->currentText();
    if (pa == "Even")      parity = QSerialPort::EvenParity;
    else if (pa == "Odd")   parity = QSerialPort::OddParity;
    else if (pa == "Space") parity = QSerialPort::SpaceParity;
    else if (pa == "Mark")  parity = QSerialPort::MarkParity;
    else                    parity = QSerialPort::NoParity;

    QSerialPort::FlowControl flow = QSerialPort::NoFlowControl;

    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setParity(parity);
    m_serialPort->setFlowControl(flow);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(m_mainWindow, "错误",
                              "无法打开串口 " + portName + ":\n" + m_serialPort->errorString());
        delete m_serialPort;
        m_serialPort = nullptr;
        return;
    }

    connect(m_serialPort, &QSerialPort::readyRead,
            this, &SerialWork::onReadyRead);

    updateUIForOpened(true);
    qDebug() << "SerialWork: 串口已打开" << portName << baudRate;
}

// ═══════════════════════════════════════════════════════════════
//  关闭串口
// ═══════════════════════════════════════════════════════════════
void SerialWork::onCloseSerial()
{
    if (m_serialPort) {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }
    updateUIForOpened(false);
    qDebug() << "SerialWork: 串口已关闭";
}

// ═══════════════════════════════════════════════════════════════
//  接收数据
// ═══════════════════════════════════════════════════════════════
void SerialWork::onReadyRead()
{
    if (!m_serialPort)
        return;

    QByteArray chunk = m_serialPort->readAll();

    if (m_serialPage->m_serialBufferCheckBox->isChecked()) {
        m_recvBuffer.append(chunk);
        m_bufferTimer->start(20);
    } else {
        logRecv(chunk);
    }
}

// ═══════════════════════════════════════════════════════════════
//  缓冲区超时
// ═══════════════════════════════════════════════════════════════
void SerialWork::onBufferTimeout()
{
    if (!m_recvBuffer.isEmpty()) {
        logRecv(m_recvBuffer);
        m_recvBuffer.clear();
    }
}

// ═══════════════════════════════════════════════════════════════
//  UI 状态切换
// ═══════════════════════════════════════════════════════════════
void SerialWork::updateUIForOpened(bool opened)
{
    m_serialPage->m_openSerialButton->setEnabled(!opened);
    m_serialPage->m_closeSerialButton->setEnabled(opened);

    m_serialPage->m_serialPortComboBox->setEnabled(!opened);
    m_serialPage->m_baudRateComboBox->setEnabled(!opened);
    m_serialPage->m_dataBitsComboBox->setEnabled(!opened);
    m_serialPage->m_stopBitsComboBox->setEnabled(!opened);
    m_serialPage->m_parityComboBox->setEnabled(!opened);

    m_serialPage->m_singleSendBtn->setEnabled(opened);

    m_serialPage->m_excelOpenBtn->setEnabled(opened);

    if (!opened) {
        m_serialPage->m_excelCaptureBtn->setEnabled(false);
        m_serialPage->m_excelSendBtn->setEnabled(false);
    }

    LED::setLED(m_serialPage->m_serialLED, opened ? 2 : 0, 16);
}

// ═══════════════════════════════════════════════════════════════
//  记录发送日志
// ═══════════════════════════════════════════════════════════════
void SerialWork::logSend(const QString &displayText)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString line = QString("[%1] TX → %2").arg(timeStr, displayText);

    if (m_serialPage->m_singleSendLog) {
        m_serialPage->m_singleSendLog->addItem(line);
        while (m_serialPage->m_singleSendLog->count() > MAX_LOG_LINES)
            delete m_serialPage->m_singleSendLog->takeItem(0);
    }
    if (m_serialPage->m_logSendList) {
        m_serialPage->m_logSendList->addItem(line);
        while (m_serialPage->m_logSendList->count() > MAX_LOG_LINES)
            delete m_serialPage->m_logSendList->takeItem(0);
    }
}

// ═══════════════════════════════════════════════════════════════
//  记录接收日志
// ═══════════════════════════════════════════════════════════════
void SerialWork::logRecv(const QByteArray &data)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString displayText;

    if (m_serialPage->m_serialHexSendCheckBox->isChecked()) {
        displayText = data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty()) {
            displayText = text;
        } else {
            displayText = data.toHex(' ').toUpper();
        }
    }

    QString line = QString("[%1] RX ← %2").arg(timeStr, displayText);

    if (m_serialPage->m_singleRecvLog) {
        m_serialPage->m_singleRecvLog->addItem(line);
        while (m_serialPage->m_singleRecvLog->count() > MAX_LOG_LINES)
            delete m_serialPage->m_singleRecvLog->takeItem(0);
    }
    if (m_serialPage->m_logRecvList) {
        m_serialPage->m_logRecvList->addItem(line);
        while (m_serialPage->m_logRecvList->count() > MAX_LOG_LINES)
            delete m_serialPage->m_logRecvList->takeItem(0);
    }

    m_totalRecv++;
    if (m_serialPage->m_logRecvCountCard)
        m_serialPage->m_logRecvCountCard->setValue(QString::number(m_totalRecv));

    emit responseReceived(data);
}

// ═══════════════════════════════════════════════════════════════
//  重置接收计数
// ═══════════════════════════════════════════════════════════════
void SerialWork::resetRecvCount()
{
    m_totalRecv = 0;
}
