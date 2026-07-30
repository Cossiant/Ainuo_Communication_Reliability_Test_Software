// GPIBPageSignals.cpp

#include "GPIBPageSignals.h"
#include "GPIBPage.h"
#include "GPIBWork.h"
#include "GPIBExcel.h"
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

GPIBPageSignals::GPIBPageSignals(GPIBPage* page)
    : QObject(page), m_page(page)
{
    setupThreadAndWork();
    connectAllSignals();
}

void GPIBPageSignals::setupThreadAndWork()
{
    m_page->m_gpibThread = new QThread(m_page);
    m_page->m_gpibWork   = new GPIBWork();

    m_page->m_gpibWork->moveToThread(m_page->m_gpibThread);

    connect(m_page->m_gpibThread, &QThread::finished,
            m_page->m_gpibWork,   &QObject::deleteLater);

    m_page->m_gpibThread->start();

    // 连接超时定时器（主线程，5 秒）
    m_page->m_connectTimeoutTimer = new QTimer(m_page);
    m_page->m_connectTimeoutTimer->setSingleShot(true);
    connect(m_page->m_connectTimeoutTimer, &QTimer::timeout, m_page, [this]() {
        if (m_page->m_isConnecting) {
            m_page->m_isConnecting = false;
            QMessageBox::warning(m_page->m_mainWindow, "GPIB 连接超时",
                                 "GPIB 连接超时，请检查设备连接。\n"
                                 "请确认板卡号、主地址是否正确，仪器是否上电。");
            QMetaObject::invokeMethod(m_page->m_gpibWork, "closeGPIBPort",
                                      Qt::QueuedConnection);
        }
    });
}

void GPIBPageSignals::connectAllSignals()
{
    // ── ① 打开 GPIB 按钮 ──
    connect(m_page->m_openGpibButton, &ElaPushButton::clicked, m_page, [this]() {
        bool ok;
        int boardIndex = m_page->m_boardIndexEdit->text().toInt(&ok);
        if (!ok || boardIndex < 0) {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请输入有效的板卡号（0-3）！");
            return;
        }

        int primaryAddr = m_page->m_primaryAddrEdit->text().toInt(&ok);
        if (!ok || primaryAddr < 0 || primaryAddr > 30) {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请输入有效的主地址（0-30）！");
            return;
        }

        int secondaryAddr = m_page->m_secondaryAddrEdit->text().toInt(&ok);
        if (!ok) secondaryAddr = 0;
        if (secondaryAddr < 0 || secondaryAddr > 30) {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请输入有效的副地址（0-30，0 表示不使用）！");
            return;
        }

        int timeoutMs = m_page->m_timeoutEdit->text().toInt(&ok);
        if (!ok || timeoutMs < 100) timeoutMs = 3000;

        bool termCharEnabled = m_page->m_termCharEnabledCheckBox->isChecked();
        QString termCharStr = m_page->m_termCharEdit->text();
        char termChar = termCharStr.isEmpty() ? '\n' : termCharStr.at(0).toLatin1();
        bool sendEndEnabled = m_page->m_sendEndEnabledCheckBox->isChecked();
        bool hexMode = m_page->m_gpibHexSendCheckBox->isChecked();

        m_page->m_isConnecting = true;
        m_page->m_connectTimeoutTimer->start(5000);

        QMetaObject::invokeMethod(m_page->m_gpibWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, hexMode));
        QMetaObject::invokeMethod(m_page->m_gpibWork, "openGPIBPort",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, boardIndex),
                                  Q_ARG(int, primaryAddr),
                                  Q_ARG(int, secondaryAddr),
                                  Q_ARG(int, timeoutMs),
                                  Q_ARG(bool, termCharEnabled),
                                  Q_ARG(char, termChar),
                                  Q_ARG(bool, sendEndEnabled));
    });

    // ── ② 关闭 GPIB 按钮 ──
    connect(m_page->m_closeGpibButton, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();
        QMetaObject::invokeMethod(m_page->m_gpibWork, "closeGPIBPort",
                                  Qt::QueuedConnection);
    });

    // ── ③ 单条发送按钮 ──
    connect(m_page->m_singleSendBtn, &ElaPushButton::clicked, m_page, [this]() {
        QString text = m_page->m_singleSendInput->text();
        bool hexMode = m_page->m_gpibHexSendCheckBox->isChecked();
        QMetaObject::invokeMethod(m_page->m_gpibWork, "sendString",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text),
                                  Q_ARG(bool, hexMode));
    });

    // ── ④ HEX 勾选框 → 同步显示模式 + 联动 HEX 区间判断控件 ──
    connect(m_page->m_gpibHexSendCheckBox, &ElaCheckBox::toggled, m_page, [this](bool checked) {
        // 原有：同步 HEX 显示模式
        QMetaObject::invokeMethod(m_page->m_gpibWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, checked));

        // 联动 HEX 区间判断控件的启用/禁用
        if (m_page->m_gpibHexRangeCheckBox) {
            m_page->m_gpibHexRangeCheckBox->setEnabled(checked);
            if (!checked) {
                // 取消 HEX 发送时，关闭区间判断并禁用偏差值输入
                m_page->m_gpibHexRangeCheckBox->setChecked(false);
                if (m_page->m_gpibHexRangeEdit)
                    m_page->m_gpibHexRangeEdit->setEnabled(false);
            }
        }
    });

    // HEX 区间判断勾选 → 联动偏差值输入框
    if (m_page->m_gpibHexRangeCheckBox && m_page->m_gpibHexRangeEdit) {
        connect(m_page->m_gpibHexRangeCheckBox, &ElaCheckBox::toggled,
                m_page, [this](bool checked) {
            m_page->m_gpibHexRangeEdit->setEnabled(checked);
        });
    }

    // 产品系列切换 → 更新 An30Layout
    connect(m_page->m_gpibProductComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        An30Product prod;
        switch (index) {
            case 0: prod = An30Product::RGL; break;
            case 1: prod = An30Product::EVT; break;   // ★ 新增
            case 2: prod = An30Product::EVH; break;
            default: prod = An30Product::RGL; break;
        }
        An30Layout::instance().setProduct(prod);
    });


    // ── ⑤ 后缀选择变更 → 同步到工作线程 ──
    connect(m_page->m_suffixComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        QMetaObject::invokeMethod(m_page->m_gpibWork, "setSuffixMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, index));
    });

    // ── ⑥ GPIB 打开成功 ──
    connect(m_page->m_gpibWork, &GPIBWork::gpibOpened, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        m_page->m_openGpibButton->setEnabled(false);
        m_page->m_closeGpibButton->setEnabled(true);

        m_page->m_boardIndexEdit->setEnabled(false);
        m_page->m_primaryAddrEdit->setEnabled(false);
        m_page->m_secondaryAddrEdit->setEnabled(false);
        m_page->m_timeoutEdit->setEnabled(false);
        m_page->m_termCharEdit->setEnabled(false);
        m_page->m_termCharEnabledCheckBox->setEnabled(false);
        m_page->m_sendEndEnabledCheckBox->setEnabled(false);

        m_page->m_singleSendBtn->setEnabled(true);
        m_page->m_excelOpenBtn->setEnabled(true);

        bool hasData = (m_page->m_excelTableWidget->rowCount() > 0);
        m_page->m_excelCaptureBtn->setEnabled(hasData);
        m_page->m_excelSendBtn->setEnabled(hasData);

        LED::setLED(m_page->m_gpibLED, 2, 16);
    });

    // ── ⑦ GPIB 关闭 ──
    connect(m_page->m_gpibWork, &GPIBWork::gpibClosed, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        m_page->m_openGpibButton->setEnabled(true);
        m_page->m_closeGpibButton->setEnabled(false);

        m_page->m_boardIndexEdit->setEnabled(true);
        m_page->m_primaryAddrEdit->setEnabled(true);
        m_page->m_secondaryAddrEdit->setEnabled(true);
        m_page->m_timeoutEdit->setEnabled(true);
        m_page->m_termCharEdit->setEnabled(true);
        m_page->m_termCharEnabledCheckBox->setEnabled(true);
        m_page->m_sendEndEnabledCheckBox->setEnabled(true);

        m_page->m_singleSendBtn->setEnabled(false);
        m_page->m_excelOpenBtn->setEnabled(false);
        m_page->m_excelCaptureBtn->setEnabled(false);
        m_page->m_excelSendBtn->setEnabled(false);

        LED::setLED(m_page->m_gpibLED, 0, 16);
    });

    // ── ⑧ 错误提示 ──
    connect(m_page->m_gpibWork, &GPIBWork::errorOccurred, m_page, [this](const QString &msg) {
        bool wasConnecting = m_page->m_isConnecting;
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        if (wasConnecting) {
            QMessageBox::critical(m_page->m_mainWindow, "GPIB 错误", msg);
        } else {
            qDebug() << "GPIBPage: 非连接阶段错误 -" << msg;
        }
    });

    // ── ⑨ 发送日志行 ──
    connect(m_page->m_gpibWork, &GPIBWork::sendLogLine, m_page, [this](const QString &line) {
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
    connect(m_page->m_gpibWork, &GPIBWork::recvLogLine, m_page, [this](const QString &line) {
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
    connect(m_page->m_gpibWork, &GPIBWork::recvCountChanged, m_page, [this](int count) {
        if (m_page->m_logRecvCountCard)
            m_page->m_logRecvCountCard->setValue(QString::number(count));
    });

    // ── ⑫ 清空按钮 ──
    connect(m_page->m_logClearBtn, &ElaPushButton::clicked, m_page, [this]() {
        if (m_page->m_singleSendLog)   m_page->m_singleSendLog->clear();
        if (m_page->m_singleRecvLog)   m_page->m_singleRecvLog->clear();
        if (m_page->m_logSendList)     m_page->m_logSendList->clear();
        if (m_page->m_logRecvList)     m_page->m_logRecvList->clear();
        QMetaObject::invokeMethod(m_page->m_gpibWork, "resetRecvCount",
                                  Qt::QueuedConnection);
    });

    // ── ⑬ 暂停/恢复日志 ──
    connect(m_page->m_logPauseBtn, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_logPaused = !m_page->m_logPaused;
        if (m_page->m_logPaused) {
            m_page->m_logPauseBtn->setText("恢复日志");
            LED::setLED(m_page->m_logLED, 0, 14);
            qDebug() << "GPIBPage: 日志更新已暂停";
        } else {
            m_page->m_logPauseBtn->setText("暂停日志");
            LED::setLED(m_page->m_logLED, 2, 14);
            qDebug() << "GPIBPage: 日志更新已恢复";
        }
    });
}
