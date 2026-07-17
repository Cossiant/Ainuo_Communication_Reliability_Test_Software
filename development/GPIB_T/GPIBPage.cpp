// GPIBPage.cpp
// 页面协调器：组装子模块 + 串联初始化
// 对齐 SerialPage / NetworkPage 架构

#include "GPIBPage.h"
#include "GPIBPageUI.h"
#include "GPIBPageSignals.h"
#include "GPIBErrorHandler.h"
#include "GPIBWork.h"
#include "GPIBExcel.h"
#include "ElaWindow.h"
#include "ElaIcon.h"

GPIBPage::GPIBPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    // ① 创建 UI
    m_ui = new GPIBPageUI(this);
    m_ui->createSettingsPage();
    m_ui->createSendPage();
    m_ui->createExcelSendPage();
    m_ui->createLogPage();
    m_ui->createErrorLogPage();

    // ② 注册导航 + 窗口配置
    initNavigation();
    initWindowConfig();

    // ③ 线程 + 信号连接
    m_signals = new GPIBPageSignals(this);

    // ④ 错误处理器
    m_errors = new GPIBErrorHandler(this);

    // ⑤ Excel 批量发送
    m_gpibFunc = new GPIBExcel(this, this);

    qDebug() << "GPIBPage: 工作线程已启动，ID =" << m_gpibThread;
}

GPIBPage::~GPIBPage()
{
    m_isConnecting = false;
    if (m_connectTimeoutTimer)
        m_connectTimeoutTimer->stop();

    if (m_gpibWork) {
        QMetaObject::invokeMethod(m_gpibWork, "closeGPIBPort",
                                  Qt::QueuedConnection);
    }

    if (m_gpibThread) {
        m_gpibThread->quit();
        if (!m_gpibThread->wait(3000)) {
            qWarning() << "GPIBPage: 工作线程未能在 3 秒内退出，强制终止";
            m_gpibThread->terminate();
            m_gpibThread->wait();
        }
    }
}

void GPIBPage::initNavigation()
{
    m_mainWindow->addExpanderNode("GPIB通讯", GpibMainPageKey, ElaIconType::Microchip);

    m_mainWindow->addPageNode("GPIB设置", _GpibSettingPage,   GpibMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _GpibSendPage,      GpibMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _GpibExcelSendPage, GpibMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _GpibLogPage,       GpibMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _GpibErrorLogPage,  GpibMainPageKey, ElaIconType::CircleExclamation);
}

void GPIBPage::initWindowConfig()
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
void GPIBPage::addTimeoutError(const QString &command, const QByteArray &expected)
    { m_errors->addTimeoutError(command, expected); }

void GPIBPage::addContentError(const QString &command,
                                const QByteArray &expected,
                                const QByteArray &actual)
    { m_errors->addContentError(command, expected, actual); }

void GPIBPage::clearErrors()
    { m_errors->clearErrors(); }

void GPIBPage::clearSingleSendLog()
{
    if (m_singleSendLog) m_singleSendLog->clear();
    if (m_singleRecvLog) m_singleRecvLog->clear();
}

void GPIBPage::clearExcelSendLog()
{
    if (m_logSendList)      m_logSendList->clear();
    if (m_logRecvList)      m_logRecvList->clear();
    if (m_logSentCountCard) m_logSentCountCard->setValue("0");
    if (m_logRecvCountCard) m_logRecvCountCard->setValue("0");
    if (m_logStartTimeCard) m_logStartTimeCard->setValue("--:--:--");
    if (m_gpibWork)
        m_gpibWork->resetRecvCount();
}
