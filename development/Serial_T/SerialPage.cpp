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

    //初始化SerialWork
    m_serialWork = new SerialWork(this, this);
    //初始化SerialFunction
    m_serialFunc = new SerialExcel(this, this);
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
    QVBoxLayout *_SerialSettingLayout1 = new QVBoxLayout(_SerialSettingPage);
    _SerialSettingLayout1->setContentsMargins(30, 30, 30, 30);

    // ──── 标题 ────
    ElaText *_SerialSettingTitle = new ElaText("串口设置界面");
    _SerialSettingTitle->setTextPixelSize(24);
    _SerialSettingTitle->setTextStyle(ElaTextType::Title);

    _SerialSettingLayout1->addWidget(_SerialSettingTitle);
    // _SerialSettingLayout->addStretch();

    // ════════════════════════════════════════════════════════
    //  串口参数 GroupBox （创建一个Group）
    // ════════════════════════════════════════════════════════
    QGroupBox* _SerialSettingGroup = new QGroupBox("串口参数");
    QGridLayout* grid = new QGridLayout(_SerialSettingGroup);
    grid->setSpacing(10);
    grid->setContentsMargins(30, 30, 30, 30);

    // ──── 第 0 行：串口端口号 | 波特率 ────
    ElaText* portLabel = new ElaText("串口端口号:");
    portLabel->setTextPixelSize(15);
    m_serialPortComboBox = new ElaComboBox();

    // 枚举系统可用串口
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        m_serialPortComboBox->addItem("无可用串口");
    } else {
        for (const QSerialPortInfo &info : portList)
            m_serialPortComboBox->addItem(info.portName());
    }

    //添加波特率
    ElaText* baudLabel = new ElaText("波特率:");
    baudLabel->setTextPixelSize(15);
    m_baudRateComboBox = new ElaComboBox();
    m_baudRateComboBox->addItems({"1200", "2400", "4800", "9600",
                                   "19200", "38400", "57600", "115200"});
    m_baudRateComboBox->setCurrentText("115200");

    grid->addWidget(portLabel,               0, 0);
    grid->addWidget(m_serialPortComboBox,    0, 1);
    grid->addWidget(baudLabel,               0, 2);
    grid->addWidget(m_baudRateComboBox,      0, 3);

    // ──── 第 1 行：数据位 | 停止位 ────
    ElaText* dataLabel = new ElaText("数据位:");
    dataLabel->setTextPixelSize(15);
    m_dataBitsComboBox = new ElaComboBox();
    m_dataBitsComboBox->addItems({"5", "6", "7", "8"});
    m_dataBitsComboBox->setCurrentText("8");
    ElaText* stopLabel = new ElaText("停止位:");
    stopLabel->setTextPixelSize(15);
    m_stopBitsComboBox = new ElaComboBox();
    m_stopBitsComboBox->addItems({"1", "1.5", "2"});
    m_stopBitsComboBox->setCurrentText("1");

    grid->addWidget(dataLabel,           1, 0);
    grid->addWidget(m_dataBitsComboBox,  1, 1);
    grid->addWidget(stopLabel,           1, 2);
    grid->addWidget(m_stopBitsComboBox,  1, 3);

    // ──── 第 2 行：校验位 | 串口状态 LED ────
    ElaText* parityLabel = new ElaText("校验位:");
    parityLabel->setTextPixelSize(15);
    m_parityComboBox = new ElaComboBox();
    m_parityComboBox->addItems({"None", "Even", "Odd", "Space", "Mark"});
    m_parityComboBox->setCurrentText("None");
    ElaText* statusLabel = new ElaText("串口状态:");
    statusLabel->setTextPixelSize(15);
    m_serialLED = new QLabel();
    LED::setLED(m_serialLED, 0, 16);          // 0 = 灰色（未连接）

    grid->addWidget(parityLabel,         2, 0);
    grid->addWidget(m_parityComboBox,    2, 1);
    grid->addWidget(statusLabel,         2, 2);
    grid->addWidget(m_serialLED,         2, 3);

    // ──── 第 3 行：勾选框 ────
    m_serialBufferCheckBox = new ElaCheckBox("合并串口接收数据（20ms 超时）");
    m_serialBufferCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_serialHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_serialHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_serialBufferCheckBox,   3, 0, 1, 2);   // 左半边
    grid->addWidget(m_serialHexSendCheckBox,  3, 2, 1, 2);   // 右半边

    // ──── 第 4 行：打开/关闭按钮 ────
    m_openSerialButton = new ElaPushButton("打开串口");
    m_openSerialButton->setFixedHeight(35);
    m_closeSerialButton = new ElaPushButton("关闭串口");
    m_closeSerialButton->setFixedHeight(35);
    m_closeSerialButton->setEnabled(false);
    grid->addWidget(m_openSerialButton,       4, 0);
    grid->addWidget(m_closeSerialButton,      4, 1);

    _SerialSettingLayout1->addWidget(_SerialSettingGroup);
    _SerialSettingLayout1->addStretch();
}

void SerialPage::createSendPage() {
    _SerialSendPage = new QWidget();
    QVBoxLayout *_SerialSendLayout = new QVBoxLayout(_SerialSendPage);
    _SerialSendLayout->setContentsMargins(30, 30, 30, 30);

    // ──── 标题 ────
    ElaText* title = new ElaText("串口单条命令发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    _SerialSendLayout->addWidget(title);

    // ──── 描述 ────
    ElaText* desc = new ElaText(
        "在此输入单条命令，通过串口发送到设备。\n"
        "发送和接收的结果将显示在下方日志区域。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    _SerialSendLayout->addWidget(desc);

    // ════════════════════════════════════════════════════════
    //  命令输入 GroupBox
    // ════════════════════════════════════════════════════════
    QGroupBox* inputGroup = new QGroupBox("命令输入");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(10);
    inputLayout->setContentsMargins(16, 20, 16, 16);

    m_singleSendInput = new ElaLineEdit();
    m_singleSendInput->setPlaceholderText("在此输入要发送的命令...");
    m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_singleSendBtn = new ElaPushButton("通过串口发送");
    m_singleSendBtn->setFixedSize(160, 42);
    m_singleSendBtn->setEnabled(false);          // 串口未打开时禁用

    m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_singleSendBtn);
    btnRow->addWidget(m_singleSendClearBtn);
    btnRow->addStretch();
    inputLayout->addWidget(m_singleSendInput);
    inputLayout->addLayout(btnRow);
    _SerialSendLayout->addWidget(inputGroup);

    // ════════════════════════════════════════════════════════
    //  日志区域（发送日志 | 接收日志 左右分栏）
    // ════════════════════════════════════════════════════════
    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);
    // ── 发送日志 ──
    QVBoxLayout* sendArea = new QVBoxLayout();
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    m_singleSendLog = new QListWidget();
    m_singleSendLog->setAlternatingRowColors(true);
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_singleSendLog);

    // ── 接收日志 ──
    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_singleRecvLog = new QListWidget();
    m_singleRecvLog->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_singleRecvLog);
    logRow->addLayout(sendArea, 1);   // stretch = 1，等宽
    logRow->addLayout(recvArea, 1);
    _SerialSendLayout->addLayout(logRow, 1);      // stretch = 1，日志区占据剩余空间

    connect(m_singleSendClearBtn, &ElaPushButton::clicked,
            this, &SerialPage::clearSingleSendLog);
}

void SerialPage::createExcelSendPage()
{
    _SerialExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_SerialExcelSendPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("串口 Excel 表格发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText* desc = new ElaText(
        "通过 Excel 表格批量加载命令，逐条通过串口发送到设备。\n"
        "支持自动比对返回值并统计发送结果。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    ElaText* tableLabel = new ElaText("读取到的 Excel 表格数据");
    tableLabel->setTextPixelSize(15);
    tableLabel->setTextStyle(ElaTextType::Subtitle);
    layout->addWidget(tableLabel);

    m_excelTableWidget = new QTableWidget();
    m_excelTableWidget->setColumnCount(3);
    m_excelTableWidget->setHorizontalHeaderLabels({
        "发送的命令", "正确的返回值", "到下一条命令的时间ms"
    });
    m_excelTableWidget->setColumnWidth(0, 250);
    m_excelTableWidget->setColumnWidth(1, 250);
    m_excelTableWidget->setColumnWidth(2, 200);
    m_excelTableWidget->setRowCount(8);
    m_excelTableWidget->setItem(0, 0, new QTableWidgetItem("等待读取 Excel 表格"));
    m_excelTableWidget->setAlternatingRowColors(true);
    layout->addWidget(m_excelTableWidget, 1);

    // ═══════════ 底部 ═══════════
    QVBoxLayout* bottomArea = new QVBoxLayout();
    bottomArea->setSpacing(12);

    // 发送次数
    QHBoxLayout* repeatRow = new QHBoxLayout();
    repeatRow->setSpacing(8);
    ElaText* repeatLabel = new ElaText("发送次数:");
    repeatLabel->setTextPixelSize(15);
    m_excelRepeatCount = new ElaLineEdit();
    m_excelRepeatCount->setFixedSize(400, 36);
    m_excelRepeatCount->setPlaceholderText("0 = 无限循环");
    m_excelRepeatCount->setText("0");
    ElaText* repeatHint = new ElaText("（0 表示一直循环发送，直到点击停止）");
    repeatHint->setTextPixelSize(15);
    repeatHint->setWordWrap(false);
    repeatHint->setStyleSheet("color: gray;");
    repeatRow->addWidget(repeatLabel);
    repeatRow->addWidget(m_excelRepeatCount);
    repeatRow->addWidget(repeatHint);
    repeatRow->addStretch();
    bottomArea->addLayout(repeatRow);

    // ──── 超时时间（新增） ────
    QHBoxLayout* timeoutRow = new QHBoxLayout();
    timeoutRow->setSpacing(8);
    ElaText* timeoutLabel = new ElaText("超时时间:");
    timeoutLabel->setTextPixelSize(15);
    m_excelTimeoutMs = new ElaLineEdit();
    m_excelTimeoutMs->setFixedSize(400, 36);
    m_excelTimeoutMs->setPlaceholderText("超时ms");
    m_excelTimeoutMs->setText("500");
    ElaText* timeoutHint = new ElaText("（超过此时间未收到回复则判定超时，发送下一条）");
    timeoutHint->setTextPixelSize(15);
    timeoutHint->setWordWrap(false);
    timeoutHint->setStyleSheet("color: gray;");
    timeoutRow->addWidget(timeoutLabel);
    timeoutRow->addWidget(m_excelTimeoutMs);
    timeoutRow->addWidget(timeoutHint);
    timeoutRow->addStretch();
    bottomArea->addLayout(timeoutRow);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(16);

    QGroupBox* fileGroup = new QGroupBox("① 文件准备");
    fileGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);
    fileLayout->setSpacing(10);
    m_excelOpenBtn = new ElaPushButton("打开 Excel 并读取");
    m_excelOpenBtn->setFixedSize(180, 40);
    m_excelOpenBtn->setEnabled(false);
    m_excelDownloadTplBtn = new ElaPushButton("下载示例模板");
    m_excelDownloadTplBtn->setFixedSize(160, 40);
    fileLayout->addWidget(m_excelOpenBtn);
    fileLayout->addWidget(m_excelDownloadTplBtn);
    fileLayout->addStretch();

    QGroupBox* sendGroup = new QGroupBox("② 发送控制");
    sendGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout* sendLayout = new QHBoxLayout(sendGroup);
    sendLayout->setSpacing(10);
    m_excelCaptureBtn = new ElaPushButton("读取返回值");
    m_excelCaptureBtn->setFixedSize(140, 40);
    m_excelCaptureBtn->setEnabled(false);
    m_excelSendBtn = new ElaPushButton("开始发送");
    m_excelSendBtn->setFixedSize(140, 40);
    m_excelSendBtn->setEnabled(false);
    m_excelStopBtn = new ElaPushButton("停止发送");
    m_excelStopBtn->setFixedSize(140, 40);
    m_excelStopBtn->setEnabled(false);
    sendLayout->addWidget(m_excelCaptureBtn);
    sendLayout->addWidget(m_excelSendBtn);
    sendLayout->addWidget(m_excelStopBtn);
    sendLayout->addStretch();

    btnRow->addWidget(fileGroup);
    btnRow->addWidget(sendGroup);
    btnRow->addStretch();
    bottomArea->addLayout(btnRow);
    layout->addLayout(bottomArea);
}

void SerialPage::createLogPage()
{
    _SerialLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_SerialLogPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);
    // ──── 标题 ────
    ElaText* title = new ElaText("串口 Excel 发送日志");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);
    // ──── 描述 ────
    ElaText* desc = new ElaText(
        "记录每次 Excel 表格发送的详细过程。\n"
        "包含发送的命令、设备返回值及时间戳。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);
    // ════════════════════════════════════════════════════════
    //  Stats 卡片行
    // ════════════════════════════════════════════════════════
    QHBoxLayout* cardRow = new QHBoxLayout();
    cardRow->setSpacing(16);
    m_logSentCountCard = new StatCard("总计发送", "0");
    m_logRecvCountCard = new StatCard("总计接收", "0");
    m_logStartTimeCard = new StatCard("开始时间", "--:--:--");

    cardRow->addWidget(m_logSentCountCard);
    cardRow->addWidget(m_logRecvCountCard);
    cardRow->addWidget(m_logStartTimeCard);
    cardRow->addStretch();

    m_logClearBtn = new ElaPushButton("清空日志");
    m_logClearBtn->setFixedSize(120, 38);
    cardRow->addWidget(m_logClearBtn);

    layout->addLayout(cardRow);
    // ════════════════════════════════════════════════════════
    //  发送日志 + 接收日志（左右分栏）
    // ════════════════════════════════════════════════════════
    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);
    // ──── 左侧：发送日志 ────
    QVBoxLayout* sendArea = new QVBoxLayout();
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    m_logSendList = new QListWidget();
    m_logSendList->setAlternatingRowColors(true);
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_logSendList);
    // ──── 右侧：接收日志 ────
    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_logRecvList = new QListWidget();
    m_logRecvList->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_logRecvList);
    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);
    layout->addLayout(logRow, 1);

    connect(m_logClearBtn, &ElaPushButton::clicked,
        this, &SerialPage::clearExcelSendLog);

}

// ═══════════════════════════════════════════════════════════════
//  错误日志页面
// ═══════════════════════════════════════════════════════════════
void SerialPage::createErrorLogPage()
{
    _SerialErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(_SerialErrorLogPage);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);
    // ──── 辅助 lambda ────
    auto createCard = [](QWidget* parent) -> QWidget* {
        QWidget* card = new QWidget(parent);
        card->setObjectName("errorCard");
        card->setStyleSheet(
            "QWidget#errorCard {"
            "  background: transparent;"
            "  border: 1px solid rgba(130,130,130,55);"
            "  border-radius: 10px;"
            "}"
        );
        return card;
    };
    auto cardTitle = [](const QString& text, QWidget* parent) -> ElaText* {
        ElaText* t = new ElaText(text, parent);
        t->setTextPixelSize(15);
        t->setTextStyle(ElaTextType::BodyStrong);
        return t;
    };
    // ──── 页面标题 ────
    ElaText* pageTitle = new ElaText("串口错误日志");
    pageTitle->setTextPixelSize(24);
    pageTitle->setTextStyle(ElaTextType::Title);
    root->addWidget(pageTitle);
    // ════════════════════════════════════════════════════════
    //  Card ① 统计概览（卡片式）
    // ════════════════════════════════════════════════════════
    QWidget* statsCard = createCard(_SerialErrorLogPage);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsCard);
    statsLayout->setContentsMargins(24, 20, 24, 20);
    statsLayout->setSpacing(16);
    m_errorTotalCard   = new StatCard("总错误",   "0");
    m_errorTimeoutCard = new StatCard("超时错误", "0");
    m_errorContentCard = new StatCard("内容错误", "0");
    statsLayout->addWidget(m_errorTotalCard);
    statsLayout->addWidget(m_errorTimeoutCard);
    statsLayout->addWidget(m_errorContentCard);
    statsLayout->addStretch();
    m_errorClearBtn = new ElaPushButton("清空记录");
    m_errorClearBtn->setFixedWidth(120);
    m_errorClearBtn->setMinimumHeight(36);
    statsLayout->addWidget(m_errorClearBtn);
    root->addWidget(statsCard);
    // ════════════════════════════════════════════════════════
    //  Card ② 错误列表
    // ════════════════════════════════════════════════════════
    QWidget* tableCard = createCard(_SerialErrorLogPage);
    QVBoxLayout* tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(20, 16, 20, 16);
    tableCardLayout->setSpacing(10);
    QHBoxLayout* tableHeader = new QHBoxLayout();
    tableHeader->addWidget(cardTitle("错误列表", tableCard));
    tableHeader->addStretch();
    ElaText* autoScrollLabel = new ElaText("自动滚动：");
    autoScrollLabel->setTextPixelSize(13);
    autoScrollLabel->setTextStyle(ElaTextType::Body);
    m_errorAutoScroll = new ElaToggleSwitch();
    m_errorAutoScroll->setIsToggled(true);
    tableHeader->addWidget(autoScrollLabel);
    tableHeader->addWidget(m_errorAutoScroll);
    tableCardLayout->addLayout(tableHeader);
    m_errorTable = new QTableWidget();
    m_errorTable->setColumnCount(6);
    m_errorTable->setHorizontalHeaderLabels(
        QStringList() << "序号" << "时间" << "错误类型"
                      << "发送命令" << "期望值" << "实际值");
    QHeaderView* hHeader = m_errorTable->horizontalHeader();
    m_errorTable->setColumnWidth(0, 50);
    m_errorTable->setColumnWidth(1, 100);
    m_errorTable->setColumnWidth(2, 80);
    hHeader->setSectionResizeMode(3, QHeaderView::Stretch);
    hHeader->setSectionResizeMode(4, QHeaderView::Stretch);
    hHeader->setSectionResizeMode(5, QHeaderView::Stretch);
    m_errorTable->setAlternatingRowColors(true);
    m_errorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_errorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_errorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_errorTable->setShowGrid(true);
    m_errorTable->verticalHeader()->setVisible(false);
    m_errorTable->setRowCount(1);
    m_errorTable->setItem(0, 0, new QTableWidgetItem("—"));
    m_errorTable->setItem(0, 1, new QTableWidgetItem("尚未记录错误"));
    m_errorTable->setSpan(0, 1, 1, 5);
    tableCardLayout->addWidget(m_errorTable, 1);
    root->addWidget(tableCard, 1);
    // ──── 信号连接 ────
    connect(m_errorClearBtn, &ElaPushButton::clicked, this, &SerialPage::clearErrors);
}

// ═══════════════════════════════════════════════════════════════
//  字节数组 → 错误日志显示文本（HEX或UTF8，跟随HEX勾选框）
// ═══════════════════════════════════════════════════════════════
static QString bytesToDisplayText(const QByteArray &data, bool isHexMode)
{
    if (data.isEmpty())
        return QString("—");
    if (isHexMode) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty())
            return text;
        else
            return data.toHex(' ').toUpper();   // 不可显示字符降级
    }
}

// ═══════════════════════════════════════════════════════════════
//  添加超时错误
// ═══════════════════════════════════════════════════════════════
void SerialPage::addTimeoutError(const QString &command, const QByteArray &expected)
{
    ++m_errorSeq;
    ++m_timeoutCount;

    int total = m_timeoutCount + m_contentCount;
    m_errorTotalCard->setValue(QString::number(total));
    m_errorTimeoutCard->setValue(QString::number(m_timeoutCount));

    if (m_errorTable->rowCount() == 1
        && m_errorTable->item(0, 1)
        && m_errorTable->item(0, 1)->text() == "尚未记录错误")
    {
        m_errorTable->setRowCount(0);
    }

    int row = m_errorTable->rowCount();
    m_errorTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    // 判断当前是否 HEX 模式
    bool hexMode = m_serialHexSendCheckBox && m_serialHexSendCheckBox->isChecked();

    m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_errorSeq)));
    m_errorTable->setItem(row, 1, new QTableWidgetItem(timeStr));
    m_errorTable->setItem(row, 2, new QTableWidgetItem("超时"));
    m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_errorTable->setItem(row, 4, new QTableWidgetItem(bytesToDisplayText(expected, hexMode)));
    m_errorTable->setItem(row, 5, new QTableWidgetItem("(无返回)"));

    for (int c = 0; c < 6; ++c) {
        QTableWidgetItem* it = m_errorTable->item(row, c);
        if (it) it->setForeground(QColor("#f39c12"));
    }

    while (m_errorTable->rowCount() > 1000)
        m_errorTable->removeRow(0);

    if (m_errorAutoScroll->getIsToggled())
        m_errorTable->scrollToBottom();
}

// ═══════════════════════════════════════════════════════════════
//  添加内容错误
// ═══════════════════════════════════════════════════════════════
void SerialPage::addContentError(const QString &command,
                                  const QByteArray &expected,
                                  const QByteArray &actual)
{
    ++m_errorSeq;
    ++m_contentCount;

    int total = m_timeoutCount + m_contentCount;
    m_errorTotalCard->setValue(QString::number(total));
    m_errorContentCard->setValue(QString::number(m_contentCount));

    if (m_errorTable->rowCount() == 1
        && m_errorTable->item(0, 1)
        && m_errorTable->item(0, 1)->text() == "尚未记录错误")
    {
        m_errorTable->setRowCount(0);
    }

    int row = m_errorTable->rowCount();
    m_errorTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    bool hexMode = m_serialHexSendCheckBox && m_serialHexSendCheckBox->isChecked();

    m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_errorSeq)));
    m_errorTable->setItem(row, 1, new QTableWidgetItem(timeStr));
    m_errorTable->setItem(row, 2, new QTableWidgetItem("内容错误"));
    m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_errorTable->setItem(row, 4, new QTableWidgetItem(bytesToDisplayText(expected, hexMode)));
    m_errorTable->setItem(row, 5, new QTableWidgetItem(bytesToDisplayText(actual,   hexMode)));

    for (int c = 0; c < 6; ++c) {
        QTableWidgetItem* it = m_errorTable->item(row, c);
        if (it) it->setForeground(QColor("#e74c3c"));
    }

    while (m_errorTable->rowCount() > 1000)
        m_errorTable->removeRow(0);

    if (m_errorAutoScroll->getIsToggled())
        m_errorTable->scrollToBottom();
}

// ═══════════════════════════════════════════════════════════════
//  清空错误记录
// ═══════════════════════════════════════════════════════════════
void SerialPage::clearErrors()
{
    m_errorSeq     = 0;
    m_timeoutCount = 0;
    m_contentCount = 0;
    m_errorTotalCard->setValue("0");
    m_errorTimeoutCard->setValue("0");
    m_errorContentCard->setValue("0");
    m_errorTable->clearContents();
    m_errorTable->setRowCount(1);
    m_errorTable->setItem(0, 0, new QTableWidgetItem("—"));
    m_errorTable->setItem(0, 1, new QTableWidgetItem("尚未记录错误"));
    m_errorTable->setSpan(0, 1, 1, 5);
}

// ═══════════════════════════════════════════════════════════════
//  清空单条发送日志
// ═══════════════════════════════════════════════════════════════
void SerialPage::clearSingleSendLog()
{
    m_singleSendLog->clear();
    m_singleRecvLog->clear();
}

// ═══════════════════════════════════════════════════════════════
//  清空表格发送日志 + 重置卡片
// ═══════════════════════════════════════════════════════════════
void SerialPage::clearExcelSendLog()
{
    // 清空日志列表
    m_logSendList->clear();
    m_logRecvList->clear();

    // 重置统计卡片
    m_logSentCountCard->setValue("0");
    m_logRecvCountCard->setValue("0");
    m_logStartTimeCard->setValue("--:--:--");

    if (m_serialWork)
        m_serialWork->resetRecvCount();
}

