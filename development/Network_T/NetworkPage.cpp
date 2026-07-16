// NetworkPage.cpp
// 页面协调器：组装子模块 + 串联初始化

#include "NetworkPage.h"
#include "NetworkPageUI.h"
#include "NetworkPageSignals.h"
#include "NetworkErrorHandler.h"
#include "NetworkWork.h"
#include "NetworkExcel.h"
#include "ElaWindow.h"

NetworkPage::NetworkPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    // ① 创建 UI
    m_ui = new NetworkPageUI(this);
    m_ui->createSettingsPage();
    m_ui->createSendPage();
    m_ui->createExcelSendPage();
    m_ui->createLogPage();
    m_ui->createErrorLogPage();

    // ② 注册导航 + 窗口配置
    initNavigation();
    initWindowConfig();

    // ③ 线程 + 信号连接
    m_signals = new NetworkPageSignals(this);

    // ④ 错误处理器
    m_errors = new NetworkErrorHandler(this);

    // ⑤ Excel 批量发送
    m_networkFunc = new NetworkExcel(this, this);
}

NetworkPage::~NetworkPage()
{
    m_isConnecting = false;
    if (m_connectTimeoutTimer)
        m_connectTimeoutTimer->stop();

    if (m_networkWork) {
        QMetaObject::invokeMethod(m_networkWork, "disconnectFromHost",
                                  Qt::QueuedConnection);
    }

    if (m_networkThread) {
        m_networkThread->quit();
        if (!m_networkThread->wait(3000)) {
            qWarning() << "NetworkPage: 工作线程未能在 3 秒内退出，强制终止";
            m_networkThread->terminate();
            m_networkThread->wait();
        }
    }
}

void NetworkPage::initNavigation()
{
    m_mainWindow->addExpanderNode("网口通讯", NetworkMainPageKey, ElaIconType::NetworkWired);

    m_mainWindow->addPageNode("网络设置", _NetworkSettingPage,   NetworkMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _NetworkSendPage,      NetworkMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _NetworkExcelSendPage, NetworkMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _NetworkLogPage,       NetworkMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _NetworkErrorLogPage,  NetworkMainPageKey, ElaIconType::CircleExclamation);
}

void NetworkPage::initWindowConfig()
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
void NetworkPage::addTimeoutError(const QString &command, const QByteArray &expected)
    { m_errors->addTimeoutError(command, expected); }

void NetworkPage::addContentError(const QString &command,
                                   const QByteArray &expected,
                                   const QByteArray &actual)
    { m_errors->addContentError(command, expected, actual); }

void NetworkPage::clearErrors()
    { m_errors->clearErrors(); }

void NetworkPage::clearSingleSendLog()
{
    if (m_singleSendLog) m_singleSendLog->clear();
    if (m_singleRecvLog) m_singleRecvLog->clear();
}

void NetworkPage::clearExcelSendLog()
{
    if (m_logSendList)   m_logSendList->clear();
    if (m_logRecvList)   m_logRecvList->clear();
    if (m_logSentCountCard) m_logSentCountCard->setValue("0");
    if (m_logRecvCountCard) m_logRecvCountCard->setValue("0");
    if (m_logStartTimeCard) m_logStartTimeCard->setValue("--:--:--");
    if (m_networkWork)
        m_networkWork->resetRecvCount();
}
