//
// Created by Cossiant on 2026/6/18.
//

#include "USERPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

USERPage::USERPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    initUSERPage();
    initNavigation();
    initwindowConfig();
}

USERPage::~USERPage() = default;

void USERPage::initUSERPage() {
    createGuidePage();
    createVersionPage();
}

void USERPage::initNavigation()
{
    m_mainWindow->addExpanderNode("用户案例", USERMainPageKey, ElaIconType::BookOpen);

    m_mainWindow->addPageNode("用户案例1", _USERGuidePage,   USERMainPageKey, ElaIconType::BookUser);
    m_mainWindow->addPageNode("用户案例2", _USERVersionPage, USERMainPageKey, ElaIconType::BookUser);
}

void USERPage::initwindowConfig() {
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

void USERPage::createGuidePage() {
    _USERGuidePage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_USERGuidePage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("用户案例1");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void USERPage::createVersionPage() {
    _USERVersionPage = new QWidget();
    QVBoxLayout* lay = new QVBoxLayout(_USERVersionPage);
    lay->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("用户案例2");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}
