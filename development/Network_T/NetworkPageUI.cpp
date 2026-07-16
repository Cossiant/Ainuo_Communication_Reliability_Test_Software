// NetworkPageUI.cpp
// 5 个 createXxxPage() 方法的完整实现

#include "NetworkPageUI.h"
#include "NetworkPage.h"

#include "ElaText.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaToggleSwitch.h"
#include "../Other_T/LED.h"
#include "../Other_T/StatCard.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QListWidget>
#include <QHeaderView>

NetworkPageUI::NetworkPageUI(NetworkPage* page)
    : QObject(page), m_page(page)
{
}

// ═══════════════════════════════════════════════════════════════
//  页面：网络设置
// ═══════════════════════════════════════════════════════════════
void NetworkPageUI::createSettingsPage() {
    m_page->_NetworkSettingPage = new QWidget();
    QVBoxLayout *_NetworkSettingLayout1 = new QVBoxLayout(m_page->_NetworkSettingPage);
    _NetworkSettingLayout1->setContentsMargins(30, 30, 30, 30);

    ElaText *_NetworkSettingTitle = new ElaText("网络设置界面");
    _NetworkSettingTitle->setTextPixelSize(24);
    _NetworkSettingTitle->setTextStyle(ElaTextType::Title);

    _NetworkSettingLayout1->addWidget(_NetworkSettingTitle);

    QGroupBox* _NetworkSettingGroup = new QGroupBox("网络参数");
    QGridLayout* grid = new QGridLayout(_NetworkSettingGroup);
    grid->setSpacing(10);
    grid->setContentsMargins(30, 30, 30, 30);

    ElaText* ipLabel = new ElaText("IP 地址:");
    ipLabel->setTextPixelSize(15);
    m_page->m_ipAddressEdit = new ElaLineEdit();
    m_page->m_ipAddressEdit->setPlaceholderText("例如: 192.168.1.100");
    m_page->m_ipAddressEdit->setText("192.168.1.100");

    ElaText* portLabel = new ElaText("端口号:");
    portLabel->setTextPixelSize(15);
    m_page->m_portEdit = new ElaLineEdit();
    m_page->m_portEdit->setPlaceholderText("例如: 5025");
    m_page->m_portEdit->setText("5025");

    grid->addWidget(ipLabel,               0, 0);
    grid->addWidget(m_page->m_ipAddressEdit,       0, 1);
    grid->addWidget(portLabel,             0, 2);
    grid->addWidget(m_page->m_portEdit,            0, 3);

    ElaText* statusLabel = new ElaText("网络状态:");
    statusLabel->setTextPixelSize(15);
    m_page->m_networkLED = new QLabel();
    LED::setLED(m_page->m_networkLED, 0, 16);

    grid->addWidget(statusLabel,           1, 0);
    grid->addWidget(m_page->m_networkLED,          1, 1);

    m_page->m_networkHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_page->m_networkHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_page->m_nagleCheckBox = new ElaCheckBox("禁用 Nagle 算法");
    m_page->m_nagleCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_networkHexSendCheckBox,  2, 0, 1, 2);
    grid->addWidget(m_page->m_nagleCheckBox,           2, 2, 1, 2);

    // 去除 \r\n 勾选框
    m_page->m_networkStripCRLFCheckBox = new ElaCheckBox("比对时去除返回值中的 \\r\\n");
    m_page->m_networkStripCRLFCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_networkStripCRLFCheckBox, 3, 0, 1, 4);

    // 粘包分割
    m_page->m_networkSplitStickyCheckBox = new ElaCheckBox("启用粘包分割（按分隔符拆分返回值）");
    m_page->m_networkSplitStickyCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_networkSplitStickyCheckBox, 4, 0, 1, 2);

    m_page->m_networkSplitDelimiterComboBox = new ElaComboBox();
    m_page->m_networkSplitDelimiterComboBox->addItems({"\\n", "\\r", "\\r\\n"});
    m_page->m_networkSplitDelimiterComboBox->setCurrentIndex(0);
    m_page->m_networkSplitDelimiterComboBox->setStyleSheet("ElaComboBox { font-size: 14px; }");
    grid->addWidget(m_page->m_networkSplitDelimiterComboBox, 4, 2, 1, 2);

    // 发送后缀
    ElaText* suffixLabel = new ElaText("发送后缀: ");
    suffixLabel->setTextPixelSize(15);
    m_page->m_suffixComboBox = new ElaComboBox();
    m_page->m_suffixComboBox->addItems({"无 (None)", "CR (\\r)", "LF (\\n)", "CRLF (\\r\\n)"});
    m_page->m_suffixComboBox->setCurrentIndex(0);

    grid->addWidget(suffixLabel,           5, 0);
    grid->addWidget(m_page->m_suffixComboBox,      5, 2, 1, 2);

    // ★ 第 6 行：区间判断
    m_page->m_networkAsciiRangeCheckBox = new ElaCheckBox("启用ASCII区间判断（如：期望3.00±0.5）");
    m_page->m_networkAsciiRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_networkAsciiRangeCheckBox, 6, 0, 1, 2);

    ElaText* asciiRangeLabel = new ElaText("ASCII区间值:");
    asciiRangeLabel->setTextPixelSize(15);
    m_page->m_networkAsciiRangeEdit = new ElaLineEdit();
    m_page->m_networkAsciiRangeEdit->setText("0.5");
    m_page->m_networkAsciiRangeEdit->setPlaceholderText("如 0.5 表示 ±0.5");
    grid->addWidget(asciiRangeLabel,           6, 2);
    grid->addWidget(m_page->m_networkAsciiRangeEdit,    6, 3);

    // ★ 第 7 行：HEX 区间判断（预留）
    m_page->m_networkHexRangeCheckBox = new ElaCheckBox("启用HEX区间判断（预留）");
    m_page->m_networkHexRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_page->m_networkHexRangeCheckBox->setEnabled(false);
    grid->addWidget(m_page->m_networkHexRangeCheckBox, 7, 0, 1, 2);

    ElaText* hexRangeLabel = new ElaText("HEX区间值:");
    hexRangeLabel->setTextPixelSize(15);
    m_page->m_networkHexRangeEdit = new ElaLineEdit();
    m_page->m_networkHexRangeEdit->setText("0.5");
    m_page->m_networkHexRangeEdit->setPlaceholderText("预留");
    m_page->m_networkHexRangeEdit->setEnabled(false);
    grid->addWidget(hexRangeLabel,             7, 2);
    grid->addWidget(m_page->m_networkHexRangeEdit,      7, 3);

    // 连接按钮
    m_page->m_openNetworkButton = new ElaPushButton("连接网络");
    m_page->m_openNetworkButton->setFixedHeight(35);
    m_page->m_closeNetworkButton = new ElaPushButton("断开网络");
    m_page->m_closeNetworkButton->setFixedHeight(35);
    m_page->m_closeNetworkButton->setEnabled(false);
    grid->addWidget(m_page->m_openNetworkButton,       8, 1);
    grid->addWidget(m_page->m_closeNetworkButton,      8, 3);

    _NetworkSettingLayout1->addWidget(_NetworkSettingGroup);
    _NetworkSettingLayout1->addStretch();
}


// ═══════════════════════════════════════════════════════════════
//  页面：单条发送
// ═══════════════════════════════════════════════════════════════
void NetworkPageUI::createSendPage() {
    m_page->_NetworkSendPage = new QWidget();
    QVBoxLayout *_NetworkSendLayout = new QVBoxLayout(m_page->_NetworkSendPage);
    _NetworkSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("网口单条命令发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    _NetworkSendLayout->addWidget(title);

    ElaText* desc = new ElaText(
        "在此输入单条命令，通过网口发送到设备。\n"
        "发送和接收的结果将显示在下方日志区域。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    _NetworkSendLayout->addWidget(desc);

    QGroupBox* inputGroup = new QGroupBox("命令输入");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(10);
    inputLayout->setContentsMargins(16, 20, 16, 16);

    m_page->m_singleSendInput = new ElaLineEdit();
    m_page->m_singleSendInput->setPlaceholderText("在此输入要发送的命令...");
    m_page->m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_page->m_singleSendBtn = new ElaPushButton("通过网口发送");
    m_page->m_singleSendBtn->setFixedSize(160, 42);
    m_page->m_singleSendBtn->setEnabled(false);

    m_page->m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_page->m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_page->m_singleSendBtn);
    btnRow->addWidget(m_page->m_singleSendClearBtn);
    btnRow->addStretch();
    inputLayout->addWidget(m_page->m_singleSendInput);
    inputLayout->addLayout(btnRow);
    _NetworkSendLayout->addWidget(inputGroup);

    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);
    QVBoxLayout* sendArea = new QVBoxLayout();
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    m_page->m_singleSendLog = new QListWidget();
    m_page->m_singleSendLog->setAlternatingRowColors(true);
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_page->m_singleSendLog);

    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_page->m_singleRecvLog = new QListWidget();
    m_page->m_singleRecvLog->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_page->m_singleRecvLog);
    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);
    _NetworkSendLayout->addLayout(logRow, 1);

    connect(m_page->m_singleSendClearBtn, &ElaPushButton::clicked,
            m_page, &NetworkPage::clearSingleSendLog);
}

// ═══════════════════════════════════════════════════════════════
//  页面：Excel 表格发送
// ═══════════════════════════════════════════════════════════════
void NetworkPageUI::createExcelSendPage()
{
    m_page->_NetworkExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_NetworkExcelSendPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("网口 Excel 表格发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText* desc = new ElaText(
        "通过 Excel 表格批量加载命令，逐条通过网口发送到设备。\n"
        "支持自动比对返回值并统计发送结果。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    ElaText* tableLabel = new ElaText("读取到的 Excel 表格数据");
    tableLabel->setTextPixelSize(15);
    tableLabel->setTextStyle(ElaTextType::Subtitle);
    layout->addWidget(tableLabel);

    m_page->m_excelTableWidget = new QTableWidget();
    m_page->m_excelTableWidget->setColumnCount(3);
    m_page->m_excelTableWidget->setHorizontalHeaderLabels({
        "发送的命令", "正确的返回值", "到下一条命令的时间ms"
    });
    m_page->m_excelTableWidget->setColumnWidth(0, 250);
    m_page->m_excelTableWidget->setColumnWidth(1, 250);
    m_page->m_excelTableWidget->setColumnWidth(2, 200);
    m_page->m_excelTableWidget->setRowCount(8);
    m_page->m_excelTableWidget->setItem(0, 0, new QTableWidgetItem("等待读取 Excel 表格"));
    m_page->m_excelTableWidget->setAlternatingRowColors(true);
    layout->addWidget(m_page->m_excelTableWidget, 1);

    QVBoxLayout* bottomArea = new QVBoxLayout();
    bottomArea->setSpacing(12);

    QHBoxLayout* repeatRow = new QHBoxLayout();
    repeatRow->setSpacing(8);
    ElaText* repeatLabel = new ElaText("发送次数:");
    repeatLabel->setTextPixelSize(15);
    m_page->m_excelRepeatCount = new ElaLineEdit();
    m_page->m_excelRepeatCount->setFixedSize(400, 36);
    m_page->m_excelRepeatCount->setPlaceholderText("0 = 无限循环");
    m_page->m_excelRepeatCount->setText("0");
    ElaText* repeatHint = new ElaText("（0 表示一直循环发送，直到点击停止）");
    repeatHint->setTextPixelSize(15);
    repeatHint->setWordWrap(false);
    repeatHint->setStyleSheet("color: gray;");
    repeatRow->addWidget(repeatLabel);
    repeatRow->addWidget(m_page->m_excelRepeatCount);
    repeatRow->addWidget(repeatHint);
    repeatRow->addStretch();
    bottomArea->addLayout(repeatRow);

    QHBoxLayout* timeoutRow = new QHBoxLayout();
    timeoutRow->setSpacing(8);
    ElaText* timeoutLabel = new ElaText("超时时间:");
    timeoutLabel->setTextPixelSize(15);
    m_page->m_excelTimeoutMs = new ElaLineEdit();
    m_page->m_excelTimeoutMs->setFixedSize(400, 36);
    m_page->m_excelTimeoutMs->setPlaceholderText("超时ms");
    m_page->m_excelTimeoutMs->setText("500");
    ElaText* timeoutHint = new ElaText("（超过此时间未收到回复则判定超时，发送下一条）");
    timeoutHint->setTextPixelSize(15);
    timeoutHint->setWordWrap(false);
    timeoutHint->setStyleSheet("color: gray;");
    timeoutRow->addWidget(timeoutLabel);
    timeoutRow->addWidget(m_page->m_excelTimeoutMs);
    timeoutRow->addWidget(timeoutHint);
    timeoutRow->addStretch();
    bottomArea->addLayout(timeoutRow);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(16);

    QGroupBox* fileGroup = new QGroupBox("① 文件准备");
    fileGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);
    fileLayout->setSpacing(10);
    m_page->m_excelOpenBtn = new ElaPushButton("打开 Excel 并读取");
    m_page->m_excelOpenBtn->setFixedSize(180, 40);
    m_page->m_excelOpenBtn->setEnabled(false);
    m_page->m_excelDownloadTplBtn = new ElaPushButton("下载示例模板");
    m_page->m_excelDownloadTplBtn->setFixedSize(160, 40);
    fileLayout->addWidget(m_page->m_excelOpenBtn);
    fileLayout->addWidget(m_page->m_excelDownloadTplBtn);
    fileLayout->addStretch();

    QGroupBox* sendGroup = new QGroupBox("② 发送控制");
    sendGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout* sendLayout = new QHBoxLayout(sendGroup);
    sendLayout->setSpacing(10);
    m_page->m_excelCaptureBtn = new ElaPushButton("读取返回值");
    m_page->m_excelCaptureBtn->setFixedSize(140, 40);
    m_page->m_excelCaptureBtn->setEnabled(false);
    m_page->m_excelSendBtn = new ElaPushButton("开始发送");
    m_page->m_excelSendBtn->setFixedSize(140, 40);
    m_page->m_excelSendBtn->setEnabled(false);
    m_page->m_excelStopBtn = new ElaPushButton("停止发送");
    m_page->m_excelStopBtn->setFixedSize(140, 40);
    m_page->m_excelStopBtn->setEnabled(false);
    sendLayout->addWidget(m_page->m_excelCaptureBtn);
    sendLayout->addWidget(m_page->m_excelSendBtn);
    sendLayout->addWidget(m_page->m_excelStopBtn);
    sendLayout->addStretch();

    btnRow->addWidget(fileGroup);
    btnRow->addWidget(sendGroup);
    btnRow->addStretch();
    bottomArea->addLayout(btnRow);
    layout->addLayout(bottomArea);
}

// ═══════════════════════════════════════════════════════════════
//  页面：发送日志
// ═══════════════════════════════════════════════════════════════
void NetworkPageUI::createLogPage()
{
    m_page->_NetworkLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_NetworkLogPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("网口 Excel 发送日志");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);
    ElaText* desc = new ElaText(
        "记录每次 Excel 表格发送的详细过程。\n"
        "包含发送的命令、设备返回值及时间戳。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    QGridLayout* cardRow = new QGridLayout();
    cardRow->setSpacing(12);

    m_page->m_logSentCountCard = new StatCard("总计发送", "0");
    m_page->m_logRecvCountCard = new StatCard("总计接收", "0");
    m_page->m_logStartTimeCard = new StatCard("开始时间", "--:--:--");

    cardRow->addWidget(m_page->m_logSentCountCard, 0, 0, 2, 1);
    cardRow->addWidget(m_page->m_logRecvCountCard, 0, 1, 2, 1);
    cardRow->addWidget(m_page->m_logStartTimeCard, 0, 2, 2, 1);
    cardRow->setColumnStretch(0, 1);
    cardRow->setColumnStretch(1, 1);
    cardRow->setColumnStretch(2, 1);

    QVBoxLayout* btnCol = new QVBoxLayout();
    btnCol->setSpacing(8);

    m_page->m_logClearBtn = new ElaPushButton("清空日志");
    m_page->m_logClearBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_page->m_logClearBtn);

    m_page->m_logPauseBtn = new ElaPushButton("暂停日志");
    m_page->m_logPauseBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_page->m_logPauseBtn);

    btnCol->addStretch();
    cardRow->addLayout(btnCol, 0, 3, 2, 1, Qt::AlignTop);

    layout->addLayout(cardRow);

    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    QVBoxLayout* sendArea = new QVBoxLayout();
    QHBoxLayout* sendTitleRow = new QHBoxLayout();
    sendTitleRow->setSpacing(8);
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    sendTitleRow->addWidget(sendLabel);

    m_page->m_logLED = new QLabel();
    m_page->m_logLED->setFixedSize(14, 14);
    LED::setLED(m_page->m_logLED, 2, 14);
    sendTitleRow->addWidget(m_page->m_logLED);
    sendTitleRow->addStretch();

    m_page->m_logSendList = new QListWidget();
    m_page->m_logSendList->setAlternatingRowColors(true);
    sendArea->addLayout(sendTitleRow);
    sendArea->addWidget(m_page->m_logSendList);

    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_page->m_logRecvList = new QListWidget();
    m_page->m_logRecvList->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_page->m_logRecvList);

    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);
    layout->addLayout(logRow, 1);
}

// ═══════════════════════════════════════════════════════════════
//  页面：错误日志
// ═══════════════════════════════════════════════════════════════
void NetworkPageUI::createErrorLogPage()
{
    m_page->_NetworkErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(m_page->_NetworkErrorLogPage);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);
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
    ElaText* pageTitle = new ElaText("网口错误日志");
    pageTitle->setTextPixelSize(24);
    pageTitle->setTextStyle(ElaTextType::Title);
    root->addWidget(pageTitle);

    QWidget* statsCard = createCard(m_page->_NetworkErrorLogPage);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsCard);
    statsLayout->setContentsMargins(24, 20, 24, 20);
    statsLayout->setSpacing(16);
    m_page->m_errorTotalCard   = new StatCard("总错误",   "0");
    m_page->m_errorTimeoutCard = new StatCard("超时错误", "0");
    m_page->m_errorContentCard = new StatCard("内容错误", "0");
    statsLayout->addWidget(m_page->m_errorTotalCard);
    statsLayout->addWidget(m_page->m_errorTimeoutCard);
    statsLayout->addWidget(m_page->m_errorContentCard);
    statsLayout->addStretch();
    m_page->m_errorClearBtn = new ElaPushButton("清空记录");
    m_page->m_errorClearBtn->setFixedWidth(120);
    m_page->m_errorClearBtn->setMinimumHeight(36);
    statsLayout->addWidget(m_page->m_errorClearBtn);
    root->addWidget(statsCard);

    QWidget* tableCard = createCard(m_page->_NetworkErrorLogPage);
    QVBoxLayout* tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(20, 16, 20, 16);
    tableCardLayout->setSpacing(10);
    QHBoxLayout* tableHeader = new QHBoxLayout();
    tableHeader->addWidget(cardTitle("错误列表", tableCard));
    tableHeader->addStretch();
    ElaText* autoScrollLabel = new ElaText("自动滚动：");
    autoScrollLabel->setTextPixelSize(13);
    autoScrollLabel->setTextStyle(ElaTextType::Body);
    m_page->m_errorAutoScroll = new ElaToggleSwitch();
    m_page->m_errorAutoScroll->setIsToggled(true);
    tableHeader->addWidget(autoScrollLabel);
    tableHeader->addWidget(m_page->m_errorAutoScroll);
    tableCardLayout->addLayout(tableHeader);
    m_page->m_errorTable = new QTableWidget();
    m_page->m_errorTable->setColumnCount(6);
    m_page->m_errorTable->setHorizontalHeaderLabels(
        QStringList() << "序号" << "时间" << "错误类型"
                      << "发送命令" << "期望值" << "实际值");
    QHeaderView* hHeader = m_page->m_errorTable->horizontalHeader();
    m_page->m_errorTable->setColumnWidth(0, 50);
    m_page->m_errorTable->setColumnWidth(1, 100);
    m_page->m_errorTable->setColumnWidth(2, 80);
    hHeader->setSectionResizeMode(3, QHeaderView::Stretch);
    hHeader->setSectionResizeMode(4, QHeaderView::Stretch);
    hHeader->setSectionResizeMode(5, QHeaderView::Stretch);
    m_page->m_errorTable->setAlternatingRowColors(true);
    m_page->m_errorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_page->m_errorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_page->m_errorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_page->m_errorTable->setShowGrid(true);
    m_page->m_errorTable->verticalHeader()->setVisible(false);
    m_page->m_errorTable->setRowCount(1);
    m_page->m_errorTable->setItem(0, 0, new QTableWidgetItem("—"));
    m_page->m_errorTable->setItem(0, 1, new QTableWidgetItem("尚未记录错误"));
    m_page->m_errorTable->setSpan(0, 1, 1, 5);
    tableCardLayout->addWidget(m_page->m_errorTable, 1);
    root->addWidget(tableCard, 1);

    connect(m_page->m_errorClearBtn, &ElaPushButton::clicked,
            m_page, &NetworkPage::clearErrors);
}
