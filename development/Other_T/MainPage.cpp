//
// Created by Cossiant on 2026/6/18.
//

#include "MainPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

MainPage::MainPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow) {
    initMainPage();
    initNavigation();
    initWindowConfig();
}

MainPage::~MainPage() = default;

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void MainPage::initMainPage() {
    createHomePage();
    createHelpPage();
    createAboutPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
//  "首页" 置顶，"帮助文档""关于软件" 置底
// ═══════════════════════════════════════════════════════════════
void MainPage::initNavigation() {
    // ────── 顶部：首页 ──────
    m_mainWindow->addPageNode("首页", _MainHomePage, ElaIconType::House);

    // ────── 底部：帮助文档 / 关于软件 ──────
    m_mainWindow->addFooterNode("帮助文档", _MainHelpPage, MainHelpKey, 0,ElaIconType::CircleQuestion);
    m_mainWindow->addFooterNode("关于软件", _MainAboutPage, MainAboutKey,0,ElaIconType::CircleInfo);
}

// ═══════════════════════════════════════════════════════════════
//  初始化：窗口外观配置（写在 MainPage 中，其他模块不再重复）
// ═══════════════════════════════════════════════════════════════
void MainPage::initWindowConfig() {
    m_mainWindow->resize(1200, 750);
    m_mainWindow->setWindowTitle("Ainuo 通用通讯可靠性测试软件V3.2.1");

    // 用户信息卡片
    m_mainWindow->setUserInfoCardTitle("Ainuo 通讯可靠性");
    m_mainWindow->setUserInfoCardSubTitle("Excel SCPI Sender");
    m_mainWindow->setUserInfoCardVisible(true);

    // 导航栏外观
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

// ═══════════════════════════════════════════════════════════════
//  页面创建
// ═══════════════════════════════════════════════════════════════
void MainPage::createHomePage() {
    _MainHomePage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainHomePage);
    lay->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText("欢迎使用 Ainuo 通讯可靠性测试软件");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    lay->addSpacing(16);

    ElaText *desc = new ElaText(
        "本软件支持串口、网口、CAN、GPIB 四种通讯方式，\n"
        "可通过 Excel 批量导入指令并自动发送、校验响应。\n\n"
        "请从左侧导航栏选择通讯方式开始使用。"
    );
    desc->setTextPixelSize(15);
    lay->addWidget(desc);
    lay->addStretch();
}

void MainPage::createHelpPage() {
    _MainHelpPage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainHelpPage);
    lay->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText("帮助文档");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);
    lay->addStretch();
}

void MainPage::createAboutPage() {
    _MainAboutPage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainAboutPage);
    lay->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText("Ainuo 通用通讯可靠性测试软件V3.2.1");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);

    ElaText *info = new ElaText(
        "版本: v3.2.1\n"
        "作者: Cossiant\n\n"
        "基于 ElaWidgetTools 现代化 UI 框架\n"
        "基于 QXlsx 高性能 Excel 读写框架\n"
        "支持串口 / 网口 / CAN / GPIB 通讯\n"
        "支持 Excel 命令批量发送与返回值验证"
    );
    info->setTextPixelSize(14);

    lay->addWidget(title);
    lay->addSpacing(12);
    lay->addWidget(info);
    lay->addStretch();
}
