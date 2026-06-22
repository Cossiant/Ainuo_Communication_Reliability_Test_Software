#include "SerialWork.h"
#include "SerialThread.h"
#include "SerialPage.h"
#include "ElaWindow.h"
#include "../Other_T/LED.h"

#include <QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QMetaObject>

SerialWork::SerialWork(SerialPage *serialPage, QObject *parent)
    : QObject(parent),
      m_serialPage(serialPage),
      m_mainWindow(serialPage->m_mainWindow)
{
    // ════════════ 连接 SerialPage 按钮信号 ════════════
    connect(m_serialPage->m_openSerialButton,  &ElaPushButton::clicked,
            this, &SerialWork::onOpenSerial);
    connect(m_serialPage->m_closeSerialButton, &ElaPushButton::clicked,
            this, &SerialWork::onCloseSerial);

}

SerialWork::~SerialWork()
{
    // 先同步通知 worker 停止，再清理线程
    if (m_worker && m_thread && m_thread->isRunning()) {
        QMetaObject::invokeMethod(m_worker, "onSerialStop",
                                  Qt::BlockingQueuedConnection);
    }
    cleanupThread();
}

bool SerialWork::isSerialOpen() const
{
    return (m_thread != nullptr && m_thread->isRunning());
}

void SerialWork::sendData(const QByteArray &data)
{
    if (!m_worker || !m_thread || !m_thread->isRunning()) {
        qWarning() << "SerialWork: 串口未打开，无法发送";
        return;
    }
    QMetaObject::invokeMethod(m_worker, "sendData",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
}

// ═══════════════════════════════════════════════════════════════
//  打开串口
// ═══════════════════════════════════════════════════════════════
void SerialWork::onOpenSerial()
{
    // ──── 防止重复打开 ────
    if (m_isOpening || isSerialOpen()) {
        qWarning() << "SerialWork: 串口已打开或正在打开，忽略重复请求";
        return;
    }

    QString portName = m_serialPage->m_serialPortComboBox->currentText();
    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(m_mainWindow, "警告", "请选择有效的串口端口！");
        return;
    }

    m_isOpening = true;

    // ──── 解析参数 ────
    qint32 baudRate = m_serialPage->m_baudRateComboBox->currentText().toInt();

    auto parseDataBits = [](const QString &s) -> QSerialPort::DataBits {
        if (s == "5") return QSerialPort::Data5;
        if (s == "6") return QSerialPort::Data6;
        if (s == "7") return QSerialPort::Data7;
        return QSerialPort::Data8;
    };
    auto parseStopBits = [](const QString &s) -> QSerialPort::StopBits {
        if (s == "1.5") return QSerialPort::OneAndHalfStop;
        if (s == "2")   return QSerialPort::TwoStop;
        return QSerialPort::OneStop;
    };
    auto parseParity = [](const QString &s) -> QSerialPort::Parity {
        if (s == "Even")  return QSerialPort::EvenParity;
        if (s == "Odd")   return QSerialPort::OddParity;
        if (s == "Space") return QSerialPort::SpaceParity;
        if (s == "Mark")  return QSerialPort::MarkParity;
        return QSerialPort::NoParity;
    };

    QSerialPort::DataBits    dataBits    = parseDataBits(m_serialPage->m_dataBitsComboBox->currentText());
    QSerialPort::StopBits    stopBits    = parseStopBits(m_serialPage->m_stopBitsComboBox->currentText());
    QSerialPort::Parity      parity      = parseParity(m_serialPage->m_parityComboBox->currentText());
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;

    // ──── 确保旧资源已清理 ────
    cleanupThread();

    // ──── 创建线程与工作对象 ────
    m_thread = new QThread(this);
    m_thread->setObjectName("SerialThread");       // ← 线程命名为 SerialThread

    m_worker = new SerialThread();                 // ← 无 parent，手动 moveToThread
    m_worker->moveToThread(m_thread);

    // ──── 工作对象关闭后自动清理 ────
    connect(m_worker, &SerialThread::serialClosed,
            this, &SerialWork::onSerialClosed);
    connect(m_worker, &SerialThread::errorOccurred,
            this, &SerialWork::onWorkerError);

    // ──── 线程结束时自动删除 ────
    connect(m_thread, &QThread::finished,
            m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished,
            m_thread, &QObject::deleteLater);

    // ──── 缓冲区模式 ────
    connect(m_serialPage->m_serialBufferCheckBox, &ElaCheckBox::toggled,
            m_worker, &SerialThread::setBufferMode);
    QMetaObject::invokeMethod(m_worker, "setBufferMode",
                              Qt::QueuedConnection,
                              Q_ARG(bool, m_serialPage->m_serialBufferCheckBox->isChecked()));

    // ──── 启动线程 → 打开串口 ────
    m_thread->start();

    QMetaObject::invokeMethod(m_worker, "onSerialStart",
                              Qt::QueuedConnection,
                              Q_ARG(QString, portName),
                              Q_ARG(qint32, baudRate),
                              Q_ARG(QSerialPort::DataBits, dataBits),
                              Q_ARG(QSerialPort::Parity, parity),
                              Q_ARG(QSerialPort::StopBits, stopBits),
                              Q_ARG(QSerialPort::FlowControl, flowControl));

    // ──── UI 状态更新 ────
    updateUIForOpened(true);
    LED::setLED(m_serialPage->m_serialLED, 2, 16);     // 绿色 = 已连接

    m_isOpening = false;

    qDebug() << "SerialWork: 串口已打开" << portName << baudRate;
}

// ═══════════════════════════════════════════════════════════════
//  关闭串口（用户主动点击）
// ═══════════════════════════════════════════════════════════════
void SerialWork::onCloseSerial()
{
    qDebug() << "SerialWork: 用户请求关闭串口";

    if (!m_worker || !m_thread || !m_thread->isRunning()) {
        qWarning() << "SerialWork: 串口未打开，无需关闭";
        return;
    }

    QMetaObject::invokeMethod(m_worker, "onSerialStop",
                              Qt::QueuedConnection);
}

// ═══════════════════════════════════════════════════════════════
//  串口已关闭（SerialThread 通知），清理线程资源
// ═══════════════════════════════════════════════════════════════
void SerialWork::onSerialClosed()
{
    qDebug() << "SerialWork: 收到串口关闭通知，开始清理";

    cleanupThread();

    updateUIForOpened(false);
    LED::setLED(m_serialPage->m_serialLED, 0, 16);     // 灰色 = 未连接

    m_isOpening = false;
}

// ═══════════════════════════════════════════════════════════════
//  SerialThread 报错处理
// ═══════════════════════════════════════════════════════════════
void SerialWork::onWorkerError(const QString &errorMessage)
{
    qWarning() << "SerialWork: 错误 —" << errorMessage;
    QMessageBox::critical(m_mainWindow, "串口错误", errorMessage);
}

// ═══════════════════════════════════════════════════════════════
//  内部：安全清理线程
// ═══════════════════════════════════════════════════════════════
void SerialWork::cleanupThread()
{
    if (m_thread) {
        if (m_thread->isRunning()) {
            m_thread->quit();
            if (!m_thread->wait(3000)) {
                qWarning() << "SerialWork: 线程退出超时，强制终止";
                m_thread->terminate();
                m_thread->wait(1000);
            }
        }
        // finished 信号已连接 deleteLater，不手动 delete
        m_thread = nullptr;
    }

    // worker 由 QThread::finished → deleteLater 自动处理
    m_worker = nullptr;
}

// ═══════════════════════════════════════════════════════════════
//  内部：更新 UI 控件状态
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
}
