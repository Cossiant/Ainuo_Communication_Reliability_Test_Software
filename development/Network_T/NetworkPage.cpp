//
// Created by Cossiant on 2026/6/18.
//

#include "NetworkPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

NetworkPage::NetworkPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent),
      m_mainWindow(mainWindow) {
    initNetworkPage();
    initNavigation();
    initwindowConfig();
}

NetworkPage::~NetworkPage() = default;

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void NetworkPage::initNetworkPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
// ═══════════════════════════════════════════════════════════════
void NetworkPage::initNavigation()
{
    m_mainWindow->addExpanderNode("网口通讯", NetworkMainPageKey, ElaIconType::NetworkWired);

    m_mainWindow->addPageNode("网络设置", _NetworkSettingPage,   NetworkMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _NetworkSendPage,      NetworkMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _NetworkExcelSendPage, NetworkMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _NetworkLogPage,       NetworkMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _NetworkErrorLogPage,  NetworkMainPageKey, ElaIconType::CircleExclamation);
}

// ═══════════════════════════════════════════════════════════════
//  初始化：窗口外观配置
// ═══════════════════════════════════════════════════════════════
void NetworkPage::initwindowConfig() {
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

// ═══════════════════════════════════════════════════════════════
//  页面创建
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createSettingsPage() {
    _NetworkSettingPage = new QWidget();
    QVBoxLayout *_NetworkSettingLayout = new QVBoxLayout(_NetworkSettingPage);
    _NetworkSettingLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkSettingTitle = new ElaText("网络设置界面");
    _NetworkSettingTitle->setTextPixelSize(24);
    _NetworkSettingTitle->setTextStyle(ElaTextType::Title);

    _NetworkSettingLayout->addWidget(_NetworkSettingTitle);
    _NetworkSettingLayout->addStretch();
}

void NetworkPage::createSendPage() {
    _NetworkSendPage = new QWidget();
    QVBoxLayout *_NetworkSendLayout = new QVBoxLayout(_NetworkSendPage);
    _NetworkSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkSendTitle = new ElaText("网口单条发送界面");
    _NetworkSendTitle->setTextPixelSize(24);
    _NetworkSendTitle->setTextStyle(ElaTextType::Title);

    _NetworkSendLayout->addWidget(_NetworkSendTitle);
    _NetworkSendLayout->addStretch();
}

void NetworkPage::createExcelSendPage() {
    _NetworkExcelSendPage = new QWidget();
    QVBoxLayout *_NetworkExcelSendLayout = new QVBoxLayout(_NetworkExcelSendPage);
    _NetworkExcelSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkExcelSendTitle = new ElaText("网口Excel发送界面");
    _NetworkExcelSendTitle->setTextPixelSize(24);
    _NetworkExcelSendTitle->setTextStyle(ElaTextType::Title);

    _NetworkExcelSendLayout->addWidget(_NetworkExcelSendTitle);
    _NetworkExcelSendLayout->addStretch();
}

void NetworkPage::createLogPage() {
    _NetworkLogPage = new QWidget();
    QVBoxLayout *_NetworkLogLayout = new QVBoxLayout(_NetworkLogPage);
    _NetworkLogLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkLogTitle = new ElaText("网口发送日志界面");
    _NetworkLogTitle->setTextPixelSize(24);
    _NetworkLogTitle->setTextStyle(ElaTextType::Title);

    _NetworkLogLayout->addWidget(_NetworkLogTitle);
    _NetworkLogLayout->addStretch();
}

void NetworkPage::createErrorLogPage() {
    _NetworkErrorLogPage = new QWidget();
    QVBoxLayout *_NetworkErrorLayout = new QVBoxLayout(_NetworkErrorLogPage);
    _NetworkErrorLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkErrorLogTitle = new ElaText("网口错误统计界面");
    _NetworkErrorLogTitle->setTextPixelSize(24);
    _NetworkErrorLogTitle->setTextStyle(ElaTextType::Title);

    _NetworkErrorLayout->addWidget(_NetworkErrorLogTitle);
    _NetworkErrorLayout->addStretch();
}
