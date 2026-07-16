// SerialPage.cpp
// 页面协调器：组装子模块 + 串联初始化
//
// Created by Cossiant on 2026/6/18.

#include "SerialPage.h"
#include "SerialPageUI.h"
#include "SerialPageSignals.h"
#include "SerialErrorHandler.h"
#include "SerialWork.h"
#include "SerialExcel.h"
#include "ElaWindow.h"

SerialPage::SerialPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    // ① 创建 UI
    m_ui = new SerialPageUI(this);
    m_ui->createSettingsPage();
    m_ui->createSendPage();
    m_ui->createExcelSendPage();
    m_ui->createLogPage();
    m_ui->createErrorLogPage();

    // ② 注册导航 + 窗口配置
    initNavigation();
    initWindowConfig();

    // ③ 线程 + 信号连接
    m_signals = new SerialPageSignals(this);

    // ④ 错误处理器
    m_errors = new SerialErrorHandler(this);

    // ⑤ Excel 批量发送
    m_serialFunc = new SerialExcel(this, this);
}

SerialPage::~SerialPage()
{
    if (m_serialWork) {
        QMetaObject::invokeMethod(m_serialWork, "closeSerialPort",
                                  Qt::QueuedConnection);
    }

    if (m_serialThread) {
        m_serialThread->quit();
        if (!m_serialThread->wait(3000)) {
            qWarning() << "SerialPage: 工作线程未能在 3 秒内退出，强制终止";
            m_serialThread->terminate();
            m_serialThread->wait();
        }
    }
}

void SerialPage::initNavigation()
{
    m_mainWindow->addExpanderNode("串口通讯", SerialMainPageKey, ElaIconType::Plug);

    m_mainWindow->addPageNode("串口设置", _SerialSettingPage,   SerialMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _SerialSendPage,      SerialMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _SerialExcelSendPage, SerialMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _SerialLogPage,       SerialMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _SerialErrorLogPage,  SerialMainPageKey, ElaIconType::CircleExclamation);
}

void SerialPage::initWindowConfig()
{
    m_mainWindow->setNavigationBarDisplayMode(ElaNavigationType::Auto);
    m_mainWindow->setNavigationBarWidth(300);
    m_mainWindow->setWindowButtonFlags(
        ElaAppBarType::NavigationButtonHint |
        ElaAppBarType::RouteBackButtonHint  |
        ElaAppBarType::StayTopButtonHint    |
        ElaAppBarType::ThemeChangeButtonHint|
        ElaAppBarType::MinimizeButtonHint   |
        ElaAppBarType::MaximizeButtonHint   |
        ElaAppBarType::CloseButtonHint
    );
    m_mainWindow->setIsAllowPageOpenInNewWindow(false);
    m_mainWindow->moveToCenter();
}

// ═══════════════════════════════════════════════ 错误记录（委托）═══
void SerialPage::addTimeoutError(const QString &command, const QByteArray &expected)
    { m_errors->addTimeoutError(command, expected); }

void SerialPage::addContentError(const QString &command,
                                  const QByteArray &expected,
                                  const QByteArray &actual)
    { m_errors->addContentError(command, expected, actual); }

void SerialPage::clearErrors()
    { m_errors->clearErrors(); }

void SerialPage::clearSingleSendLog()
{
    if (m_singleSendLog) m_singleSendLog->clear();
    if (m_singleRecvLog) m_singleRecvLog->clear();
}

void SerialPage::clearExcelSendLog()
{
    if (m_logSendList)       m_logSendList->clear();
    if (m_logRecvList)       m_logRecvList->clear();
    if (m_logSentCountCard)  m_logSentCountCard->setValue("0");
    if (m_logRecvCountCard)  m_logRecvCountCard->setValue("0");
    if (m_logStartTimeCard)  m_logStartTimeCard->setValue("--:--:--");
    if (m_serialWork)
        m_serialWork->resetRecvCount();
}
