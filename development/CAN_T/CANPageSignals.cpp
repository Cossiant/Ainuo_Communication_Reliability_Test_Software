#include "CANPageSignals.h"

#include "CANPage.h"
#include "CANWork.h"
#include "CANFrameUtils.h"
#include "../Other_T/LED.h"
#include "ElaWindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QThread>

CANPageSignals::CANPageSignals(CANPage *page)
    : QObject(page), m_page(page)
{
    setupThreadAndWork();
    connectAllSignals();
}

void CANPageSignals::setupThreadAndWork()
{
    m_page->m_canThread = new QThread(m_page);
    m_page->m_canWork = new CANWork();
    m_page->m_canWork->moveToThread(m_page->m_canThread);

    connect(m_page->m_canThread, &QThread::finished,
            m_page->m_canWork, &QObject::deleteLater);

    connect(this, &CANPageSignals::openCANRequested,
            m_page->m_canWork, &CANWork::openCANPort);

    m_page->m_canThread->start();
    qDebug() << "CANPage: 工作线程已启动，ID =" << m_page->m_canThread;
}

void CANPageSignals::connectAllSignals()
{
    // 打开 CAN
    connect(m_page->m_openButton, &ElaPushButton::clicked, m_page, [this]() {
        bool okType = false, okIndex = false, okChannel = false;
        const int deviceType = m_page->m_deviceTypeEdit->text().toInt(&okType);
        const int deviceIndex = m_page->m_deviceIndexEdit->text().toInt(&okIndex);
        const int canIndex = m_page->m_canIndexEdit->text().toInt(&okChannel);
        if (!okType || !okIndex || !okChannel) {
            QMessageBox::warning(m_page->m_mainWindow, QStringLiteral("警告"),
                                 QStringLiteral("设备类型、设备索引和 CAN 通道必须为整数。"));
            return;
        }

        const int canType = m_page->m_canModeComboBox->currentText() == QStringLiteral("CAN")
                ? 0 : 1;
        const int abitBaud = m_page->m_baudRateComboBox->currentText().toInt();
        const int dbitBaud = m_page->m_dbitBaudComboBox->currentText().toInt();
        const int terminalResistance = m_page->m_terminalResCheckBox->isChecked() ? 1 : 0;

        emit openCANRequested(deviceType, deviceIndex, canIndex,
                              canType, abitBaud, dbitBaud,
                              0, terminalResistance,
                              0, 0xFFFFFFFF, 0, 0);
    });

    // 关闭 CAN
    connect(m_page->m_closeButton, &ElaPushButton::clicked, m_page, [this]() {
        QMetaObject::invokeMethod(m_page->m_canWork, "closeCANPort",
                                  Qt::QueuedConnection);
    });

    // 单条发送
    connect(m_page->m_singleSendBtn, &ElaPushButton::clicked, m_page, [this]() {
        const QString text = m_page->m_singleSendInput->text().trimmed();
        if (text.isEmpty()) {
            return;
        }

        quint32 canId = 0;
        QByteArray data;
        bool isFd = false;
        bool brs = false;
        QString error;
        if (!CANFrameUtils::parseFrameText(text, canId, data, isFd, brs, &error)) {
            QMessageBox::warning(m_page->m_mainWindow, QStringLiteral("帧格式错误"), error);
            return;
        }

        const QByteArray frame = CANFrameUtils::encodeFrame(canId, data, isFd, brs);
        QMetaObject::invokeMethod(m_page->m_canWork, "sendFrameWithDelay",
                                  Qt::QueuedConnection,
                                  Q_ARG(QByteArray, frame),
                                  Q_ARG(QByteArray, QByteArray()),
                                  Q_ARG(int, 0),
                                  Q_ARG(bool, false),
                                  Q_ARG(int, 0));
    });

    // 打开成功
    connect(m_page->m_canWork, &CANWork::canOpened, m_page, [this]() {
        m_page->m_openButton->setEnabled(false);
        m_page->m_closeButton->setEnabled(true);
        m_page->m_singleSendBtn->setEnabled(true);
        m_page->m_excelOpenBtn->setEnabled(true);

        const bool hasData = m_page->m_excelTableWidget->rowCount() > 0;
        m_page->m_excelCaptureBtn->setEnabled(hasData);
        m_page->m_excelSendBtn->setEnabled(hasData);
        LED::setLED(m_page->m_canLED, 2, 16);
    });

    // 关闭
    connect(m_page->m_canWork, &CANWork::canClosed, m_page, [this]() {
        m_page->m_openButton->setEnabled(true);
        m_page->m_closeButton->setEnabled(false);
        m_page->m_singleSendBtn->setEnabled(false);
        m_page->m_excelOpenBtn->setEnabled(false);
        m_page->m_excelCaptureBtn->setEnabled(false);
        m_page->m_excelSendBtn->setEnabled(false);
        m_page->m_excelStopBtn->setEnabled(false);
        LED::setLED(m_page->m_canLED, 0, 16);
    });

    // 错误提示
    connect(m_page->m_canWork, &CANWork::errorOccurred, m_page, [this](const QString &msg) {
        qDebug() << "CANPage: 错误 -" << msg;
        QMessageBox::warning(m_page->m_mainWindow, QStringLiteral("CAN 错误"), msg);
    });

    // 发送日志
    connect(m_page->m_canWork, &CANWork::sendLogLine, m_page, [this](const QString &line) {
        if (!m_page->m_singleLogPaused && m_page->m_singleSendLog) {
            m_page->m_singleSendLog->addItem(line);
            while (m_page->m_singleSendLog->count() > 200) {
                delete m_page->m_singleSendLog->takeItem(0);
            }
        }
        if (!m_page->m_logPaused && m_page->m_logSendList) {
            m_page->m_logSendList->addItem(line);
            while (m_page->m_logSendList->count() > 200) {
                delete m_page->m_logSendList->takeItem(0);
            }
        }
    });

    // 接收日志
    connect(m_page->m_canWork, &CANWork::recvLogLine, m_page, [this](const QString &line) {
        if (!m_page->m_singleLogPaused && m_page->m_singleRecvLog) {
            m_page->m_singleRecvLog->addItem(line);
            while (m_page->m_singleRecvLog->count() > 200) {
                delete m_page->m_singleRecvLog->takeItem(0);
            }
        }
        if (!m_page->m_logPaused && m_page->m_logRecvList) {
            m_page->m_logRecvList->addItem(line);
            while (m_page->m_logRecvList->count() > 200) {
                delete m_page->m_logRecvList->takeItem(0);
            }
        }
    });

    // 接收计数
    connect(m_page->m_canWork, &CANWork::recvCountChanged, m_page, [this](int count) {
        if (m_page->m_logRecvCountCard) {
            m_page->m_logRecvCountCard->setValue(QString::number(count));
        }
    });

    // 主日志清空
    connect(m_page->m_logClearBtn, &ElaPushButton::clicked, m_page, [this]() {
        if (m_page->m_singleSendLog) m_page->m_singleSendLog->clear();
        if (m_page->m_singleRecvLog) m_page->m_singleRecvLog->clear();
        if (m_page->m_logSendList) m_page->m_logSendList->clear();
        if (m_page->m_logRecvList) m_page->m_logRecvList->clear();
        QMetaObject::invokeMethod(m_page->m_canWork, "resetRecvCount", Qt::QueuedConnection);
    });

    // 主日志暂停
    connect(m_page->m_logPauseBtn, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_logPaused = !m_page->m_logPaused;
        if (m_page->m_logPaused) {
            m_page->m_logPauseBtn->setText(QStringLiteral("恢复日志"));
            LED::setLED(m_page->m_logLED, 0, 14);
        } else {
            m_page->m_logPauseBtn->setText(QStringLiteral("暂停日志"));
            LED::setLED(m_page->m_logLED, 2, 14);
        }
    });

    // 单条日志暂停
    connect(m_page->m_singleLogPauseBtn, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_singleLogPaused = !m_page->m_singleLogPaused;
        if (m_page->m_singleLogPaused) {
            m_page->m_singleLogPauseBtn->setText(QStringLiteral("恢复日志"));
            LED::setLED(m_page->m_singleLogLED, 0, 14);
        } else {
            m_page->m_singleLogPauseBtn->setText(QStringLiteral("暂停日志"));
            LED::setLED(m_page->m_singleLogLED, 2, 14);
        }
    });
}
