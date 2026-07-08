//
// Created by Cossiant on 2026/6/18.
//

#include "CANPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

CANPage::CANPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    initCANPage();
    initNavigation();
    initwindowConfig();
}

CANPage::~CANPage() = default;

void CANPage::initCANPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

void CANPage::initNavigation()
{
    m_mainWindow->addExpanderNode("CAN通讯", CANMainPageKey, ElaIconType::CarBus);

    m_mainWindow->addPageNode("CAN设置", _CANSettingPage,   CANMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _CANSendPage,      CANMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _CANExcelSendPage, CANMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _CANLogPage,       CANMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _CANErrorLogPage,  CANMainPageKey, ElaIconType::CircleExclamation);
}

void CANPage::initwindowConfig() {
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

void CANPage::createSettingsPage() {
    _CANSettingPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_CANSettingPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("CAN设置界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void CANPage::createSendPage() {
    _CANSendPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_CANSendPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("CAN单条发送界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void CANPage::createExcelSendPage() {
    _CANExcelSendPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_CANExcelSendPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("CAN表格发送界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void CANPage::createLogPage() {
    _CANLogPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_CANLogPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("CAN发送日志界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void CANPage::createErrorLogPage() {
    _CANErrorLogPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_CANErrorLogPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("CAN错误统计界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}
