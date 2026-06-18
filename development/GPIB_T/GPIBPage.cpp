//
// Created by Cossiant on 2026/6/18.
//

#include "GPIBPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

GPIBPage::GPIBPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    initGPIBPage();
    initNavigation();
    initwindowConfig();
}

GPIBPage::~GPIBPage() = default;

void GPIBPage::initGPIBPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

void GPIBPage::initNavigation()
{
    m_mainWindow->addExpanderNode("GPIB通讯", GPIBMainPageKey, ElaIconType::Gauge);

    m_mainWindow->addPageNode("GPIB设置", _GPIBSettingPage,   GPIBMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _GPIBSendPage,      GPIBMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _GPIBExcelSendPage, GPIBMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _GPIBLogPage,       GPIBMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _GPIBErrorLogPage,  GPIBMainPageKey, ElaIconType::CircleExclamation);
}

void GPIBPage::initwindowConfig() {
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

void GPIBPage::createSettingsPage() {
    _GPIBSettingPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_GPIBSettingPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("GPIB设置界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void GPIBPage::createSendPage() {
    _GPIBSendPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_GPIBSendPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("GPIB单条发送界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void GPIBPage::createExcelSendPage() {
    _GPIBExcelSendPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_GPIBExcelSendPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("GPIB表格发送界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void GPIBPage::createLogPage() {
    _GPIBLogPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_GPIBLogPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("GPIB发送日志界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void GPIBPage::createErrorLogPage() {
    _GPIBErrorLogPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_GPIBErrorLogPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("GPIB错误统计界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}
