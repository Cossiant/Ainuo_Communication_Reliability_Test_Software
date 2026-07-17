// NetworkPageSignals.cpp

#include "NetworkPageSignals.h"
#include "NetworkPage.h"
#include "NetworkWork.h"
#include "NetworkExcel.h"
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

NetworkPageSignals::NetworkPageSignals(NetworkPage* page)
    : QObject(page), m_page(page)
{
    setupThreadAndWork();
    connectAllSignals();
}

void NetworkPageSignals::setupThreadAndWork()
{
    m_page->m_networkThread = new QThread(m_page);
    m_page->m_networkWork   = new NetworkWork();

    m_page->m_networkWork->moveToThread(m_page->m_networkThread);

    connect(m_page->m_networkThread, &QThread::finished,
            m_page->m_networkWork,   &QObject::deleteLater);

    m_page->m_networkThread->start();

    qDebug() << "NetworkPage: 工作线程已启动，ID =" << m_page->m_networkThread;

    // 连接超时定时器（主线程，3 秒）
    m_page->m_connectTimeoutTimer = new QTimer(m_page);
    m_page->m_connectTimeoutTimer->setSingleShot(true);
    connect(m_page->m_connectTimeoutTimer, &QTimer::timeout, m_page, [this]() {
        if (m_page->m_isConnecting) {
            m_page->m_isConnecting = false;
            QMessageBox::warning(m_page->m_mainWindow, "网络连接超时",
                                 "网络连接超时，请检查网络。\n"
                                 "请确认 IP 地址和端口号是否正确，目标设备是否在线。");
            QMetaObject::invokeMethod(m_page->m_networkWork, "disconnectFromHost",
                                      Qt::QueuedConnection);
        }
    });
}

void NetworkPageSignals::connectAllSignals()
{
    // ── ① 连接网络按钮 ──
    connect(m_page->m_openNetworkButton, &ElaPushButton::clicked, m_page, [this]() {
        QString ipAddress = m_page->m_ipAddressEdit->text().trimmed();
        if (ipAddress.isEmpty()) {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请输入有效的 IP 地址！");
            return;
        }

        bool portOk = false;
        quint16 port = static_cast<quint16>(m_page->m_portEdit->text().toUInt(&portOk));
        if (!portOk || port == 0) {
            QMessageBox::warning(m_page->m_mainWindow, "警告", "请输入有效的端口号（1-65535）！");
            return;
        }

        bool hexMode      = m_page->m_networkHexSendCheckBox->isChecked();
        bool disableNagle = m_page->m_nagleCheckBox->isChecked();

        m_page->m_isConnecting = true;
        m_page->m_connectTimeoutTimer->start(3000);

        QMetaObject::invokeMethod(m_page->m_networkWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, hexMode));
        QMetaObject::invokeMethod(m_page->m_networkWork, "connectToHost",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, ipAddress),
                                  Q_ARG(quint16, port),
                                  Q_ARG(bool, disableNagle));
    });

    // ── ② 断开网络按钮 ──
    connect(m_page->m_closeNetworkButton, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();
        QMetaObject::invokeMethod(m_page->m_networkWork, "disconnectFromHost",
                                  Qt::QueuedConnection);
    });

    // ── ③ 单条发送按钮 ──
    connect(m_page->m_singleSendBtn, &ElaPushButton::clicked, m_page, [this]() {
        QString text = m_page->m_singleSendInput->text();
        bool hexMode = m_page->m_networkHexSendCheckBox->isChecked();
        QMetaObject::invokeMethod(m_page->m_networkWork, "sendString",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text),
                                  Q_ARG(bool, hexMode));
    });

    // ── ④ HEX 勾选框 → 同步显示模式 + 联动 HEX 区间判断控件 ──
    connect(m_page->m_networkHexSendCheckBox, &ElaCheckBox::toggled, m_page, [this](bool checked) {
        // 原有：同步 HEX 显示模式
        QMetaObject::invokeMethod(m_page->m_networkWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, checked));

        // ★ 新增：联动 HEX 区间判断控件的启用/禁用
        if (m_page->m_networkHexRangeCheckBox) {
            m_page->m_networkHexRangeCheckBox->setEnabled(checked);
            if (!checked) {
                // 取消 HEX 发送时，关闭区间判断并禁用偏差值输入
                m_page->m_networkHexRangeCheckBox->setChecked(false);
                if (m_page->m_networkHexRangeEdit)
                    m_page->m_networkHexRangeEdit->setEnabled(false);
            }
        }
    });

    // HEX 区间判断勾选 → 联动偏差值输入框
    if (m_page->m_networkHexRangeCheckBox && m_page->m_networkHexRangeEdit) {
        connect(m_page->m_networkHexRangeCheckBox, &ElaCheckBox::toggled,
                m_page, [this](bool checked) {
            m_page->m_networkHexRangeEdit->setEnabled(checked);
        });
    }

    // 产品系列切换 → 更新 An30Layout
    connect(m_page->m_networkProductComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        An30Layout::instance().setProduct(
            index == 0 ? An30Product::RGL : An30Product::EVH);
    });

    // ── ⑤ 后缀选择变更 → 同步到工作线程 ──
    connect(m_page->m_suffixComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            m_page, [this](int index) {
        QMetaObject::invokeMethod(m_page->m_networkWork, "setSuffixMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, index));
    });

    // ── ⑥ 网络连接成功 ──
    connect(m_page->m_networkWork, &NetworkWork::networkConnected, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        m_page->m_openNetworkButton->setEnabled(false);
        m_page->m_closeNetworkButton->setEnabled(true);
        m_page->m_ipAddressEdit->setEnabled(false);
        m_page->m_portEdit->setEnabled(false);
        m_page->m_singleSendBtn->setEnabled(true);
        m_page->m_excelOpenBtn->setEnabled(true);

        bool hasData = (m_page->m_excelTableWidget->rowCount() > 0);
        m_page->m_excelCaptureBtn->setEnabled(hasData);
        m_page->m_excelSendBtn->setEnabled(hasData);

        LED::setLED(m_page->m_networkLED, 2, 16);
    });

    // ── ⑦ 网络断开 ──
    connect(m_page->m_networkWork, &NetworkWork::networkDisconnected, m_page, [this]() {
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        m_page->m_openNetworkButton->setEnabled(true);
        m_page->m_closeNetworkButton->setEnabled(false);
        m_page->m_ipAddressEdit->setEnabled(true);
        m_page->m_portEdit->setEnabled(true);
        m_page->m_singleSendBtn->setEnabled(false);
        m_page->m_excelOpenBtn->setEnabled(false);
        m_page->m_excelCaptureBtn->setEnabled(false);
        m_page->m_excelSendBtn->setEnabled(false);

        LED::setLED(m_page->m_networkLED, 0, 16);
    });

    // ── ⑧ 错误提示（静默）──
    connect(m_page->m_networkWork, &NetworkWork::errorOccurred, m_page, [this](const QString &msg) {
        bool wasConnecting = m_page->m_isConnecting;
        m_page->m_isConnecting = false;
        if (m_page->m_connectTimeoutTimer)
            m_page->m_connectTimeoutTimer->stop();

        if (wasConnecting) {
            qDebug() << "NetworkPage: 连接阶段错误 -" << msg;
        } else {
            qDebug() << "NetworkPage: 非连接阶段错误 -" << msg;
        }
    });

    // ── ⑨ 发送日志行 ──
    connect(m_page->m_networkWork, &NetworkWork::sendLogLine, m_page, [this](const QString &line) {
        if (m_page->m_logPaused) return;
        if (m_page->m_singleSendLog) {
            m_page->m_singleSendLog->addItem(line);
            while (m_page->m_singleSendLog->count() > 100)
                delete m_page->m_singleSendLog->takeItem(0);
        }
        if (m_page->m_logSendList) {
            m_page->m_logSendList->addItem(line);
            while (m_page->m_logSendList->count() > 100)
                delete m_page->m_logSendList->takeItem(0);
        }
    });

    // ── ⑩ 接收日志行 ──
    connect(m_page->m_networkWork, &NetworkWork::recvLogLine, m_page, [this](const QString &line) {
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
    connect(m_page->m_networkWork, &NetworkWork::recvCountChanged, m_page, [this](int count) {
        if (m_page->m_logRecvCountCard)
            m_page->m_logRecvCountCard->setValue(QString::number(count));
    });

    // ── ⑫ 清空按钮 ──
    connect(m_page->m_logClearBtn, &ElaPushButton::clicked, m_page, [this]() {
        if (m_page->m_singleSendLog)   m_page->m_singleSendLog->clear();
        if (m_page->m_singleRecvLog)   m_page->m_singleRecvLog->clear();
        if (m_page->m_logSendList)     m_page->m_logSendList->clear();
        if (m_page->m_logRecvList)     m_page->m_logRecvList->clear();
        QMetaObject::invokeMethod(m_page->m_networkWork, "resetRecvCount",
                                  Qt::QueuedConnection);
    });

    // ── ⑬ 暂停/恢复日志按钮 ──
    connect(m_page->m_logPauseBtn, &ElaPushButton::clicked, m_page, [this]() {
        m_page->m_logPaused = !m_page->m_logPaused;
        if (m_page->m_logPaused) {
            m_page->m_logPauseBtn->setText("恢复日志");
            LED::setLED(m_page->m_logLED, 0, 14);
            qDebug() << "NetworkPage: 日志更新已暂停";
        } else {
            m_page->m_logPauseBtn->setText("暂停日志");
            LED::setLED(m_page->m_logLED, 2, 14);
            qDebug() << "NetworkPage: 日志更新已恢复";
        }
    });
}
