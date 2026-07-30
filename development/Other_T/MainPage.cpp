//
// Created by Cossiant on 2026/6/18.
//

#include "MainPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "ElaIcon.h"
#include "ElaPushButton.h"
#include "ElaCheckBox.h"
#include "ElaComboBox.h"

#include "../Other_T/An30Layout.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>
#include <QTextEdit>
#include <QTextCursor>
#include <QClipboard>
#include <QApplication>

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
    m_mainWindow->setWindowTitle("Ainuo 通用通讯可靠性测试软件V3.4.14");

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

// ═══════════════════════════════════════════════════════════════
//  命令转换页面：ASCII ↔ HEX 双向互转
// ═══════════════════════════════════════════════════════════════
void MainPage::createCommandCalculationPage() {
    _MainCommandCalculationPage = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(_MainCommandCalculationPage);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    // ──── 标题 ────
    ElaText *title = new ElaText("命令转换工具");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    root->addWidget(title);

    ElaText *desc = new ElaText(
        "在 ASCII 字符串与 HEX 十六进制之间互相转换。\n"
        "常用于将 SCPI 命令（如 *IDN?）转换为 HEX 格式下发给设备，或反向解析。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    root->addWidget(desc);

    // ═══════════════════════════════════════════════════════
    //  输入输出区：左右两栏（均可编辑）
    // ═══════════════════════════════════════════════════════
    QHBoxLayout* mainRow = new QHBoxLayout();
    mainRow->setSpacing(20);

    // ── 左栏：ASCII ──
    QVBoxLayout* leftCol = new QVBoxLayout();
    leftCol->setSpacing(8);

    ElaText* asciiLabel = new ElaText("ASCII 字符串");
    asciiLabel->setTextPixelSize(16);
    asciiLabel->setTextStyle(ElaTextType::Subtitle);

    m_asciiEdit = new QTextEdit();
    m_asciiEdit->setPlaceholderText("在此输入 ASCII 字符串...\n例如：*IDN?");
    m_asciiEdit->setMinimumHeight(100);
    m_asciiEdit->setAcceptRichText(false);
    m_asciiEdit->setStyleSheet(
        "QTextEdit {"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 14px;"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 6px;"
        "  padding: 10px;"
        "}"
    );

    leftCol->addWidget(asciiLabel);
    leftCol->addWidget(m_asciiEdit, 1);

    // ── 中间箭头按钮区 ──
    QVBoxLayout* midCol = new QVBoxLayout();
    midCol->setSpacing(12);
    midCol->setAlignment(Qt::AlignCenter);

    midCol->addStretch();

    m_asciiToHexBtn = new ElaPushButton("→ 转为HEX");
    m_asciiToHexBtn->setFixedSize(120, 40);
    m_asciiToHexBtn->setToolTip("将左侧 ASCII 转为 HEX 并填入右侧");

    m_hexToAsciiBtn = new ElaPushButton("← 转为ASCII");
    m_hexToAsciiBtn->setFixedSize(120, 40);
    m_hexToAsciiBtn->setToolTip("将右侧 HEX 转为 ASCII 并填入左侧");

    midCol->addWidget(m_asciiToHexBtn);
    midCol->addWidget(m_hexToAsciiBtn);
    midCol->addStretch();

    // ── 右栏：HEX ──
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(8);

    ElaText* hexLabel = new ElaText("HEX 十六进制");
    hexLabel->setTextPixelSize(16);
    hexLabel->setTextStyle(ElaTextType::Subtitle);

    m_hexEdit = new QTextEdit();
    m_hexEdit->setPlaceholderText("在此输入 HEX 十六进制...\n例如：2A 49 44 4E 3F 0A");
    m_hexEdit->setMinimumHeight(100);
    m_hexEdit->setAcceptRichText(false);
    m_hexEdit->setStyleSheet(
        "QTextEdit {"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 14px;"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 6px;"
        "  padding: 10px;"
        "}"
    );

    rightCol->addWidget(hexLabel);
    rightCol->addWidget(m_hexEdit, 1);

    mainRow->addLayout(leftCol, 2);
    mainRow->addLayout(midCol, 0);
    mainRow->addLayout(rightCol, 2);
    root->addLayout(mainRow, 1);

    // ═══════════════════════════════════════════════════════
    //  底部按钮 + 选项行
    // ═══════════════════════════════════════════════════════
    QHBoxLayout* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(12);

    // 选项
    m_upperCaseCheck = new ElaCheckBox("大写 HEX 字母");
    m_upperCaseCheck->setChecked(true);

    m_spaceSepCheck = new ElaCheckBox("字节之间用空格分隔");
    m_spaceSepCheck->setChecked(true);

    bottomRow->addWidget(m_upperCaseCheck);
    bottomRow->addWidget(m_spaceSepCheck);
    bottomRow->addStretch();

    // 操作按钮
    m_copyHexBtn = new ElaPushButton("复制右侧 HEX");
    m_copyHexBtn->setFixedHeight(36);

    m_clearBtn = new ElaPushButton("清空全部");
    m_clearBtn->setFixedHeight(36);

    bottomRow->addWidget(m_copyHexBtn);
    bottomRow->addWidget(m_clearBtn);

    root->addLayout(bottomRow);

    // ═══════════════════════════════════════════════════════
    //  信号连接
    // ═══════════════════════════════════════════════════════
    connect(m_asciiToHexBtn, &ElaPushButton::clicked,
            this, &MainPage::convertAsciiToHex);
    connect(m_hexToAsciiBtn, &ElaPushButton::clicked,
            this, &MainPage::convertHexToAscii);

    connect(m_clearBtn, &ElaPushButton::clicked, this, [this]() {
        m_asciiEdit->clear();
        m_hexEdit->clear();
    });

    connect(m_copyHexBtn, &ElaPushButton::clicked, this, [this]() {
        QString hex = m_hexEdit->toPlainText();
        if (!hex.isEmpty()) {
            QApplication::clipboard()->setText(hex);
        }
    });

    // ═══════════════════════════════════════════════════════
    //  AN3.0 字段解析区
    // ═══════════════════════════════════════════════════════
    {
        QFrame *sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("QFrame { color: rgba(128,128,128,60); }");
        root->addWidget(sep);

        ElaText *an30Title = new ElaText("AN3.0 字段解析（HEX → 十进制）");
        an30Title->setTextPixelSize(18);
        an30Title->setTextStyle(ElaTextType::Subtitle);
        root->addWidget(an30Title);

        // ── 产品系列 + 命令码选择 ──
        QHBoxLayout *selectRow = new QHBoxLayout();
        selectRow->setSpacing(12);

        ElaText *prodLabel = new ElaText("产品系列:");
        prodLabel->setTextPixelSize(14);
        m_an30ProductCombo = new ElaComboBox();
        m_an30ProductCombo->addItems({"RGL系列 (交流源载)", "EVT系列 (直流电源)", "EVH系列 (双向直流电源 v1.5)"});
        m_an30ProductCombo->setCurrentIndex(0);
        m_an30ProductCombo->setStyleSheet("ElaComboBox { font-size: 13px; }");

        ElaText *cmdLabel = new ElaText("命令码:");
        cmdLabel->setTextPixelSize(14);
        m_an30CmdCombo = new ElaComboBox();
        m_an30CmdCombo->setMinimumWidth(100);
        m_an30CmdCombo->setStyleSheet("ElaComboBox { font-size: 13px; }");

        m_an30ParseBtn = new ElaPushButton("解析 →");
        m_an30ParseBtn->setFixedSize(90, 36);

        selectRow->addWidget(prodLabel);
        selectRow->addWidget(m_an30ProductCombo, 1);
        selectRow->addWidget(cmdLabel);
        selectRow->addWidget(m_an30CmdCombo, 2);
        selectRow->addWidget(m_an30ParseBtn);
        root->addLayout(selectRow);

        // ── 输入输出区 ──
        QHBoxLayout *an30Row = new QHBoxLayout();
        an30Row->setSpacing(16);

        QVBoxLayout *inCol = new QVBoxLayout();
        ElaText *inLabel = new ElaText("HEX 字段字节");
        inLabel->setTextPixelSize(14);
        m_an30Input = new QTextEdit();
        m_an30Input->setPlaceholderText("粘贴 AN3.0 帧中字段部分的 HEX 字节...\n例如: 00 00 27 28 00 00 27 12 00 00 03 EA");
        m_an30Input->setMinimumHeight(100);
        m_an30Input->setAcceptRichText(false);
        m_an30Input->setStyleSheet(m_hexEdit->styleSheet());
        inCol->addWidget(inLabel);
        inCol->addWidget(m_an30Input, 1);

        QVBoxLayout *outCol = new QVBoxLayout();
        QHBoxLayout *outTitleRow = new QHBoxLayout();
        ElaText *outLabel = new ElaText("解析结果（十进制物理值）");
        outLabel->setTextPixelSize(14);
        outTitleRow->addWidget(outLabel);
        outTitleRow->addStretch();
        m_an30CopyBtn = new ElaPushButton("复制");
        m_an30CopyBtn->setFixedSize(60, 28);
        outTitleRow->addWidget(m_an30CopyBtn);

        m_an30Output = new QTextEdit();
        m_an30Output->setReadOnly(true);
        m_an30Output->setMinimumHeight(100);
        m_an30Output->setAcceptRichText(false);
        m_an30Output->setStyleSheet(m_hexEdit->styleSheet());
        outCol->addLayout(outTitleRow);
        outCol->addWidget(m_an30Output, 1);

        an30Row->addLayout(inCol, 1);
        an30Row->addLayout(outCol, 1);
        root->addLayout(an30Row, 1);

        // ── 信号 ──
        connect(m_an30ProductCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
                this, &MainPage::onAn30ProductChanged);
        connect(m_an30ParseBtn, &ElaPushButton::clicked,
                this, &MainPage::parseAn30Fields);
        connect(m_an30CopyBtn, &ElaPushButton::clicked, this, [this]() {
            QString text = m_an30Output->toPlainText();
            if (!text.isEmpty())
                QApplication::clipboard()->setText(text);
        });

        // 初始化命令码下拉框
        populateAn30CmdCombo();
    }
}

// ═══════════════════════════════════════════════════════════════
//  AN3.0: 产品系列切换 → 刷新命令码下拉框
// ═══════════════════════════════════════════════════════════════
void MainPage::onAn30ProductChanged(int index) {
    An30Product prod;
    switch (index) {
        case 0: prod = An30Product::RGL; break;
        case 1: prod = An30Product::EVT; break;
        case 2: prod = An30Product::EVH; break;
        default: prod = An30Product::RGL; break;
    }
    An30Layout::instance().setProduct(prod);
    populateAn30CmdCombo();
    m_an30Output->clear();
}
// ═══════════════════════════════════════════════════════════════
//  AN3.0: 填充命令码下拉框
// ═══════════════════════════════════════════════════════════════
void MainPage::populateAn30CmdCombo() {
    m_an30CmdCombo->clear();

    An30Product prod = An30Layout::instance().product();

    // ═══════════════════════════════════════════════════════
    //  RGL 系列 — 可回馈交流源载一体机（原有，不变）
    // ═══════════════════════════════════════════════════════
    if (prod == An30Product::RGL) {
        m_an30CmdCombo->addItem("F0 A4 — 查询输出测量值 (15字段)",        "F0A4");
        m_an30CmdCombo->addItem("F0 EB — 查询状态/报警码 (2字段)",         "F0EB");
        m_an30CmdCombo->addItem("A5 41 — 查询常规参数 (3字段)",            "A541");
        m_an30CmdCombo->addItem("A5 40 — 查询更多设置 (10字段)",           "A540");
        m_an30CmdCombo->addItem("A5 AE — 查询序列参数 (13字段)",           "A5AE");
        m_an30CmdCombo->addItem("A5 80 — 查询输出限值 (4字段)",            "A580");
        m_an30CmdCombo->addItem("A5 82 — 查询输出波形 (8字段)",            "A582");
        m_an30CmdCombo->addItem("A5 90 — 查询系统状态 (6字段)",            "A590");
        m_an30CmdCombo->addItem("A5 A3 — 查询恒流参数 (4字段)",            "A5A3");
        m_an30CmdCombo->addItem("A5 A6 — 查询恒有功参数 (4字段)",          "A5A6");
        m_an30CmdCombo->addItem("A5 A4 — 查询恒阻参数 (1字段)",            "A5A4");
        m_an30CmdCombo->addItem("A5 32 — 查询间谐波参数 (4字段)",          "A532");
    }
    // ═══════════════════════════════════════════════════════
    //  EVT 系列 — 直流电源（原 EVH 协议）
    // ═══════════════════════════════════════════════════════
    else if (prod == An30Product::EVT) {
        m_an30CmdCombo->addItem("F0 00 — 查询输出状态 (1字段)",            "F000");
        m_an30CmdCombo->addItem("F0 80 — 查询电压电流功率 (3字段)",        "F080");
        m_an30CmdCombo->addItem("F0 81 — 查询电压电流功率电阻 (4字段)",    "F081");
        m_an30CmdCombo->addItem("F0 EB — 查询当前状态 (1字段)",            "F0EB");
        m_an30CmdCombo->addItem("F0 24 — 查询报警代码 (1字段)",            "F024");
        m_an30CmdCombo->addItem("A5 00 — 查询设定电压 (1字段)",            "A500");
        m_an30CmdCombo->addItem("A5 01 — 查询设定正向电流 (1字段)",        "A501");
        m_an30CmdCombo->addItem("A5 02 — 查询设定正向功率 (1字段)",        "A502");
        m_an30CmdCombo->addItem("A5 03 — 查询OVP (1字段)",                 "A503");
        m_an30CmdCombo->addItem("A5 04 — 查询设定反向电流 (1字段)",        "A504");
        m_an30CmdCombo->addItem("A5 05 — 查询设定反向功率 (1字段)",        "A505");
        m_an30CmdCombo->addItem("A5 06 — 查询设定电阻 (1字段)",            "A506");
        m_an30CmdCombo->addItem("A5 1F — 查询全部设定值 (7字段)",          "A51F");
        m_an30CmdCombo->addItem("A5 60 — 查询电压上下限报警 (6字段)",      "A560");
        m_an30CmdCombo->addItem("A5 63 — 查询限值 (5字段)",                "A563");
        m_an30CmdCombo->addItem("A5 68 — 查询全部限值 (8字段)",            "A568");
        m_an30CmdCombo->addItem("A6 01 — 查询电子负载参数 (5字段)",        "A601");
    }
    // ═══════════════════════════════════════════════════════
    //  EVH 系列 — 双向可编程直流电源（根据用户手册 v1.5）
    // ═══════════════════════════════════════════════════════
    else {
        m_an30CmdCombo->addItem("F0 00 — 查询输出状态 (1字段)",            "F000");
        m_an30CmdCombo->addItem("F0 10 — 查询电压输出值 (1字段)",          "F010");
        m_an30CmdCombo->addItem("F0 11 — 查询电流输出值 (1字段)",          "F011");
        m_an30CmdCombo->addItem("F0 12 — 查询功率输出值 (1字段)",          "F012");
        m_an30CmdCombo->addItem("F0 20 — 查询电压电流斜率 (2字段)",        "F020");
        m_an30CmdCombo->addItem("F0 80 — 查询电压电流功率 (3字段)",        "F080");
        m_an30CmdCombo->addItem("F0 81 — 查询电压电流功率电阻 (4字段)",    "F081");
        m_an30CmdCombo->addItem("F0 EB — 查询当前状态 (1字段)",            "F0EB");
        m_an30CmdCombo->addItem("F0 24 — 查询报警代码 (1字段)",            "F024");
        m_an30CmdCombo->addItem("F0 ED — 查询机器型号 (3字段)",            "F0ED");
        m_an30CmdCombo->addItem("A5 00 — 查询设定电压 (1字段)",            "A500");
        m_an30CmdCombo->addItem("A5 01 — 查询设定正向电流 (1字段)",        "A501");
        m_an30CmdCombo->addItem("A5 02 — 查询设定正向功率 (1字段)",        "A502");
        m_an30CmdCombo->addItem("A5 03 — 查询OVP (1字段)",                 "A503");
        m_an30CmdCombo->addItem("A5 04 — 查询设定反向电流 (1字段)",        "A504");
        m_an30CmdCombo->addItem("A5 05 — 查询设定反向功率 (1字段)",        "A505");
        m_an30CmdCombo->addItem("A5 06 — 查询设定电阻 (1字段)",            "A506");
        m_an30CmdCombo->addItem("A5 1F — 查询全部设定值 (7字段)",          "A51F");
        m_an30CmdCombo->addItem("A5 58 — 查询正向功率上限报警 (2字段)",    "A558");
        m_an30CmdCombo->addItem("A5 59 — 查询反向功率上限报警 (2字段)",    "A559");
        m_an30CmdCombo->addItem("A5 60 — 查询电压上下限报警 (6字段)",      "A560");
        m_an30CmdCombo->addItem("A5 61 — 查询正向电流上下限报警 (6字段)",  "A561");
        m_an30CmdCombo->addItem("A5 62 — 查询反向电流上下限报警 (3字段)",  "A562");
        m_an30CmdCombo->addItem("A5 63 — 查询限值 (5字段)",                "A563");
        m_an30CmdCombo->addItem("A5 68 — 查询全部限值 (8字段)",            "A568");
        m_an30CmdCombo->addItem("A5 67 — 查询从机系统配置 (3字段)",        "A567");
        m_an30CmdCombo->addItem("A6 01 — 查询电子负载参数 (5字段)",        "A601");
    }
}

// ═══════════════════════════════════════════════════════════════
//  AN3.0: 解析 HEX 字段 → 十进制物理值
// ═══════════════════════════════════════════════════════════════
void MainPage::parseAn30Fields() {
    m_an30Output->clear();

    QString cmdKey = m_an30CmdCombo->currentData().toString();
    if (cmdKey.isEmpty() || cmdKey.length() != 4) {
        m_an30Output->setPlainText("请选择命令码");
        return;
    }

    uint8_t cmdType = static_cast<uint8_t>(cmdKey.left(2).toInt(nullptr, 16));
    uint8_t cmdWord = static_cast<uint8_t>(cmdKey.mid(2, 2).toInt(nullptr, 16));

    const CmdLayout* layout = An30Layout::instance().find(cmdType, cmdWord);
    if (!layout) {
        m_an30Output->setPlainText("未找到该命令码的字段布局");
        return;
    }

    // 解析输入 HEX
    QString hexText = m_an30Input->toPlainText();
    hexText.remove(' ').remove('\n').remove('\r');
    QByteArray payload = QByteArray::fromHex(hexText.toLatin1());
    if (payload.isEmpty()) {
        m_an30Output->setPlainText("请输入有效的 HEX 字段字节");
        return;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.constData());
    QVector<double> values = An30Layout::instance().extractAll(*layout, data, payload.size());

    QStringList result;
    for (int i = 0; i < values.size() && i < layout->fields.size(); ++i) {
        const FieldDef& f = layout->fields[i];
        // 判断是否为整数字段（除数=1）
        if (f.divisor == 1.0 && f.byteLen <= 2) {
            result.append(QString("%1 = %2").arg(f.name).arg((int)values[i]));
        } else {
            result.append(QString("%1 = %2").arg(f.name)
                              .arg(values[i], 0, 'f', 3));
        }
    }

    m_an30Output->setPlainText(result.join('\n'));
}

// ═══════════════════════════════════════════════════════════════
//  转换：ASCII → HEX  （读左侧 → 写右侧）
// ═══════════════════════════════════════════════════════════════
void MainPage::convertAsciiToHex() {
    QString input = m_asciiEdit->toPlainText();
    if (input.isEmpty()) {
        m_hexEdit->clear();
        return;
    }

    bool upper = m_upperCaseCheck->isChecked();
    bool space = m_spaceSepCheck->isChecked();

    QStringList lines = input.split('\n');
    QStringList result;

    for (const QString& line : lines) {
        QByteArray utf8 = line.toUtf8();
        if (utf8.isEmpty()) {
            result.append("");
            continue;
        }

        QString hexLine;
        for (int i = 0; i < utf8.size(); ++i) {
            if (i > 0 && space)
                hexLine += ' ';
            hexLine += QString("%1").arg(
                (unsigned char)utf8.at(i), 2, 16, QChar('0'));
        }

        result.append(upper ? hexLine.toUpper() : hexLine.toLower());
    }

    m_hexEdit->setPlainText(result.join('\n'));
}

// ═══════════════════════════════════════════════════════════════
//  转换：HEX → ASCII  （读右侧 → 写左侧）
// ═══════════════════════════════════════════════════════════════
void MainPage::convertHexToAscii() {
    QString input = m_hexEdit->toPlainText();
    if (input.isEmpty()) {
        m_asciiEdit->clear();
        return;
    }

    QStringList lines = input.split('\n');
    QStringList result;

    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            result.append("");
            continue;
        }

        // 去掉所有空白字符
        QString hex = line;
        hex.remove(' ');
        hex.remove('\t');

        // 必须是偶数长度
        if (hex.length() % 2 != 0) {
            result.append(QString("[错误] 无效HEX长度: %1").arg(line.trimmed()));
            continue;
        }

        QByteArray bytes = QByteArray::fromHex(hex.toLatin1());
        if (bytes.isEmpty() && !hex.isEmpty()) {
            result.append(QString("[错误] 无法解析: %1").arg(line.trimmed()));
            continue;
        }

        result.append(QString::fromUtf8(bytes));
    }

    m_asciiEdit->setPlainText(result.join('\n'));
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

    // ════════════════ 2. 发送后缀（通用功能）════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("2. 发送后缀（串口 / 网口 / GPIB 通用）"),
        QStringList{
            QString::fromUtf8("「发送后缀」是三个通讯模块共用的核心功能，位于各模块的「设置」页面中。"),
            QString::fromUtf8("作用：发送命令时，自动在命令末尾追加指定的控制字符作为终止符。"),
            QString(),
            QString::fromUtf8("后缀选项与追加的实际字节："),
            QString::fromUtf8("  • 无 (None) — 不追加任何字节"),
            QString::fromUtf8("  • CR (\\r)   — 追加回车符 0x0D（1 字节）"),
            QString::fromUtf8("  • LF (\\n)   — 追加换行符 0x0A（1 字节，大多数 SCPI 仪器推荐）"),
            QString::fromUtf8("  • CRLF (\\r\\n) — 追加回车+换行 0x0D 0x0A（2 字节）"),
            QString(),
            QString::fromUtf8("后缀处理流程（buildSendData 统一入口）："),
            QString::fromUtf8("  ① unescape — 将命令中 \\r \\n 字面量转为真实控制字符"),
            QString::fromUtf8("  ② 去尾 — 去除命令末尾已有的 \\r \\n（避免与后缀重复）"),
            QString::fromUtf8("  ③ 加后缀 — 根据下拉框选择追加对应的终止字节"),
            QString(),
            QString::fromUtf8("建议：Excel 命令列中不要手动写 \\n，统一通过后缀下拉框控制终止符。"),
            QString::fromUtf8("      如果仪器发命令后无响应，请优先尝试切换后缀为 LF (\\n)。")
        }));

    // ════════════════ 3. 网口通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("3. 网口通讯"),
        QStringList{
            QString::fromUtf8("3.1  配置网络参数"),
            QString::fromUtf8("    在「网络设置」页中输入目标设备的 IP 地址和端口号。"),
            QString::fromUtf8("    可选勾选「禁用 Nagle 算法」以降低发送延迟（适合小数据包高频发送场景）。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」以十六进制方式编码命令。"),
            QString::fromUtf8("    通过「发送后缀」下拉框选择命令终止符（详见第 2 节）。"),
            QString::fromUtf8("    可选勾选「比对时去除返回值中的 \\r\\n」以忽略返回值中的换行差异。"),
            QString::fromUtf8("    可选勾选「启用粘包分割」以按分隔符拆分连续返回的多条数据。"),
            QString::fromUtf8("    点击「连接网络」，软件将在 3 秒内尝试建立 TCP 连接。"),
            QString::fromUtf8("    连接成功后 LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("3.2  单条命令发送"),
            QString::fromUtf8("    在「单条发送」页中输入命令文本，点击「通过网口发送」。"),
            QString::fromUtf8("    发送和接收日志实时显示在下方列表中。"),
            QString(),
            QString::fromUtf8("3.3  表格批量发送"),
            QString::fromUtf8("    ① 点击「下载示例模板」获取标准格式的 Excel 文件"),
            QString::fromUtf8("    ② 按模板格式编写命令表（A列=命令，B列=期望返回值，C列=命令间隔ms）"),
            QString::fromUtf8("    ③ 点击「打开 Excel 并读取」加载文件到表格"),
            QString::fromUtf8("    ④ 在「发送次数」中填写循环次数（0=无限循环）"),
            QString::fromUtf8("    ⑤ 在「超时时间」中填写全局超时（超过此时长未收到回复即判定超时）"),
            QString::fromUtf8("    ⑥ 点击「开始发送」启动批量测试"),
            QString::fromUtf8("    ⑦ 点击「读取返回值」会仅发送一次并记录实际返回值（预扫模式）"),
            QString(),
            QString::fromUtf8("3.4  错误统计"),
            QString::fromUtf8("    超时错误：在超时时间内未收到设备回复"),
            QString::fromUtf8("    内容错误：设备回复的内容与 Excel 中定义的期望值不一致"),
            QString::fromUtf8("    错误列表支持自动滚动和清空操作")
        }));

    // ════════════════ 4. 串口通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("4. 串口通讯"),
        QStringList{
            QString::fromUtf8("4.1  配置串口参数"),
            QString::fromUtf8("    在「串口设置」中选择端口号、波特率、数据位、停止位、校验位。"),
            QString::fromUtf8("    可选勾选「合并串口接收数据(20ms超时)」以避免数据帧被拆分显示。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」切换十六进制模式。"),
            QString::fromUtf8("    通过「发送后缀」下拉框选择命令终止符（详见第 2 节）。"),
            QString::fromUtf8("    可选勾选「比对时去除返回值中的 \\r\\n」以忽略返回值中的换行差异。"),
            QString::fromUtf8("    可选勾选「启用粘包分割」以按分隔符拆分连续返回的多条数据。"),
            QString::fromUtf8("    点击「打开串口」建立连接，LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("4.2  单条命令与表格批量发送"),
            QString::fromUtf8("    操作方式与网口通讯一致，请参考第 3 节的说明。")
        }));

    // ════════════════ 5. GPIB 通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("5. GPIB 通讯"),
        QStringList{
            QString::fromUtf8("5.1  配置 GPIB 参数"),
            QString::fromUtf8("    在「GPIB设置」页中输入板卡号（通常为 0）、仪器主地址（0-30）。"),
            QString::fromUtf8("    如需使用副地址（某些多通道仪器），可填入副地址（0=不使用）。"),
            QString::fromUtf8("    设置超时时间（建议 3000ms，可根据仪器响应速度调整）。"),
            QString::fromUtf8("    通过「发送后缀」下拉框选择命令终止符（详见第 2 节）。"),
            QString::fromUtf8("    结束字符默认为换行符 \\n，启用后 viRead 会在收到换行符时终止读取。"),
            QString::fromUtf8("    可选勾选「发送时附加 EOI 信号」，大多数 GPIB 仪器需要此信号标识命令结束。"),
            QString::fromUtf8("    可选勾选「以HEX格式发送」切换十六进制模式。"),
            QString::fromUtf8("    点击「打开 GPIB」建立连接，LED 指示灯变为绿色。"),
            QString(),
            QString::fromUtf8("5.2  前置条件"),
            QString::fromUtf8("    a) 必须安装 NI-VISA 运行时驱动（NI-488.2 或 NI-VISA 独立包）。"),
            QString::fromUtf8("    b) GPIB 控制器（如 NI GPIB-USB-HS）需正确插入并被系统识别。"),
            QString::fromUtf8("    c) 可在 NI-MAX（Measurement & Automation Explorer）中验证设备可见性。"),
            QString::fromUtf8("    d) 确认仪器 GPIB 地址与软件中填写的主地址一致。"),
            QString(),
            QString::fromUtf8("5.3  单条命令发送"),
            QString::fromUtf8("    在「单条发送」页中输入 SCPI 命令文本，点击「通过 GPIB 发送」。"),
            QString::fromUtf8("    发送和接收日志实时显示在下方列表中。"),
            QString(),
            QString::fromUtf8("5.4  表格批量发送"),
            QString::fromUtf8("    操作方式与网口通讯一致，请参考第 3.3 节的说明。"),
            QString::fromUtf8("    支持「读取返回值（捕获）」模式，自动将仪器返回值填入 Excel B 列。"),
            QString(),
            QString::fromUtf8("5.5  错误统计"),
            QString::fromUtf8("    超时错误：在超时时间内未收到仪器回复"),
            QString::fromUtf8("    内容错误：仪器回复的内容与 Excel 中定义的期望值不一致"),
            QString::fromUtf8("    错误列表支持自动滚动和清空操作"),
            QString(),
            QString::fromUtf8("5.6  常见 GPIB 问题"),
            QString::fromUtf8("    Q: 打开 GPIB 时提示「未找到目标资源」？"),
            QString::fromUtf8("    A: 检查板卡号和主地址是否正确，仪器是否上电，NI-MAX 中是否可见该设备。"),
            QString(),
            QString::fromUtf8("    Q: 打开 GPIB 时提示「未找到 VISA 运行库」？"),
            QString::fromUtf8("    A: 请确认 NI-VISA 已正确安装。可从 NI 官网下载 NI-VISA 运行时（免费）。"),
            QString(),
            QString::fromUtf8("    Q: GPIB 连接超时（5秒弹窗）？"),
            QString::fromUtf8("    A: 检查 GPIB 线缆连接是否牢固，仪器是否上电且处于远程控制模式。")
        }));

    // ════════════════ 6. CAN 通讯 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("6. CAN 通讯"),
        QStringList{
            QString::fromUtf8("CAN 通讯模块目前为预留接口，具体功能将在后续版本中完善。"),
            QString::fromUtf8("如需使用请联系开发者获取技术支持。")
        }));

    // ════════════════ 7. Excel 模板格式 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("7. Excel 模板格式"),
        QStringList{
            QString::fromUtf8("Excel 文件必须包含表头行（第 1 行），数据从第 2 行开始。"),
            QString::fromUtf8("表格结构："),
            QString::fromUtf8("  第 1 列 (A) — 发送的命令          (必需)"),
            QString::fromUtf8("  第 2 列 (B) — 正确的返回值        (可选，留空=不校验)"),
            QString::fromUtf8("  第 3 列 (C) — 到下一条命令的时间ms  (可选，默认 100ms)"),
            QString(),
            QString::fromUtf8("命令文本说明："),
            QString::fromUtf8("  普通模式：直接输入 ASCII 命令，如  *IDN?"),
            QString::fromUtf8("            建议不要在命令末尾添加 \\n，统一通过「发送后缀」控制"),
            QString::fromUtf8("  HEX 模式：输入十六进制字符串（空格分隔），如  2A 49 44 4E 3F"),
            QString::fromUtf8("            勾选「以HEX格式发送」后生效（HEX 模式不追加后缀）"),
            QString(),
            QString::fromUtf8("期望返回值说明："),
            QString::fromUtf8("  普通模式：直接输入期望的 ASCII 文本"),
            QString::fromUtf8("  HEX 模式：输入期望的十六进制字符串"),
            QString::fromUtf8("  留空则不对该命令的返回值进行校验"),
            QString(),
            QString::fromUtf8("设置命令（无回复）说明："),
            QString::fromUtf8("  对于 *RST、CONF:VOLT:DC 10 等设备不回复的设置命令，"),
            QString::fromUtf8("  只需将 B 列留空，软件会自动识别并在延时后直接发送下一条。")
        }));

        // ════════════════ 7.5 AN3.0 HEX 区间判断 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("7.5  AN3.0 HEX 区间判断（V3.4.11 新增）"),
        QStringList{
            QString::fromUtf8("适用于 RGL 系列可回馈交流源载一体机的 AN3.0 二进制协议。"),
            QString::fromUtf8("软件会根据应答帧中的命令码自动匹配字段布局（偏移、字节长度、除数），"),
            QString::fromUtf8("将参考帧和实时帧同构解析为物理值后逐字段对比。"),
            QString(),
            QString::fromUtf8("使用步骤："),
            QString::fromUtf8("    a) 在设置页勾选「以HEX格式发送（AN3.0）」，HEX 区间判断控件自动解禁。"),
            QString::fromUtf8("    b) 勾选「启用HEX区间判断（AN3.0自动解析）」，输入容差值（如 0.5）。"),
            QString::fromUtf8("    c) 先用捕获模式获取参考帧，自动填入 Excel B 列。"),
            QString::fromUtf8("    d) 切换到发送模式，软件自动解析并逐字段对比。"),
            QString(),
            QString::fromUtf8("支持的查询命令（22 条）："),
            QString::fromUtf8("    查询输出测量值    F0 A4 — 15 字段（电压/电流/功率/频率/峰值等）"),
            QString::fromUtf8("    查询状态/报警码    F0 EB — 2 字段（状态码/报警码）"),
            QString::fromUtf8("    查询常规参数      A5 41 — 3 字段（设定交流/直流电压/频率）"),
            QString::fromUtf8("    查询更多设置      A5 40 — 10 字段（转换率/波形/模式等）"),
            QString::fromUtf8("    查询序列参数      A5 AE — 13 字段（步号/起止电压/频率等）"),
            QString::fromUtf8("    查询输出限值      A5 80 — 4 字段"),
            QString::fromUtf8("    查询输出保护      A5 81 — 4 字段"),
            QString::fromUtf8("    查询输出波形      A5 82 — 8 字段"),
            QString::fromUtf8("    查询输出其他      A5 83 — 9 字段"),
            QString::fromUtf8("    查询系统状态      A5 90 — 6 字段"),
            QString::fromUtf8("    查询第二组系统    A5 EF — 4 字段"),
            QString::fromUtf8("    查询输出模式      A5 20 — 1 字段"),
            QString::fromUtf8("    查询恒流参数      A5 A3 — 4 字段"),
            QString::fromUtf8("    查询恒流更多      A5 B0 — 7 字段"),
            QString::fromUtf8("    查询恒有功        A5 A6 — 4 字段"),
            QString::fromUtf8("    查询恒有功更多    A5 B2 — 3 字段"),
            QString::fromUtf8("    查询恒视在        A5 A5 — 4 字段"),
            QString::fromUtf8("    查询恒视在更多    A5 B1 — 3 字段"),
            QString::fromUtf8("    查询恒阻参数      A5 A4 — 1 字段"),
            QString::fromUtf8("    查询恒阻更多      A5 BF — 1 字段"),
            QString::fromUtf8("    查询RLC参数       A5 A7 — 6 字段"),
            QString::fromUtf8("    查询间谐波参数    A5 32 — 4 字段"),
            QString::fromUtf8("    查询谐波更多      A5 62 — 4 字段"),
            QString(),
            QString::fromUtf8("比对示例（查询常规参数 0xA5 0x41）："),
            QString::fromUtf8("    发送: 7B 00 08 01 A5 AA 58 7D"),
            QString::fromUtf8("    参考帧(列B): 7B 00 11 01 A5 41 00 55 F0 00 00 00 00 C3 50 4A 7D"),
            QString::fromUtf8("    解析: 设定交流电压=220.00V  设定直流电压=0.00V  设定频率=50.000Hz"),
            QString::fromUtf8("    容差=0.5: 实时帧解析值在 [参考±0.5] 范围内 → 合格 ✅"),
            QString(),
            QString::fromUtf8("注意："),
            QString::fromUtf8("    ① 列B必须填完整HEX帧（含帧头7B到帧尾7D），不能只填部分字节。"),
            QString::fromUtf8("    ② 参考帧与实时帧命令码必须一致，否则直接判定不合格。"),
            QString::fromUtf8("    ③ 不勾选HEX区间判断时，HEX模式仍使用精确逐字节比对。"),
        }));

    // ════════════════ 8. 常见问题 ════════════════
    lay->addWidget(createHelpSection(
        QString::fromUtf8("8. 常见问题"),
        QStringList{
            QString::fromUtf8("Q: 连接网络超时（3秒弹窗）？"),
            QString::fromUtf8("A: 请检查 IP 地址和端口号是否正确，目标设备是否在线，防火墙是否阻止了连接。"),
            QString(),
            QString::fromUtf8("Q: 串口列表中没有可用端口？"),
            QString::fromUtf8("A: 请检查设备驱动是否安装正确，设备是否已插入，可在 Windows 设备管理器中确认。"),
            QString(),
            QString::fromUtf8("Q: 发送命令后未收到回复？"),
            QString::fromUtf8("A: 检查连接状态 LED 是否为绿色；检查命令格式是否正确；适当增大全局超时时间。"),
            QString::fromUtf8("   ★ 如果是串口/网口/GPIB 仪器不响应，请优先检查「发送后缀」设置。"),
            QString::fromUtf8("      大多数 SCPI 仪器需要 LF (\\n) 作为命令终止符。"),
            QString(),
            QString::fromUtf8("Q: 发送后缀应该怎么选？"),
            QString::fromUtf8("A: 如果不确定，按以下顺序尝试："),
            QString::fromUtf8("   ① 先选 LF (\\n) — 绝大多数 SCPI 仪器的标准终止符"),
            QString::fromUtf8("   ② 如果仍无响应，尝试 CRLF (\\r\\n) — 部分 PLC / 工控设备需要"),
            QString::fromUtf8("   ③ 对于 GPIB 仪器，可以先试「无 (None)」— 靠 EOI 硬件信号终止"),
            QString::fromUtf8("   ④ HEX 模式下不追加后缀，请直接在命令中编码所需的终止字节"),
            QString(),
            QString::fromUtf8("Q: Excel 文件读取失败？"),
            QString::fromUtf8("A: 请确保使用 .xlsx 格式，且表头和数据格式符合模板规范。可先下载示例模板参考。"),
            QString(),
            QString::fromUtf8("Q: Nagle 算法是什么？是否需要禁用？"),
            QString::fromUtf8("A: Nagle 算法会合并小数据包以提升网络效率。对于 SCPI 指令这种小包高频场景，"),
            QString::fromUtf8("   建议禁用（勾选该选项）以避免延迟。"),
            QString(),
            QString::fromUtf8("Q: 命令中已包含 \\n，又选了后缀会重复吗？"),
            QString::fromUtf8("A: 不会。软件在追加后缀前会自动去除命令末尾已有的 \\r 和 \\n，确保不重复。"),
            QString(),
            QString::fromUtf8("Q: GPIB 发送命令后仪器不响应？"),
            QString::fromUtf8("A: 首先检查 GPIB 连接状态 LED 是否为绿色。若连接正常但仍无响应："),
            QString::fromUtf8("   ① 在 GPIB 设置页中尝试切换「发送后缀」为 LF (\\n)；"),
            QString::fromUtf8("   ② 适当增大超时时间（如 5000ms），部分老旧仪器响应较慢；"),
            QString::fromUtf8("   ③ 确认仪器支持的命令格式无误（可先用 NI-MAX 手动测试）。")
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

    // ──── 可滚动区域 ────
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(scrollContent);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(16);

    // ──── 软件名称 ────
    ElaText *title = new ElaText("Ainuo 通用通讯可靠性测试软件");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    // ──── 版本 ────
    ElaText *version = new ElaText(QString::fromUtf8("版本: v3.4.14"));
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

    addInfo(QString::fromUtf8("软件版本："), "v3.4.14");
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
        QString::fromUtf8("支持串口 / 网口 / CAN / GPIB 四种通讯方式"),
        QString::fromUtf8("Excel 表格批量导入命令，自动逐条发送"),
        QString::fromUtf8("自动校验返回值，统计超时和内容错误"),
        QString::fromUtf8("支持 HEX 和 ASCII 两种命令编码格式"),
        QString::fromUtf8("全模块发送后缀选择（无 / CR / LF / CRLF），自动去重"),
        QString::fromUtf8("buildSendData() 统一数据构建（unescape → 去尾 → 加后缀）"),
        QString::fromUtf8("独立单条命令发送模式，便于调试"),
        QString::fromUtf8("实时收发日志，带毫秒级时间戳"),
        QString::fromUtf8("错误统计卡片 + 6列详细错误列表"),
        QString::fromUtf8("Nagle 算法可选禁用（网口低延迟模式）"),
        QString::fromUtf8("GPIB 仪器地址配置 + EOI / 结束字符控制"),
        QString::fromUtf8("命令间隔精确延时控制（1ms 精度，EMA 补偿）"),
        QString::fromUtf8("多线程架构，收发与 UI 完全分离"),
        QString::fromUtf8("粘包分割（串口 / 网口），按分隔符拆分连续数据"),
        QString::fromUtf8("比对时去除返回值 \\r\\n，灵活适配不同仪器")
    };

    for (const QString &feat : features) {
        ElaText *ft = new ElaText(feat);
        ft->setTextPixelSize(14);
        ft->setWordWrap(true);
        featureLayout->addWidget(ft);
    }

    lay->addWidget(featureGroup);

    // ──── 更新日志摘要 ────
    QGroupBox *changelogGroup = new QGroupBox(QString::fromUtf8("V3.4.11 更新要点"));
    changelogGroup->setStyleSheet(infoGroup->styleSheet());
    QVBoxLayout *changelogLayout = new QVBoxLayout(changelogGroup);
    changelogLayout->setSpacing(4);
    changelogLayout->setContentsMargins(20, 20, 20, 20);

    QStringList changelog = {
        QString::fromUtf8("AN3.0 HEX 区间判断：串口/网口/GPIB 设置页「HEX区间判断」全面启用"),
        QString::fromUtf8("基于 An30Layout 命令码自动匹配字段布局，零配置使用"),
        QString::fromUtf8("注册全部 22 条 AN3.0 查询命令（F0 A4 / A5 41 / F0 EB 等）"),
        QString::fromUtf8("参考帧与实时帧同构解析后逐字段对比，误差 ±tolerance 判定"),
        QString::fromUtf8("Excel B 列支持直接填入 HEX 捕获帧，自动识别命令码无需手动配置"),
        QString::fromUtf8("新增 An30FieldExtractor / An30Layout 公共解析库"),
        QString::fromUtf8("RangeComparer 新增 compareHexFrame() 公共方法"),
        QString::fromUtf8("HEX 区间判断与 HEX 发送勾选框三级联动（HEX→区间→偏差值）"),
        QString::fromUtf8("修复 HEX 区间模式重复记录错误统计的 Bug"),
        QString::fromUtf8("帮助文档新增「AN3.0 HEX 区间判断」专题章节"),
    };

    for (const QString &log : changelog) {
        ElaText *lt = new ElaText(log);
        lt->setTextPixelSize(13);
        lt->setWordWrap(true);
        changelogLayout->addWidget(lt);
    }

    lay->addWidget(changelogGroup);


    lay->addStretch();

    // ──── 底部版权 ────
    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("QFrame { color: rgba(128,128,128,40); }");
    lay->addWidget(line2);

    ElaText *copyright = new ElaText(QString::fromUtf8("\u00a9 2026 Cossiant. All rights reserved."));
    copyright->setTextPixelSize(12);
    copyright->setAlignment(Qt::AlignCenter);
    lay->addWidget(copyright);

    // ──── 将内容挂到滚动区 ────
    scrollArea->setWidget(scrollContent);

    QVBoxLayout *outer = new QVBoxLayout(_MainAboutPage);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scrollArea);
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
