//
// Created by Cossiant on 2026/6/18.
//

#include "SerialPage.h"
#include "ElaWindow.h"       // 需要完整定义来调用 addExpanderNode / addPageNode
#include "ElaText.h"
#include "ElaIcon.h"

SerialPage::SerialPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent),
      m_mainWindow(mainWindow) {
    initSerialPage();
    initNavigation();
    initwindowConfig();
}

SerialPage::~SerialPage() = default;

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void SerialPage::initSerialPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
// ═══════════════════════════════════════════════════════════════
void SerialPage::initNavigation()
{
    m_mainWindow->addExpanderNode("串口通讯", SerialMainPageKey, ElaIconType::Plug);

    m_mainWindow->addPageNode("串口设置", _SerialSettingPage,   SerialMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _SerialSendPage,      SerialMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _SerialExcelSendPage, SerialMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _SerialLogPage,       SerialMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _SerialErrorLogPage,  SerialMainPageKey, ElaIconType::CircleExclamation);
}


// ═══════════════════════════════════════════════════════════════
//  初始化：窗口外观配置
// ═══════════════════════════════════════════════════════════════
void SerialPage::initwindowConfig() {
    m_mainWindow->setNavigationBarDisplayMode(ElaNavigationType::Auto);
    m_mainWindow->setNavigationBarWidth(300);
    m_mainWindow->setWindowButtonFlags(
        ElaAppBarType::NavigationButtonHint |
        ElaAppBarType::RouteBackButtonHint |
        ElaAppBarType::StayTopButtonHint |
        ElaAppBarType::ThemeChangeButtonHint |
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint
    );
    m_mainWindow->setIsAllowPageOpenInNewWindow(false);
    m_mainWindow->moveToCenter();
}

// void SerialPage::createMainPage() {
//     _SerialMainPage = new QWidget();
//     QVBoxLayout *_SerialMainLayout = new QVBoxLayout(_SerialMainPage);
//     _SerialMainLayout->setContentsMargins(30, 30, 30, 30);
//
//     ElaText *_SerialMainTitle = new ElaText("串口主界面");
//     _SerialMainTitle->setTextPixelSize(24);
//     _SerialMainTitle->setTextStyle(ElaTextType::Title);
//
//     _SerialMainLayout->addWidget(_SerialMainTitle);
// }

void SerialPage::createSettingsPage() {
    _SerialSettingPage = new QWidget();
    QVBoxLayout *_SerialSettingLayout = new QVBoxLayout(_SerialSettingPage);
    _SerialSettingLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialSettingTitle = new ElaText("串口设置界面");
    _SerialSettingTitle->setTextPixelSize(24);
    _SerialSettingTitle->setTextStyle(ElaTextType::Title);

    _SerialSettingLayout->addWidget(_SerialSettingTitle);
    _SerialSettingLayout->addStretch();
}

void SerialPage::createSendPage() {
    _SerialSendPage = new QWidget();
    QVBoxLayout *_SerialSendLayout = new QVBoxLayout(_SerialSendPage);
    _SerialSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialSendTitle = new ElaText("串口单条发送界面");
    _SerialSendTitle->setTextPixelSize(24);
    _SerialSendTitle->setTextStyle(ElaTextType::Title);

    _SerialSendLayout->addWidget(_SerialSendTitle);
    _SerialSendLayout->addStretch();
}

void SerialPage::createExcelSendPage() {
    _SerialExcelSendPage = new QWidget();
    QVBoxLayout *_SerialExcelSendLayout = new QVBoxLayout(_SerialExcelSendPage);
    _SerialExcelSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialExcelSendTitle = new ElaText("串口excel发送界面");
    _SerialExcelSendTitle->setTextPixelSize(24);
    _SerialExcelSendTitle->setTextStyle(ElaTextType::Title);

    _SerialExcelSendLayout->addWidget(_SerialExcelSendTitle);
    _SerialExcelSendLayout->addStretch();
}

void SerialPage::createLogPage() {
    _SerialLogPage = new QWidget();
    QVBoxLayout *_SerialLogLayout = new QVBoxLayout(_SerialLogPage);
    _SerialLogLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialLogTitle = new ElaText("串口日志界面");
    _SerialLogTitle->setTextPixelSize(24);
    _SerialLogTitle->setTextStyle(ElaTextType::Title);

    _SerialLogLayout->addWidget(_SerialLogTitle);
    _SerialLogLayout->addStretch();
}

void SerialPage::createErrorLogPage() {
    _SerialErrorLogPage = new QWidget();
    QVBoxLayout *_SerialErrorLayout = new QVBoxLayout(_SerialErrorLogPage);
    _SerialErrorLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialErrorLogTitle = new ElaText("串口错误日志界面");
    _SerialErrorLogTitle->setTextPixelSize(24);
    _SerialErrorLogTitle->setTextStyle(ElaTextType::Title);

    _SerialErrorLayout->addWidget(_SerialErrorLogTitle);
    _SerialErrorLayout->addStretch();
}
