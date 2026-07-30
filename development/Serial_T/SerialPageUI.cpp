// SerialPageUI.cpp
// 5 个 createXxxPage() 方法的完整实现
// ★ 新增：缓冲区超时输入框

#include "SerialPageUI.h"
#include "SerialPage.h"

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
#include <QSerialPortInfo>

SerialPageUI::SerialPageUI(SerialPage* page)
    : QObject(page), m_page(page)
{
}

// ═══════════════════════════════════════════════════════════════
//  页面：串口设置
// ═══════════════════════════════════════════════════════════════
void SerialPageUI::createSettingsPage() {
    m_page->_SerialSettingPage = new QWidget();
    QVBoxLayout *_SerialSettingLayout1 = new QVBoxLayout(m_page->_SerialSettingPage);
    _SerialSettingLayout1->setContentsMargins(30, 30, 30, 30);

    ElaText *_SerialSettingTitle = new ElaText("串口设置界面");
    _SerialSettingTitle->setTextPixelSize(24);
    _SerialSettingTitle->setTextStyle(ElaTextType::Title);

    _SerialSettingLayout1->addWidget(_SerialSettingTitle);

    QGroupBox* _SerialSettingGroup = new QGroupBox("串口参数");
    QGridLayout* grid = new QGridLayout(_SerialSettingGroup);
    grid->setSpacing(10);
    grid->setContentsMargins(30, 30, 30, 30);

    // ──── 第 0 行：串口端口号 | 波特率 ────
    ElaText* portLabel = new ElaText("串口端口号:");
    portLabel->setTextPixelSize(15);
    m_page->m_serialPortComboBox = new ElaComboBox();

    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        m_page->m_serialPortComboBox->addItem("无可用串口");
    } else {
        for (const QSerialPortInfo &info : portList)
            m_page->m_serialPortComboBox->addItem(info.portName());
    }

    ElaText* baudLabel = new ElaText("波特率:");
    baudLabel->setTextPixelSize(15);
    m_page->m_baudRateComboBox = new ElaComboBox();
    m_page->m_baudRateComboBox->addItems({"1200", "2400", "4800", "9600",
                                   "19200", "38400", "57600", "115200"});
    m_page->m_baudRateComboBox->setCurrentText("115200");

    grid->addWidget(portLabel,               0, 0);
    grid->addWidget(m_page->m_serialPortComboBox,    0, 1);
    grid->addWidget(baudLabel,               0, 2);
    grid->addWidget(m_page->m_baudRateComboBox,      0, 3);

    // ──── 第 1 行：数据位 | 停止位 ────
    ElaText* dataLabel = new ElaText("数据位:");
    dataLabel->setTextPixelSize(15);
    m_page->m_dataBitsComboBox = new ElaComboBox();
    m_page->m_dataBitsComboBox->addItems({"5", "6", "7", "8"});
    m_page->m_dataBitsComboBox->setCurrentText("8");
    ElaText* stopLabel = new ElaText("停止位:");
    stopLabel->setTextPixelSize(15);
    m_page->m_stopBitsComboBox = new ElaComboBox();
    m_page->m_stopBitsComboBox->addItems({"1", "1.5", "2"});
    m_page->m_stopBitsComboBox->setCurrentText("1");

    grid->addWidget(dataLabel,           1, 0);
    grid->addWidget(m_page->m_dataBitsComboBox,  1, 1);
    grid->addWidget(stopLabel,           1, 2);
    grid->addWidget(m_page->m_stopBitsComboBox,  1, 3);

    // ──── 第 2 行：校验位 | 串口状态 LED ────
    ElaText* parityLabel = new ElaText("校验位:");
    parityLabel->setTextPixelSize(15);
    m_page->m_parityComboBox = new ElaComboBox();
    m_page->m_parityComboBox->addItems({"None", "Even", "Odd", "Space", "Mark"});
    m_page->m_parityComboBox->setCurrentText("None");
    ElaText* statusLabel = new ElaText("串口状态:");
    statusLabel->setTextPixelSize(15);
    m_page->m_serialLED = new QLabel();
    LED::setLED(m_page->m_serialLED, 0, 16);

    grid->addWidget(parityLabel,         2, 0);
    grid->addWidget(m_page->m_parityComboBox,    2, 1);
    grid->addWidget(statusLabel,         2, 2);
    grid->addWidget(m_page->m_serialLED,         2, 3);

    // ──── 第 3 行：合并缓冲区勾选框 + 超时输入框 | HEX发送勾选框 ────
    m_page->m_serialBufferCheckBox = new ElaCheckBox("单条命令缓冲区读取延时(防止粘包)");
    m_page->m_serialBufferCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    // ★ 新增：超时时间输入框（放在水平布局中：输入框 + "ms" 标签）
    QWidget* timeoutContainer = new QWidget();
    QHBoxLayout* timeoutLayout = new QHBoxLayout(timeoutContainer);
    timeoutLayout->setContentsMargins(0, 0, 0, 0);
    timeoutLayout->setSpacing(4);

    ElaText* timeoutLabel = new ElaText("缓冲区读取延时(ms):");
    timeoutLabel->setTextPixelSize(14);

    m_page->m_bufferTimeoutEdit = new ElaLineEdit();

    m_page->m_bufferTimeoutEdit->setText("20");
    m_page->m_bufferTimeoutEdit->setPlaceholderText("ms");
    m_page->m_bufferTimeoutEdit->setAlignment(Qt::AlignCenter);

    timeoutLayout->addWidget(timeoutLabel);
    timeoutLayout->addWidget(m_page->m_bufferTimeoutEdit, 1);
    timeoutLayout->addStretch();

    m_page->m_serialHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_page->m_serialHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    grid->addWidget(m_page->m_serialBufferCheckBox,   3, 0);
    grid->addWidget(timeoutContainer,                 3, 2,1,2);
    grid->addWidget(m_page->m_serialHexSendCheckBox,  4, 0, 1, 2);

    // ──── 第 4 行：去除 \r\n 勾选框 ────
    m_page->m_serialStripCRLFCheckBox = new ElaCheckBox("比对时去除返回值中的 \\r\\n");
    m_page->m_serialStripCRLFCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_serialStripCRLFCheckBox, 4, 2, 1, 4);

    // ──── 第 5 行：粘包分割 ────
    m_page->m_serialSplitStickyCheckBox = new ElaCheckBox("启用粘包分割（按分隔符拆分返回值）");
    m_page->m_serialSplitStickyCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_serialSplitStickyCheckBox, 5, 0, 1, 2);

    m_page->m_serialSplitDelimiterComboBox = new ElaComboBox();
    m_page->m_serialSplitDelimiterComboBox->addItems({"\\n", "\\r", "\\r\\n"});
    m_page->m_serialSplitDelimiterComboBox->setCurrentIndex(0);
    m_page->m_serialSplitDelimiterComboBox->setStyleSheet("ElaComboBox { font-size: 14px; }");
    grid->addWidget(m_page->m_serialSplitDelimiterComboBox, 5, 2, 1, 2);

    // ──── 第 6 行：发送后缀 ────
    ElaText* suffixLabel = new ElaText("发送后缀:");
    suffixLabel->setTextPixelSize(15);
    m_page->m_suffixComboBox = new ElaComboBox();
    m_page->m_suffixComboBox->addItems({"无 (None)", "CR (\\r)", "LF (\\n)", "CRLF (\\r\\n)"});
    m_page->m_suffixComboBox->setCurrentIndex(0);

    grid->addWidget(suffixLabel,           6, 0);
    grid->addWidget(m_page->m_suffixComboBox,      6, 2, 1, 2);

    // ★ 第 7 行：区间判断（ASCII）
    m_page->m_serialAsciiRangeCheckBox = new ElaCheckBox("启用ASCII区间判断（如：期望3.00±0.5）");
    m_page->m_serialAsciiRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_serialAsciiRangeCheckBox, 7, 0, 1, 2);

    ElaText* asciiRangeLabel = new ElaText("ASCII区间值:");
    asciiRangeLabel->setTextPixelSize(15);
    m_page->m_serialAsciiRangeEdit = new ElaLineEdit();
    m_page->m_serialAsciiRangeEdit->setText("0.5");
    m_page->m_serialAsciiRangeEdit->setPlaceholderText("如 0.5 表示 ±0.5");
    grid->addWidget(asciiRangeLabel,          7, 2);
    grid->addWidget(m_page->m_serialAsciiRangeEdit,    7, 3);

    // ★ 第 8 行：HEX 区间判断（AN3.0 自动解析命令码）
    m_page->m_serialHexRangeCheckBox = new ElaCheckBox("启用HEX区间判断（AN3.0自动解析）");
    m_page->m_serialHexRangeCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_page->m_serialHexRangeCheckBox->setEnabled(false);   // 初始禁用，等勾选HEX发送后启用
    grid->addWidget(m_page->m_serialHexRangeCheckBox, 8, 0, 1, 2);

    ElaText* hexRangeLabel = new ElaText("HEX区间值:");
    hexRangeLabel->setTextPixelSize(15);
    m_page->m_serialHexRangeEdit = new ElaLineEdit();
    m_page->m_serialHexRangeEdit->setText("0.5");
    m_page->m_serialHexRangeEdit->setPlaceholderText("如 0.5 表示 ±0.5");
    m_page->m_serialHexRangeEdit->setEnabled(false);       // 初始禁用，勾选区间判断后启用
    grid->addWidget(hexRangeLabel,             8, 2);
    grid->addWidget(m_page->m_serialHexRangeEdit,      8, 3);

    // 第 9行：AN3.0 产品系列选择
    ElaText* productLabel = new ElaText("产品系列:");
    productLabel->setTextPixelSize(15);
    m_page->m_serialProductComboBox = new ElaComboBox();
    m_page->m_serialProductComboBox->addItems({"RGL系列 (交流源载)", "EVH系列 (直流电源)"});
    m_page->m_serialProductComboBox->setCurrentIndex(0);
    m_page->m_serialProductComboBox->setStyleSheet("ElaComboBox { font-size: 14px; }");
    grid->addWidget(productLabel,                    9, 0);
    grid->addWidget(m_page->m_serialProductComboBox, 9, 2, 1, 2);

    // ──── 第 10 行：打开/关闭按钮 ────
    m_page->m_openSerialButton = new ElaPushButton("打开串口");
    m_page->m_openSerialButton->setFixedHeight(35);
    m_page->m_closeSerialButton = new ElaPushButton("关闭串口");
    m_page->m_closeSerialButton->setFixedHeight(35);
    m_page->m_closeSerialButton->setEnabled(false);
    grid->addWidget(m_page->m_openSerialButton,       10, 0);
    grid->addWidget(m_page->m_closeSerialButton,      10, 1);

    _SerialSettingLayout1->addWidget(_SerialSettingGroup);
    _SerialSettingLayout1->addStretch();
}

// ═══════════════════════════════════════════════════════════════
//  页面：单条发送
// ═══════════════════════════════════════════════════════════════
void SerialPageUI::createSendPage() {
    m_page->_SerialSendPage = new QWidget();
    QVBoxLayout *_SerialSendLayout = new QVBoxLayout(m_page->_SerialSendPage);
    _SerialSendLayout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("串口单条命令发送");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    _SerialSendLayout->addWidget(title);

    ElaText* desc = new ElaText(
        "在此输入单条命令，通过串口发送到设备。\n"
        "发送和接收的结果将显示在下方日志区域。");
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    _SerialSendLayout->addWidget(desc);

    QGroupBox* inputGroup = new QGroupBox("命令输入");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(10);
    inputLayout->setContentsMargins(16, 20, 16, 16);

    m_page->m_singleSendInput = new ElaLineEdit();
    m_page->m_singleSendInput->setPlaceholderText("在此输入要发送的命令...");
    m_page->m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_page->m_singleSendBtn = new ElaPushButton("通过串口发送");
    m_page->m_singleSendBtn->setFixedSize(160, 42);
    m_page->m_singleSendBtn->setEnabled(false);

    m_page->m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_page->m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_page->m_singleSendBtn);
    btnRow->addWidget(m_page->m_singleSendClearBtn);
    btnRow->addStretch();
    inputLayout->addWidget(m_page->m_singleSendInput);
    inputLayout->addLayout(btnRow);
    _SerialSendLayout->addWidget(inputGroup);

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
    _SerialSendLayout->addLayout(logRow, 1);

    connect(m_page->m_singleSendClearBtn, &ElaPushButton::clicked,
            m_page, &SerialPage::clearSingleSendLog);
}

// ═══════════════════════════════════════════════════════════════
//  页面：Excel 表格发送
// ═══════════════════════════════════════════════════════════════
void SerialPageUI::createExcelSendPage()
{
    m_page->_SerialExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_SerialExcelSendPage);
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
void SerialPageUI::createLogPage()
{
    m_page->_SerialLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_page->_SerialLogPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText* title = new ElaText("串口 Excel 发送日志");
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
void SerialPageUI::createErrorLogPage()
{
    m_page->_SerialErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(m_page->_SerialErrorLogPage);
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
    ElaText* pageTitle = new ElaText("串口错误日志");
    pageTitle->setTextPixelSize(24);
    pageTitle->setTextStyle(ElaTextType::Title);
    root->addWidget(pageTitle);

    QWidget* statsCard = createCard(m_page->_SerialErrorLogPage);
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

    QWidget* tableCard = createCard(m_page->_SerialErrorLogPage);
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
            m_page, &SerialPage::clearErrors);
}
