// GPIBPageUI.cpp
// 5 个 createXxxPage() 方法的完整实现

#include "GPIBPageUI.h"
#include "GPIBPage.h"

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

GPIBPageUI::GPIBPageUI(GPIBPage* page)
    : QObject(page), m_page(page)
{
}

// ═══════════════════════════════════════════════════════════════
//  页面：GPIB 设置
// ═══════════════════════════════════════════════════════════════
void GPIBPageUI::createSettingsPage() {
    m_page->_GpibSettingPage = new QWidget();
    QVBoxLayout *_GpibSettingLayout = new QVBoxLayout(m_page->_GpibSettingPage);
    _GpibSettingLayout->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText("GPIB 设置界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    _GpibSettingLayout->addWidget(title);

    QGroupBox* group = new QGroupBox("GPIB 参数");
    QGridLayout* grid = new QGridLayout(group);
    grid->setSpacing(10);
    grid->setContentsMargins(30, 30, 30, 30);

    // ──── 第 0 行：板卡号 | 主地址 ────
    ElaText* boardLabel = new ElaText("板卡号:");
    boardLabel->setTextPixelSize(15);
    m_page->m_boardIndexEdit = new ElaLineEdit();
    m_page->m_boardIndexEdit->setText("0");
    m_page->m_boardIndexEdit->setPlaceholderText("GPIB 板卡号 (0-3)");

    ElaText* primaryLabel = new ElaText("主地址:");
    primaryLabel->setTextPixelSize(15);
    m_page->m_primaryAddrEdit = new ElaLineEdit();
    m_page->m_primaryAddrEdit->setText("1");
    m_page->m_primaryAddrEdit->setPlaceholderText("仪器主地址 (0-30)");

    grid->addWidget(boardLabel,          0, 0);
    grid->addWidget(m_page->m_boardIndexEdit,    0, 1);
    grid->addWidget(primaryLabel,        0, 2);
    grid->addWidget(m_page->m_primaryAddrEdit,   0, 3);

    // ──── 第 1 行：副地址 | 超时 ────
    ElaText* secondaryLabel = new ElaText("副地址:");
    secondaryLabel->setTextPixelSize(15);
    m_page->m_secondaryAddrEdit = new ElaLineEdit();
    m_page->m_secondaryAddrEdit->setText("0");
    m_page->m_secondaryAddrEdit->setPlaceholderText("副地址 (0=不使用)");

    ElaText* timeoutLabel = new ElaText("超时(ms):");
    timeoutLabel->setTextPixelSize(15);
    m_page->m_timeoutEdit = new ElaLineEdit();
    m_page->m_timeoutEdit->setText("499");
    m_page->m_timeoutEdit->setPlaceholderText("超时时间 (ms)");

    grid->addWidget(secondaryLabel,       1, 0);
    grid->addWidget(m_page->m_secondaryAddrEdit,  1, 1);
    grid->addWidget(timeoutLabel,         1, 2);
    grid->addWidget(m_page->m_timeoutEdit,        1, 3);

    // ──── 第 2 行：结束字符 | GPIB 状态 LED ────
    ElaText* termCharLabel = new ElaText("结束字符:");
    termCharLabel->setTextPixelSize(15);
    m_page->m_termCharEdit = new ElaLineEdit();
    m_page->m_termCharEdit->setText("\\n");
    m_page->m_termCharEdit->setPlaceholderText("如 \\n 表示换行");

    ElaText* statusLabel = new ElaText("GPIB 状态:");
    statusLabel->setTextPixelSize(15);
    m_page->m_gpibLED = new QLabel();
    LED::setLED(m_page->m_gpibLED, 0, 16);

    grid->addWidget(termCharLabel,    2, 0);
    grid->addWidget(m_page->m_termCharEdit,   2, 1);
    grid->addWidget(statusLabel,      2, 2);
    grid->addWidget(m_page->m_gpibLED,        2, 3);

    // ──── 第 3 行：勾选框 ────
    m_page->m_termCharEnabledCheckBox = new ElaCheckBox("启用结束字符检测");
    m_page->m_termCharEnabledCheckBox->setChecked(true);
    m_page->m_termCharEnabledCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    m_page->m_sendEndEnabledCheckBox = new ElaCheckBox("发送时附加 EOI 信号");
    m_page->m_sendEndEnabledCheckBox->setChecked(true);
    m_page->m_sendEndEnabledCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    grid->addWidget(m_page->m_termCharEnabledCheckBox, 3, 0, 1, 2);
    grid->addWidget(m_page->m_sendEndEnabledCheckBox,  3, 2, 1, 2);

    // ──── 第 4 行：HEX 发送勾选框、发送后缀选择 ────
    m_page->m_gpibHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_page->m_gpibHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_gpibHexSendCheckBox, 4, 0, 1, 2);

    ElaText* suffixLabel = new ElaText("发送后缀:");
    suffixLabel->setTextPixelSize(15);
    m_page->m_suffixComboBox = new ElaComboBox();
    m_page->m_suffixComboBox->addItems({"无 (None)", "CR (\\r)", "LF (\\n)", "CRLF (\\r\\n)"});
    m_page->m_suffixComboBox->setCurrentIndex(0);

    grid->addWidget(suffixLabel,       4, 2);
    grid->addWidget(m_page->m_suffixComboBox,  4, 3);

    // ──── 第 5 行：去除 \r\n 勾选框 ────
    m_page->m_gpibStripCRLFCheckBox = new ElaCheckBox("比对时去除返回值中的 \\r\\n");
    m_page->m_gpibStripCRLFCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_gpibStripCRLFCheckBox, 5, 0, 1, 4);

    // ★ 第 6 行：区间判断（ASCII）
    m_page->m_gpibAsciiRangeCheckBox = new ElaCheckBox("启用ASCII区间判断（如：期望3.00±0.5）");
    m_page->m_gpibAsciiRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_gpibAsciiRangeCheckBox, 6, 0, 1, 2);

    ElaText* asciiRangeLabel = new ElaText("ASCII区间值:");
    asciiRangeLabel->setTextPixelSize(15);
    m_page->m_gpibAsciiRangeEdit = new ElaLineEdit();
    m_page->m_gpibAsciiRangeEdit->setText("0.5");
    m_page->m_gpibAsciiRangeEdit->setPlaceholderText("如 0.5 表示 ±0.5");
    grid->addWidget(asciiRangeLabel,      6, 2);
    grid->addWidget(m_page->m_gpibAsciiRangeEdit,  6, 3);

    // ★ 第 7 行：HEX 区间判断（预留）
    m_page->m_gpibHexRangeCheckBox = new ElaCheckBox("启用HEX区间判断（预留）");
    m_page->m_gpibHexRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_page->m_gpibHexRangeCheckBox->setEnabled(false);
    grid->addWidget(m_page->m_gpibHexRangeCheckBox, 7, 0, 1, 2);

    ElaText* hexRangeLabel = new ElaText("HEX区间值:");
    hexRangeLabel->setTextPixelSize(15);
    m_page->m_gpibHexRangeEdit = new ElaLineEdit();
    m_page->m_gpibHexRangeEdit->setText("0.5");
    m_page->m_gpibHexRangeEdit->setPlaceholderText("预留");
    m_page->m_gpibHexRangeEdit->setEnabled(false);
    grid->addWidget(hexRangeLabel,        7, 2);
    grid->addWidget(m_page->m_gpibHexRangeEdit,   7, 3);

    // ──── 第 8 行：打开/关闭按钮 ────
    m_page->m_openGpibButton = new ElaPushButton("打开 GPIB");
    m_page->m_openGpibButton->setFixedHeight(35);
    m_page->m_closeGpibButton = new ElaPushButton("关闭 GPIB");
    m_page->m_closeGpibButton->setFixedHeight(35);
    m_page->m_closeGpibButton->setEnabled(false);
    grid->addWidget(m_page->m_openGpibButton,   8, 1);
    grid->addWidget(m_page->m_closeGpibButton,  8, 3);

    _GpibSettingLayout->addWidget(group);
    _GpibSettingLayout->addStretch();
}


// ═══════════════════════════════════════════════════════════════
//  页面：单条发送
// ═══════════════════════════════════════════════════════════════
void GPIBPageUI::createSendPage() {
    m_page->_GpibSendPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_page->_GpibSendPage);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("GPIB 单条命令发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText* desc = new ElaText(
        "在此输入单条 SCPI 命令，通过 GPIB 发送到仪器。\n"
        "提示：如需查看仪器返回值，请使用「表格发送」页面进行查询。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    QGroupBox* inputGroup = new QGroupBox("命令输入");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(10);
    inputLayout->setContentsMargins(16, 20, 16, 16);

    m_page->m_singleSendInput = new ElaLineEdit();
    m_page->m_singleSendInput->setPlaceholderText("在此输入要发送的 SCPI 命令，如 *IDN? ...");
    m_page->m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_page->m_singleSendBtn = new ElaPushButton("通过 GPIB 发送");
    m_page->m_singleSendBtn->setFixedSize(160, 42);
    m_page->m_singleSendBtn->setEnabled(false);

    m_page->m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_page->m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_page->m_singleSendBtn);
    btnRow->addWidget(m_page->m_singleSendClearBtn);
    btnRow->addStretch();

    inputLayout->addWidget(m_page->m_singleSendInput);
    inputLayout->addLayout(btnRow);
    layout->addWidget(inputGroup);

    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    QVBoxLayout* leftCol = new QVBoxLayout();
    ElaText* sendLogLabel = new ElaText("发送日志:");
    sendLogLabel->setTextPixelSize(14);
    m_page->m_singleSendLog = new QListWidget();
    leftCol->addWidget(sendLogLabel);
    leftCol->addWidget(m_page->m_singleSendLog);

    QVBoxLayout* rightCol = new QVBoxLayout();
    ElaText* recvLogLabel = new ElaText("接收日志:");
    recvLogLabel->setTextPixelSize(14);
    m_page->m_singleRecvLog = new QListWidget();
    rightCol->addWidget(recvLogLabel);
    rightCol->addWidget(m_page->m_singleRecvLog);

    logRow->addLayout(leftCol);
    logRow->addLayout(rightCol);
    layout->addLayout(logRow);

    connect(m_page->m_singleSendClearBtn, &ElaPushButton::clicked,
            m_page, &GPIBPage::clearSingleSendLog);
}

// ═══════════════════════════════════════════════════════════════
//  页面：表格发送
// ═══════════════════════════════════════════════════════════════
void GPIBPageUI::createExcelSendPage() {
    m_page->_GpibExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_GpibExcelSendPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("GPIB Excel 表格发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText* desc = new ElaText(
        "通过 Excel 表格批量加载命令，逐条通过 GPIB 发送到仪器。\n"
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
void GPIBPageUI::createLogPage() {
    m_page->_GpibLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_GpibLogPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("GPIB Excel 发送日志");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);
    ElaText* desc = new ElaText(
        "记录每次 Excel 表格发送的详细过程。\n"
        "包含发送的命令、仪器返回值及时间戳。");
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
void GPIBPageUI::createErrorLogPage() {
    m_page->_GpibErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(m_page->_GpibErrorLogPage);
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
    ElaText* pageTitle = new ElaText("GPIB 错误日志");
    pageTitle->setTextPixelSize(24);
    pageTitle->setTextStyle(ElaTextType::Title);
    root->addWidget(pageTitle);

    QWidget* statsCard = createCard(m_page->_GpibErrorLogPage);
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

    QWidget* tableCard = createCard(m_page->_GpibErrorLogPage);
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
            m_page, &GPIBPage::clearErrors);
}
