// SerialPageSignals.cpp

#include "SerialPageSignals.h"
#include "SerialPage.h"
#include "SerialWork.h"
#include "SerialExcel.h"
#include "../Other_T/LED.h"
#include "../Other_T/An30Layout.h"

#include "ElaComboBox.h"
#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaLineEdit.h"
#include "ElaWindow.h"

#include <QThread>
#include <QDebug>
#include <QMessageBox>
#include <QSerialPortInfo>

SerialPageSignals::SerialPageSignals(SerialPage* page)
    : QObject(page), m_page(page)
{
    setupThreadAndWork();
    connectAllSignals();
}

void SerialPageSignals::setupThreadAndWork()
{
    m_page->m_serialThread = new QThread(m_page);
    m_page->m_serialWork   = new SerialWork();

    m_page->m_serialWork->moveToThread(m_page->m_serialThread);

    connect(m_page->m_serialThread, &QThread::finished,
            m_page->m_serialWork,   &QObject::deleteLater);

    m_page->m_serialThread->start();

    qDebug() << "SerialPage: 工作线程已启动，ID =" << m_page->m_serialThread;
}

void SerialPageSignals::connectAllSignals()
{
    // ── ① 打开串口按钮 ──
    connect(m_page->m_openSerialButton, &ElaPushButton::clicked, m_page, [this]() {
        QString portName = m_page->m_serialPortComboBox->currentText();
        if (portName.isEmpty() || portName == "无可用串口") {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请选择有效的串口端口！");
            return;
        }

        int baudRate = m_page->m_baudRateComboBox->currentText().toInt();

        QSerialPort::DataBits dataBits;
        QString db = m_page->m_dataBitsComboBox->currentText();
        if (db == "5")      dataBits = QSerialPort::Data5;
        else if (db == "6") dataBits = QSerialPort::Data6;
        else if (db == "7") dataBits = QSerialPort::Data7;
        else                dataBits = QSerialPort::Data8;

        QSerialPort::StopBits stopBits;
        QString sb = m_page->m_stopBitsComboBox->currentText();
        if (sb == "1.5")    stopBits = QSerialPort::OneAndHalfStop;
        else if (sb == "2") stopBits = QSerialPort::TwoStop;
        else                stopBits = QSerialPort::OneStop;

        QSerialPort::Parity parity;
        QString pa = m_page->m_parityComboBox->currentText();
        if (pa == "Even")      parity = QSerialPort::EvenParity;
        else if (pa == "Odd")   parity = QSerialPort::OddParity;
        else if (pa == "Space") parity = QSerialPort::SpaceParity;
        else if (pa == "Mark")  parity = QSerialPort::MarkParity;
        else                    parity = QSerialPort::NoParity;

        bool buffered = m_page->m_serialBufferCheckBox->isChecked();
        bool hexMode  = m_page->m_serialHexSendCheckBox->isChecked();

        QMetaObject::invokeMethod(m_page->m_serialWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, hexMode));
        QMetaObject::invokeMethod(m_page->m_serialWork, "openSerialPort",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, portName),
                                  Q_ARG(int, baudRate),
                                  Q_ARG(QSerialPort::DataBits, dataBits),
                                  Q_ARG(QSerialPort::Parity, parity),
                                  Q_ARG(QSerialPort::StopBits, stopBits),
                                  Q_ARG(bool, buffered));
    });

    // ── ② 关闭串口按钮 ──
    connect(m_page->m_closeSerialButton, &ElaPushButton::clicked, m_page, [this]() {
        QMetaObject::invokeMethod(m_page->m_serialWork, "closeSerialPort",
                                  Qt::QueuedConnection);
    });

    // ── ③ 单条发送按钮 ──
    connect(m_page->m_singleSendBtn, &ElaPushButton::clicked, m_page, [this]() {
        QString text = m_page->m_singleSendInput->text();
        bool hexMode = m_page->m_serialHexSendCheckBox->isChecked();
        QMetaObject::invokeMethod(m_page->m_serialWork, "sendString",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text),
                                  Q_ARG(bool, hexMode));
    });

    // ── ④ HEX 勾选框 → 同步显示模式 + 联动 HEX 区间判断控件 ──
    connect(m_page->m_serialHexSendCheckBox, &ElaCheckBox::toggled, m_page, [this](bool checked) {
        // 原有：同步 HEX 显示模式
        QMetaObject::invokeMethod(m_page->m_serialWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, checked));

        // 联动 HEX 区间判断控件的启用/禁用
        if (m_page->m_serialHexRangeCheckBox) {
            m_page->m_serialHexRangeCheckBox->setEnabled(checked);
            if (!checked) {
                // 取消 HEX 发送时，关闭区间判断并禁用偏差值输入
                m_page->m_serialHexRangeCheckBox->setChecked(false);
                if (m_page->m_serialHexRangeEdit)
                    m_page->m_serialHexRangeEdit->setEnabled(false);
            }
        }
    });

    // HEX 区间判断勾选 → 联动偏差值输入框
    if (m_page->m_serialHexRangeCheckBox && m_page->m_serialHexRangeEdit) {
        connect(m_page->m_serialHexRangeCheckBox, &ElaCheckBox::toggled,
                m_page, [this](bool checked) {
            m_page->m_serialHexRangeEdit->setEnabled(checked);
        });
    }

    // ★ 新增：产品系列切换 → 更新 An30Layout
    connect(m_page->m_serialProductComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        An30Layout::instance().setProduct(
            index == 0 ? An30Product::RGL : An30Product::EVH);
    });


    // ── ⑤ 后缀选择变更 → 同步到工作线程 ──
    connect(m_page->m_suffixComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        QMetaObject::invokeMethod(m_page->m_serialWork, "setSuffixMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, index));
    });

    // ── ⑥ 串口打开成功 ──
    connect(m_page->m_serialWork, &SerialWork::serialOpened, m_page, [this]() {
        m_page->m_openSerialButton->setEnabled(false);
        m_page->m_closeSerialButton->setEnabled(true);

        m_page->m_serialPortComboBox->setEnabled(false);
        m_page->m_baudRateComboBox->setEnabled(false);
        m_page->m_dataBitsComboBox->setEnabled(false);
        m_page->m_stopBitsComboBox->setEnabled(false);
        m_page->m_parityComboBox->setEnabled(false);

        m_page->m_singleSendBtn->setEnabled(true);
        m_page->m_excelOpenBtn->setEnabled(true);

        bool hasData = (m_page->m_excelTableWidget->rowCount() > 0);
        m_page->m_excelCaptureBtn->setEnabled(hasData);
        m_page->m_excelSendBtn->setEnabled(hasData);

        LED::setLED(m_page->m_serialLED, 2, 16);
    });

    // ── ⑦ 串口关闭 ──
    connect(m_page->m_serialWork, &SerialWork::serialClosed, m_page, [this]() {
        m_page->m_openSerialButton->setEnabled(true);
        m_page->m_closeSerialButton->setEnabled(false);

        m_page->m_serialPortComboBox->setEnabled(true);
        m_page->m_baudRateComboBox->setEnabled(true);
        m_page->m_dataBitsComboBox->setEnabled(true);
        m_page->m_stopBitsComboBox->setEnabled(true);
        m_page->m_parityComboBox->setEnabled(true);

        m_page->m_singleSendBtn->setEnabled(false);
        m_page->m_excelOpenBtn->setEnabled(false);
        m_page->m_excelCaptureBtn->setEnabled(false);
        m_page->m_excelSendBtn->setEnabled(false);

        LED::setLED(m_page->m_serialLED, 0, 16);
    });

    // ── ⑧ 错误提示（静默）──
    connect(m_page->m_serialWork, &SerialWork::errorOccurred, m_page, [this](const QString &msg) {
        qDebug() << "SerialPage: 串口错误 -" << msg;
    });

    // ── ⑨ 发送日志行 ──
    connect(m_page->m_serialWork, &SerialWork::sendLogLine, m_page, [this](const QString &line) {
        if (m_page->m_logPaused) return;
        if (m_page->m_singleSendLog) {
            m_page->m_singleSendLog->addItem(line);
            while (m_page->m_singleSendLog->count() > 200)
                delete m_page->m_singleSendLog->takeItem(0);
        }
        if (m_page->m_logSendList) {
            m_page->m_logSendList->addItem(line);
            while (m_page->m_logSendList->count() > 200)
                delete m_page->m_logSendList->takeItem(0);
        }
    });

    // ── ⑩ 接收日志行 ──
    connect(m_page->m_serialWork, &SerialWork::recvLogLine, m_page, [this](const QString &line) {
        if (m_page->m_logPaused) return;
        if (m_page->m_singleRecvLog) {
            m_page->m_singleRecvLog->addItem(line);
            while (m_page->m_singleRecvLog->count() > 200)
                delete m_page->m_singleRecvLog->takeItem(0);
        }
        if (m_page->m_logRecvList) {
            m_page->m_logRecvList->addItem(line);
            while (m_page->m_logRecvList->count() > 200)
                delete m_page->m_logRecvList->takeItem(0);
        }
    });

    // ── ⑪ 接收计数 ──
    connect(m_page->m_serialWork, &SerialWork::recvCountChanged, m_page, [this](int count) {
        if (m_page->m_logRecvCountCard)
            m_page->m_logRecvCountCard->setValue(QString::number(count));
    });

    // ── ⑫ 清空按钮 ──
    connect(m_page->m_logClearBtn, &ElaPushButton::clicked, m_page, [this]() {
        if (m_page->m_singleSendLog)   m_page->m_singleSendLog->clear();
        if (m_page->m_singleRecvLog)   m_page->m_singleRecvLog->clear();
        if (m_page->m_logSendList)     m_page->m_logSendList->clear();
        if (m_page->m_logRecvList)     m_page->m_logRecvList->clear();
        QMetaObject::invokeMethod(m_page->m_serialWork, "resetRecvCount",
                                  Qt::QueuedConnection);
    });

    // ── ⑬ 暂停/恢复日志按钮 ──
    connect(m_page->m_logPauseBtn, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_logPaused = !m_page->m_logPaused;
        if (m_page->m_logPaused) {
            m_page->m_logPauseBtn->setText("恢复日志");
            LED::setLED(m_page->m_logLED, 0, 14);
            qDebug() << "SerialPage: 日志更新已暂停";
        } else {
            m_page->m_logPauseBtn->setText("暂停日志");
            LED::setLED(m_page->m_logLED, 2, 14);
            qDebug() << "SerialPage: 日志更新已恢复";
        }
    });
}
