//
// Created by Cossiant on 2026/6/18.
//

#include "MainPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>

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
    createCommandCalculationPage();
    createHelpPage();
    createAboutPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
// ═══════════════════════════════════════════════════════════════
void MainPage::initNavigation() {
    // ────── 顶部：首页 ──────
    m_mainWindow->addPageNode("首页", _MainHomePage, ElaIconType::House);
    m_mainWindow->addPageNode("命令转换",_MainCommandCalculationPage, ElaIconType::Command);
    // ────── 底部：帮助文档 / 关于软件 ──────
    m_mainWindow->addFooterNode("帮助文档", _MainHelpPage,  MainHelpKey,  0, ElaIconType::CircleQuestion);
    m_mainWindow->addFooterNode("关于软件", _MainAboutPage, MainAboutKey, 0, ElaIconType::CircleInfo);
}

// ═══════════════════════════════════════════════════════════════
//  初始化：窗口外观配置
// ═══════════════════════════════════════════════════════════════
void MainPage::initWindowConfig() {
    m_mainWindow->resize(1200, 750);
    m_mainWindow->setWindowTitle("Ainuo 通用通讯可靠性测试软件V3.4.3");

    // 用户信息卡片
    m_mainWindow->setUserInfoCardTitle("Ainuo 通讯可靠性");
    m_mainWindow->setUserInfoCardSubTitle("Excel SCPI Sender");
    m_mainWindow->setUserInfoCardVisible(true);


    // 导航栏外观
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
//  首页
// ═══════════════════════════════════════════════════════════════
void MainPage::createHomePage() {
    _MainHomePage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainHomePage);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(20);

    // ──── 标题 ────
    ElaText *title = new ElaText("欢迎使用 Ainuo 通讯可靠性测试软件");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    // ──── 简介 ────
    ElaText *desc = new ElaText(
        "本软件支持串口、网口、CAN、GPIB 四种通讯方式，\n"
        "可通过 Excel 表格批量导入指令并自动发送、校验响应。\n\n"
        "请从左侧导航栏选择通讯方式开始使用。"
    );
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    lay->addWidget(desc);

    // ──── 分隔线 ────
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("QFrame { color: rgba(128,128,128,60); }");
    lay->addWidget(line);

    // ──── 快速开始 ────
    ElaText *quickTitle = new ElaText("快速开始");
    quickTitle->setTextPixelSize(20);
    quickTitle->setTextStyle(ElaTextType::Subtitle);
    lay->addWidget(quickTitle);

    // 步骤（使用 「」 避免中文引号与 C++ 字符串冲突）
    QStringList steps = {
        QString::fromUtf8("①  在左侧导航栏展开「网口通讯」或「串口通讯」"),
        QString::fromUtf8("②  在「网络设置 / 串口设置」中配置参数并点击连接"),
        QString::fromUtf8("③  在「表格发送」页中点击「打开 Excel 并读取」加载命令表"),
        QString::fromUtf8("④  设置发送次数和超时时间，点击「开始发送」"),
        QString::fromUtf8("⑤  在「发送日志」页中实时查看收发数据"),
        QString::fromUtf8("⑥  在「错误统计」页中查看发送结果汇总")
    };

    for (const QString &step : steps) {
        ElaText *stepLabel = new ElaText(step);
        stepLabel->setTextPixelSize(14);
        stepLabel->setWordWrap(true);
        lay->addWidget(stepLabel);
    }

    lay->addStretch();
}
void MainPage::createCommandCalculationPage() {
    _MainCommandCalculationPage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainCommandCalculationPage);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(20);

    // ──── 标题 ────
    ElaText *title = new ElaText("命令转换页面");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    lay->addStretch();
}

// ═══════════════════════════════════════════════════════════════
//  帮助文档
// ═══════════════════════════════════════════════════════════════
void MainPage::createHelpPage() {
    _MainHelpPage = new QWidget();

    // ──── 可滚动区域 ────
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(scrollContent);
    lay->setContentsMargins(40, 30, 40, 30);
    lay->setSpacing(24);

    // ──── 标题 ────
    ElaText *title = new ElaText("帮助文档");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    ElaText *subtitle = new ElaText(
        QString::fromUtf8("本文档介绍软件的基本使用方法与注意事项。"));
    subtitle->setTextPixelSize(15);
    subtitle->setWordWrap(true);
    lay->addWidget(subtitle);

    // ════════════════ 1. 软件概述 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("1. 软件概述"),
        QStringList{
            QString::fromUtf8("Ainuo 通用通讯可靠性测试软件是一款用于对仪器设备进行自动化通讯测试的工具。"),
            QString::fromUtf8("软件通过 Excel 表格定义测试指令集，自动逐条发送命令并校验设备返回值，"),
            QString::fromUtf8("统计发送成功率、超时和内容错误，帮助快速定位通讯问题。"),
            QString(),
            QString::fromUtf8("支持通讯方式：串口 (RS-232/485)  /  网口 (TCP 客户端)  /  CAN  /  GPIB")
        }));

    // ════════════════ 2. 网口通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("2. 网口通讯"),
        QStringList{
            QString::fromUtf8("2.1  配置网络参数"),
            QString::fromUtf8("    在「网络设置」页中输入目标设备的 IP 地址和端口号。"),
            QString::fromUtf8("    可选勾选「禁用 Nagle 算法」以降低发送延迟（适合小数据包高频发送场景）。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」以十六进制方式编码命令。"),
            QString::fromUtf8("    点击「连接网络」，软件将在 3 秒内尝试建立 TCP 连接。"),
            QString::fromUtf8("    连接成功后 LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("2.2  单条命令发送"),
            QString::fromUtf8("    在「单条发送」页中输入命令文本，点击「通过网口发送」。"),
            QString::fromUtf8("    发送和接收日志实时显示在下方列表中。"),
            QString(),
            QString::fromUtf8("2.3  表格批量发送"),
            QString::fromUtf8("    ① 点击「下载示例模板」获取标准格式的 Excel 文件"),
            QString::fromUtf8("    ② 按模板格式编写命令表（A列=命令，B列=期望返回值，C列=命令间隔ms）"),
            QString::fromUtf8("    ③ 点击「打开 Excel 并读取」加载文件到表格"),
            QString::fromUtf8("    ④ 在「发送次数」中填写循环次数（0=无限循环）"),
            QString::fromUtf8("    ⑤ 在「超时时间」中填写全局超时（超过此时长未收到回复即判定超时）"),
            QString::fromUtf8("    ⑥ 点击「开始发送」启动批量测试"),
            QString::fromUtf8("    ⑦ 点击「读取返回值」会仅发送一次并记录实际返回值（预扫模式）"),
            QString(),
            QString::fromUtf8("2.4  错误统计"),
            QString::fromUtf8("    超时错误：在超时时间内未收到设备回复"),
            QString::fromUtf8("    内容错误：设备回复的内容与 Excel 中定义的期望值不一致"),
            QString::fromUtf8("    错误列表支持自动滚动和清空操作")
        }));

    // ════════════════ 3. 串口通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("3. 串口通讯"),
        QStringList{
            QString::fromUtf8("3.1  配置串口参数"),
            QString::fromUtf8("    在「串口设置」中选择端口号、波特率、数据位、停止位、校验位。"),
            QString::fromUtf8("    可选勾选「合并串口接收数据(20ms超时)」以避免数据帧被拆分显示。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」切换十六进制模式。"),
            QString::fromUtf8("    点击「打开串口」建立连接，LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("3.2  单条命令与表格批量发送"),
            QString::fromUtf8("    操作方式与网口通讯一致，请参考第 2 节的说明。")
        }));

    // <<< 修改：GPIB 已完整实现，替换预留接口说明为完整帮助文档 >>>
    // ════════════════ 4. GPIB 通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("4. GPIB 通讯"),
        QStringList{
            QString::fromUtf8("4.1  配置 GPIB 参数"),
            QString::fromUtf8("    在「GPIB设置」页中输入板卡号（通常为 0）、仪器主地址（0-30）。"),
            QString::fromUtf8("    如需使用副地址（某些多通道仪器），可填入副地址（0=不使用）。"),
            QString::fromUtf8("    设置超时时间（建议 3000ms，可根据仪器响应速度调整）。"),
            QString::fromUtf8("    结束字符默认为换行符 \\n，启用后 viRead 会在收到换行符时终止读取。"),
            QString::fromUtf8("    可选勾选「发送时附加 EOI 信号」，大多数 GPIB 仪器需要此信号标识命令结束。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」切换十六进制模式。"),
            QString::fromUtf8("    点击「打开 GPIB」建立连接，LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("4.2  前置条件"),
            QString::fromUtf8("    a) 必须安装 NI-VISA 运行时驱动（NI-488.2 或 NI-VISA 独立包）。"),
            QString::fromUtf8("    b) GPIB 控制器（如 NI GPIB-USB-HS）需正确插入并被系统识别。"),
            QString::fromUtf8("    c) 可在 NI-MAX（Measurement & Automation Explorer）中验证设备可见性。"),
            QString::fromUtf8("    d) 确认仪器 GPIB 地址与软件中填写的主地址一致。"),
            QString(),
            QString::fromUtf8("4.3  单条命令发送"),
            QString::fromUtf8("    在「单条发送」页中输入 SCPI 命令文本，点击「通过 GPIB 发送」。"),
            QString::fromUtf8("    发送和接收日志实时显示在下方列表中。"),
            QString(),
            QString::fromUtf8("4.4  表格批量发送"),
            QString::fromUtf8("    操作方式与网口通讯一致，请参考第 2.3 节的说明。"),
            QString::fromUtf8("    支持「读取返回值（捕获）」模式，自动将仪器返回值填入 Excel B 列。"),
            QString(),
            QString::fromUtf8("4.5  错误统计"),
            QString::fromUtf8("    超时错误：在超时时间内未收到仪器回复"),
            QString::fromUtf8("    内容错误：仪器回复的内容与 Excel 中定义的期望值不一致"),
            QString::fromUtf8("    错误列表支持自动滚动和清空操作"),
            QString(),
            QString::fromUtf8("4.6  常见 GPIB 问题"),
            QString::fromUtf8("    Q: 打开 GPIB 时提示「未找到目标资源」？"),
            QString::fromUtf8("    A: 检查板卡号和主地址是否正确，仪器是否上电，NI-MAX 中是否可见该设备。"),
            QString(),
            QString::fromUtf8("    Q: 打开 GPIB 时提示「未找到 VISA 运行库」？"),
            QString::fromUtf8("    A: 请确认 NI-VISA 已正确安装。可从 NI 官网下载 NI-VISA 运行时（免费）。"),
            QString(),
            QString::fromUtf8("    Q: GPIB 连接超时（5秒弹窗）？"),
            QString::fromUtf8("    A: 检查 GPIB 线缆连接是否牢固，仪器是否上电且处于远程控制模式。")
        }));

    // ════════════════ 5. CAN 通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("5. CAN 通讯"),
        QStringList{
            QString::fromUtf8("CAN 通讯模块目前为预留接口，具体功能将在后续版本中完善。"),
            QString::fromUtf8("如需使用请联系开发者获取技术支持。")
        }));

    // ════════════════ 6. Excel 模板格式 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("6. Excel 模板格式"),
        QStringList{
            QString::fromUtf8("Excel 文件必须包含表头行（第 1 行），数据从第 2 行开始。"),
            QString::fromUtf8("表格结构："),
            QString::fromUtf8("  第 1 列 (A) — 发送的命令          (必需)"),
            QString::fromUtf8("  第 2 列 (B) — 正确的返回值        (可选，留空=不校验)"),
            QString::fromUtf8("  第 3 列 (C) — 到下一条命令的时间ms  (可选，默认 100ms)"),
            QString(),
            QString::fromUtf8("命令文本说明："),
            QString::fromUtf8("  普通模式：直接输入 ASCII 命令，如  *IDN?"),
            QString::fromUtf8("  HEX 模式：输入十六进制字符串（空格分隔），如  2A 49 44 4E 3F"),
            QString::fromUtf8("            勾选「以HEX格式发送」后生效"),
            QString(),
            QString::fromUtf8("期望返回值说明："),
            QString::fromUtf8("  普通模式：直接输入期望的 ASCII 文本"),
            QString::fromUtf8("  HEX 模式：输入期望的十六进制字符串"),
            QString::fromUtf8("  留空则不对该命令的返回值进行校验")
        }));

    // ════════════════ 7. 常见问题 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("7. 常见问题"),
        QStringList{
            QString::fromUtf8("Q: 连接网络超时（3秒弹窗）？"),
            QString::fromUtf8("A: 请检查 IP 地址和端口号是否正确，目标设备是否在线，防火墙是否阻止了连接。"),
            QString(),
            QString::fromUtf8("Q: 串口列表中没有可用端口？"),
            QString::fromUtf8("A: 请检查设备驱动是否安装正确，设备是否已插入，可在 Windows 设备管理器中确认。"),
            QString(),
            QString::fromUtf8("Q: 发送命令后未收到回复？"),
            QString::fromUtf8("A: 检查连接状态 LED 是否为绿色；检查命令格式是否正确；适当增大全局超时时间。"),
            QString(),
            QString::fromUtf8("Q: Excel 文件读取失败？"),
            QString::fromUtf8("A: 请确保使用 .xlsx 格式，且表头和数据格式符合模板规范。可先下载示例模板参考。"),
            QString(),
            QString::fromUtf8("Q: Nagle 算法是什么？是否需要禁用？"),
            QString::fromUtf8("A: Nagle 算法会合并小数据包以提升网络效率。对于 SCPI 指令这种小包高频场景，"),
            QString::fromUtf8("   建议禁用（勾选该选项）以避免延迟。")
        }));

    scrollArea->setWidget(scrollContent);

    QVBoxLayout *outer = new QVBoxLayout(_MainHelpPage);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scrollArea);
}

// ═══════════════════════════════════════════════════════════════
//  关于软件
// ═══════════════════════════════════════════════════════════════
void MainPage::createAboutPage() {
    _MainAboutPage = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(_MainAboutPage);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(16);

    // ──── 软件名称 ────
    ElaText *title = new ElaText("Ainuo 通用通讯可靠性测试软件");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    // ──── 版本 ────
    ElaText *version = new ElaText(QString::fromUtf8("版本: v3.4.3"));
    version->setTextPixelSize(18);
    version->setTextStyle(ElaTextType::Subtitle);
    lay->addWidget(version);

    // ──── 分隔线 ────
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("QFrame { color: rgba(128,128,128,60); }");
    lay->addWidget(line);

    // ──── 详细信息（分组卡片） ────
    QGroupBox *infoGroup = new QGroupBox(QString::fromUtf8("软件信息"));
    infoGroup->setStyleSheet(
        "QGroupBox { font-size: 15px; font-weight: bold; "
        "border: 1px solid rgba(128,128,128,50); border-radius: 8px; "
        "margin-top: 12px; padding-top: 24px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 16px; }"
    );
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setSpacing(8);
    infoLayout->setContentsMargins(20, 20, 20, 20);

    auto addInfo = [&](const QString &label, const QString &value) {
        QHBoxLayout *row = new QHBoxLayout();
        ElaText *lbl = new ElaText(label);
        lbl->setTextPixelSize(14);
        lbl->setMinimumWidth(120);
        lbl->setTextStyle(ElaTextType::BodyStrong);
        ElaText *val = new ElaText(value);
        val->setTextPixelSize(14);
        val->setWordWrap(true);
        row->addWidget(lbl);
        row->addWidget(val, 1);
        infoLayout->addLayout(row);
    };

    addInfo(QString::fromUtf8("软件版本："), "v3.4.3");
    addInfo(QString::fromUtf8("发布日期："), QString::fromUtf8("2026 年 7 月"));
    addInfo(QString::fromUtf8("开发者："),   "Cossiant");
    addInfo(QString::fromUtf8("开发环境："), "Qt 5.15 + MinGW");
    infoLayout->addSpacing(8);
    addInfo(QString::fromUtf8("UI 框架："),  "ElaWidgetTools（现代化 Fluent Design 组件库）");
    addInfo(QString::fromUtf8("Excel 引擎："), "QXlsx（高性能跨平台 Excel 读写库）");
    addInfo(QString::fromUtf8("通讯协议："), "TCP/IP、RS-232/485 (Serial)、CAN、GPIB (NI-VISA)");

    lay->addWidget(infoGroup);

    // ──── 功能特性 ────
    QGroupBox *featureGroup = new QGroupBox(QString::fromUtf8("功能特性"));
    featureGroup->setStyleSheet(infoGroup->styleSheet());
    QVBoxLayout *featureLayout = new QVBoxLayout(featureGroup);
    featureLayout->setSpacing(6);
    featureLayout->setContentsMargins(20, 20, 20, 20);

    QStringList features = {
        QString::fromUtf8("\u2022  支持串口 / 网口 / CAN / GPIB 四种通讯方式"),
        QString::fromUtf8("\u2022  Excel 表格批量导入命令，自动逐条发送"),
        QString::fromUtf8("\u2022  自动校验返回值，统计超时和内容错误"),
        QString::fromUtf8("\u2022  支持 HEX 和 ASCII 两种命令编码格式"),
        QString::fromUtf8("\u2022  独立单条命令发送模式，便于调试"),
        QString::fromUtf8("\u2022  实时收发日志，带毫秒级时间戳"),
        QString::fromUtf8("\u2022  错误统计卡片 + 6列详细错误列表"),
        QString::fromUtf8("\u2022  Nagle 算法可选禁用（网口低延迟模式）"),
        QString::fromUtf8("\u2022  GPIB 仪器地址配置 + EOI / 结束字符控制"),
        QString::fromUtf8("\u2022  命令间隔精确延时控制（1ms 精度，EMA 补偿）"),
        QString::fromUtf8("\u2022  多线程架构，收发与 UI 完全分离")
    };

    for (const QString &feat : features) {
        ElaText *ft = new ElaText(feat);
        ft->setTextPixelSize(14);
        ft->setWordWrap(true);
        featureLayout->addWidget(ft);
    }

    lay->addWidget(featureGroup);

    lay->addStretch();

    // ──── 底部版权 ────
    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("QFrame { color: rgba(128,128,128,40); }");
    lay->addWidget(line2);

    ElaText *copyright = new ElaText(QString::fromUtf8("\u00a9 2026 Cossiant. All rights reserved."));
    copyright->setTextPixelSize(12);
    copyright->setStyleSheet("color: rgba(128,128,128,180);");
    lay->addWidget(copyright);
}

// ═══════════════════════════════════════════════════════════════
//  辅助：创建帮助文档章节
// ═══════════════════════════════════════════════════════════════
QWidget* MainPage::createHelpSection(const QString &title, const QStringList &lines) const
{
    QGroupBox *group = new QGroupBox(title);
    group->setStyleSheet(
        "QGroupBox { font-size: 16px; font-weight: bold; "
        "border: 1px solid rgba(128,128,128,50); border-radius: 8px; "
        "margin-top: 14px; padding-top: 28px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 16px; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(4);
    layout->setContentsMargins(20, 18, 20, 16);

    for (const QString &line : lines) {
        if (line.isEmpty()) {
            layout->addSpacing(6);
            continue;
        }

        // 小标题（以数字或 Q: 开头）
        bool isSubTitle = (line.startsWith("Q:") || line.startsWith("2.") ||
                           line.startsWith("3.") || line.startsWith("4.") ||
                           line.startsWith("5.") || line.startsWith("6.") ||
                           line.startsWith("7."));
        // 子步骤（以空格开头）
        bool isIndent = line.startsWith("    ");

        ElaText *text = new ElaText(line);
        if (isSubTitle) {
            text->setTextPixelSize(15);
            text->setTextStyle(ElaTextType::BodyStrong);
        } else if (isIndent) {
            text->setTextPixelSize(13);
        } else {
            text->setTextPixelSize(14);
        }
        text->setWordWrap(true);

        if (isIndent) {
            text->setContentsMargins(16, 0, 0, 0);
        }

        layout->addWidget(text);
    }

    return group;
}
