#include "CANPageUI.h"

#include "CANPage.h"
#include "CANWork.h"
#include "CANFrameUtils.h"

#include "ElaText.h"
#include "ElaWindow.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QTableWidget>

CANPageUI::CANPageUI(CANPage *page)
    : QObject(page), m_page(page)
{
}

void CANPageUI::createSettingsPage()
{
    m_page->_CANSettingPage = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(m_page->_CANSettingPage);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(14);

    ElaText *title = new ElaText(QStringLiteral("CAN 设置"));
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    root->addWidget(title);

    QGroupBox *group = new QGroupBox(QStringLiteral("CANFD200 / ZLGCAN 参数"));
    QGridLayout *grid = new QGridLayout(group);
    grid->setContentsMargins(20, 24, 20, 20);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    auto addLabel = [&](int row, int col, const QString &text) {
        ElaText *label = new ElaText(text);
        label->setTextPixelSize(15);
        grid->addWidget(label, row, col);
    };

    addLabel(0, 0, QStringLiteral("设备类型:"));
    m_page->m_deviceTypeEdit = new ElaLineEdit();
    m_page->m_deviceTypeEdit->setText(QStringLiteral("41"));
    m_page->m_deviceTypeEdit->setPlaceholderText(QStringLiteral("CANFD200 = 41"));
    grid->addWidget(m_page->m_deviceTypeEdit, 0, 1);

    addLabel(0, 2, QStringLiteral("设备索引:"));
    m_page->m_deviceIndexEdit = new ElaLineEdit();
    m_page->m_deviceIndexEdit->setText(QStringLiteral("0"));
    grid->addWidget(m_page->m_deviceIndexEdit, 0, 3);

    addLabel(1, 0, QStringLiteral("CAN通道:"));
    m_page->m_canIndexEdit = new ElaLineEdit();
    m_page->m_canIndexEdit->setText(QStringLiteral("0"));
    grid->addWidget(m_page->m_canIndexEdit, 1, 1);

    addLabel(1, 2, QStringLiteral("帧模式:"));
    m_page->m_canModeComboBox = new ElaComboBox();
    m_page->m_canModeComboBox->addItems({QStringLiteral("CAN FD"), QStringLiteral("CAN")});
    m_page->m_canModeComboBox->setCurrentIndex(0);
    grid->addWidget(m_page->m_canModeComboBox, 1, 3);

    addLabel(2, 0, QStringLiteral("仲裁域波特率:"));
    m_page->m_baudRateComboBox = new ElaComboBox();
    m_page->m_baudRateComboBox->addItems({
        QStringLiteral("1000000"), QStringLiteral("800000"), QStringLiteral("500000"),
        QStringLiteral("250000"), QStringLiteral("125000"), QStringLiteral("100000"),
        QStringLiteral("50000"), QStringLiteral("20000"), QStringLiteral("10000"),
        QStringLiteral("5000")
    });
    m_page->m_baudRateComboBox->setCurrentText(QStringLiteral("500000"));
    grid->addWidget(m_page->m_baudRateComboBox, 2, 1);

    addLabel(2, 2, QStringLiteral("数据域波特率:"));
    m_page->m_dbitBaudComboBox = new ElaComboBox();
    m_page->m_dbitBaudComboBox->addItems({
        QStringLiteral("8000000"), QStringLiteral("5000000"), QStringLiteral("4000000"),
        QStringLiteral("2000000"), QStringLiteral("1000000"), QStringLiteral("500000")
    });
    m_page->m_dbitBaudComboBox->setCurrentText(QStringLiteral("2000000"));
    grid->addWidget(m_page->m_dbitBaudComboBox, 2, 3);

    m_page->m_terminalResCheckBox = new ElaCheckBox(QStringLiteral("使能内置终端电阻"));
    m_page->m_terminalResCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_page->m_terminalResCheckBox, 3, 0, 1, 2);

    m_page->m_openButton = new ElaPushButton(QStringLiteral("打开 CAN"));
    m_page->m_openButton->setFixedHeight(38);
    m_page->m_closeButton = new ElaPushButton(QStringLiteral("关闭 CAN"));
    m_page->m_closeButton->setFixedHeight(38);
    m_page->m_closeButton->setEnabled(false);
    m_page->m_canLED = new QLabel();
    m_page->m_canLED->setFixedSize(16, 16);
    LED::setLED(m_page->m_canLED, 0, 16);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addWidget(m_page->m_openButton);
    btnRow->addWidget(m_page->m_closeButton);
    btnRow->addWidget(m_page->m_canLED);
    btnRow->addStretch();
    grid->addLayout(btnRow, 4, 0, 1, 4);

    root->addWidget(group);

    ElaText *hint = new ElaText(
        QStringLiteral("帧格式说明：CAN ID 使用十六进制，数据使用十六进制并以空格分隔，例如：100 11 22 33 44。")
        + QStringLiteral("\n数据超过 8 字节自动按 CAN FD 发送，也可用 FD 关键字强制 FD，例如：100 FD 01 02 03。")
        + QStringLiteral("\nCAN FD 数据最多 64 字节，默认开启 BRS。"));
    hint->setTextPixelSize(13);
    hint->setWordWrap(true);
    root->addWidget(hint);
    root->addStretch();
}

void CANPageUI::createSendPage()
{
    m_page->_CANSendPage = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(m_page->_CANSendPage);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(12);

    ElaText *title = new ElaText(QStringLiteral("CAN 单条发送"));
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    root->addWidget(title);

    QGroupBox *inputGroup = new QGroupBox(QStringLiteral("帧输入"));
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(10);
    inputLayout->setContentsMargins(16, 20, 16, 16);

    m_page->m_singleSendInput = new ElaLineEdit();
    m_page->m_singleSendInput->setPlaceholderText(
        QStringLiteral("CAN ID(十六进制) [FD] 数据HEX，如：100 FD 11 22 33 44"));
    m_page->m_singleSendInput->setFixedHeight(42);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);
    m_page->m_singleSendBtn = new ElaPushButton(QStringLiteral("通过 CAN 发送"));
    m_page->m_singleSendBtn->setFixedSize(160, 42);
    m_page->m_singleSendBtn->setEnabled(false);
    m_page->m_singleSendClearBtn = new ElaPushButton(QStringLiteral("清空发送日志"));
    m_page->m_singleSendClearBtn->setFixedSize(120, 38);
    m_page->m_singleLogPauseBtn = new ElaPushButton(QStringLiteral("暂停日志"));
    m_page->m_singleLogPauseBtn->setFixedSize(120, 38);
    m_page->m_singleLogLED = new QLabel();
    m_page->m_singleLogLED->setFixedSize(14, 14);
    LED::setLED(m_page->m_singleLogLED, 2, 14);

    btnRow->addWidget(m_page->m_singleSendBtn);
    btnRow->addWidget(m_page->m_singleSendClearBtn);
    btnRow->addWidget(m_page->m_singleLogPauseBtn);
    btnRow->addWidget(m_page->m_singleLogLED);
    btnRow->addStretch();

    inputLayout->addWidget(m_page->m_singleSendInput);
    inputLayout->addLayout(btnRow);
    root->addWidget(inputGroup);

    QHBoxLayout *logRow = new QHBoxLayout();
    logRow->setSpacing(12);
    QVBoxLayout *sendArea = new QVBoxLayout();
    ElaText *sendLabel = new ElaText(QStringLiteral("发送日志"));
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    m_page->m_singleSendLog = new QListWidget();
    m_page->m_singleSendLog->setAlternatingRowColors(true);
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_page->m_singleSendLog);

    QVBoxLayout *recvArea = new QVBoxLayout();
    ElaText *recvLabel = new ElaText(QStringLiteral("接收日志"));
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_page->m_singleRecvLog = new QListWidget();
    m_page->m_singleRecvLog->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_page->m_singleRecvLog);

    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);
    root->addLayout(logRow, 1);

    connect(m_page->m_singleSendClearBtn, &ElaPushButton::clicked,
            m_page, &CANPage::clearSingleSendLog);
}

void CANPageUI::createExcelSendPage()
{
    m_page->_CANExcelSendPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_page->_CANExcelSendPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText(QStringLiteral("CAN Excel 表格发送"));
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText *desc = new ElaText(
        QStringLiteral("通过 Excel 表格批量加载 CAN 帧，逐条发送到总线。\n")
        + QStringLiteral("支持自动比对返回帧并统计发送结果。"));
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    ElaText *tableLabel = new ElaText(QStringLiteral("读取到的 Excel 表格数据"));
    tableLabel->setTextPixelSize(15);
    tableLabel->setTextStyle(ElaTextType::Subtitle);
    layout->addWidget(tableLabel);

    m_page->m_excelTableWidget = new QTableWidget();
    m_page->m_excelTableWidget->setColumnCount(3);
    m_page->m_excelTableWidget->setHorizontalHeaderLabels({
        QStringLiteral("发送的CAN帧(ID 数据HEX)"),
        QStringLiteral("正确的返回值"),
        QStringLiteral("到下一条命令的时间ms")
    });
    m_page->m_excelTableWidget->setColumnWidth(0, 250);
    m_page->m_excelTableWidget->setColumnWidth(1, 250);
    m_page->m_excelTableWidget->setColumnWidth(2, 200);
    m_page->m_excelTableWidget->setRowCount(8);
    m_page->m_excelTableWidget->setItem(0, 0, new QTableWidgetItem(QStringLiteral("等待读取 Excel 表格")));
    m_page->m_excelTableWidget->setAlternatingRowColors(true);
    layout->addWidget(m_page->m_excelTableWidget, 1);

    QVBoxLayout *bottomArea = new QVBoxLayout();
    bottomArea->setSpacing(12);

    QHBoxLayout *repeatRow = new QHBoxLayout();
    repeatRow->setSpacing(8);
    ElaText *repeatLabel = new ElaText(QStringLiteral("发送次数:"));
    repeatLabel->setTextPixelSize(15);
    m_page->m_excelRepeatCount = new ElaLineEdit();
    m_page->m_excelRepeatCount->setFixedSize(400, 36);
    m_page->m_excelRepeatCount->setPlaceholderText(QStringLiteral("0 = 无限循环"));
    m_page->m_excelRepeatCount->setText(QStringLiteral("0"));
    ElaText *repeatHint = new ElaText(QStringLiteral("（0 表示一直循环发送，直到点击停止）"));
    repeatHint->setTextPixelSize(15);
    repeatHint->setWordWrap(false);
    repeatHint->setStyleSheet("color: gray;");
    repeatRow->addWidget(repeatLabel);
    repeatRow->addWidget(m_page->m_excelRepeatCount);
    repeatRow->addWidget(repeatHint);
    repeatRow->addStretch();
    bottomArea->addLayout(repeatRow);

    QHBoxLayout *timeoutRow = new QHBoxLayout();
    timeoutRow->setSpacing(8);
    ElaText *timeoutLabel = new ElaText(QStringLiteral("超时时间:"));
    timeoutLabel->setTextPixelSize(15);
    m_page->m_excelTimeoutMs = new ElaLineEdit();
    m_page->m_excelTimeoutMs->setFixedSize(400, 36);
    m_page->m_excelTimeoutMs->setPlaceholderText(QStringLiteral("超时ms"));
    m_page->m_excelTimeoutMs->setText(QStringLiteral("500"));
    ElaText *timeoutHint = new ElaText(QStringLiteral("（超过此时间未收到回复帧则判定超时，发送下一条）"));
    timeoutHint->setTextPixelSize(15);
    timeoutHint->setWordWrap(false);
    timeoutHint->setStyleSheet("color: gray;");
    timeoutRow->addWidget(timeoutLabel);
    timeoutRow->addWidget(m_page->m_excelTimeoutMs);
    timeoutRow->addWidget(timeoutHint);
    timeoutRow->addStretch();
    bottomArea->addLayout(timeoutRow);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(16);

    QGroupBox *fileGroup = new QGroupBox(QStringLiteral("① 文件准备"));
    fileGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout *fileLayout = new QHBoxLayout(fileGroup);
    fileLayout->setSpacing(10);
    m_page->m_excelOpenBtn = new ElaPushButton(QStringLiteral("打开 Excel 并读取"));
    m_page->m_excelOpenBtn->setFixedSize(180, 40);
    m_page->m_excelOpenBtn->setEnabled(false);
    m_page->m_excelDownloadTplBtn = new ElaPushButton(QStringLiteral("下载示例模板"));
    m_page->m_excelDownloadTplBtn->setFixedSize(160, 40);
    fileLayout->addWidget(m_page->m_excelOpenBtn);
    fileLayout->addWidget(m_page->m_excelDownloadTplBtn);
    fileLayout->addStretch();

    QGroupBox *sendGroup = new QGroupBox(QStringLiteral("② 发送控制"));
    sendGroup->setStyleSheet("QGroupBox { font-size: 15px; font-weight: bold; }");
    QHBoxLayout *sendLayout = new QHBoxLayout(sendGroup);
    sendLayout->setSpacing(10);
    m_page->m_excelCaptureBtn = new ElaPushButton(QStringLiteral("读取返回值"));
    m_page->m_excelCaptureBtn->setFixedSize(140, 40);
    m_page->m_excelCaptureBtn->setEnabled(false);
    m_page->m_excelSendBtn = new ElaPushButton(QStringLiteral("开始发送"));
    m_page->m_excelSendBtn->setFixedSize(140, 40);
    m_page->m_excelSendBtn->setEnabled(false);
    m_page->m_excelStopBtn = new ElaPushButton(QStringLiteral("停止发送"));
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

void CANPageUI::createLogPage()
{
    m_page->_CANLogPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_page->_CANLogPage);
    layout->setSpacing(16);
    layout->setContentsMargins(30, 30, 30, 30);

    ElaText *title = new ElaText(QStringLiteral("CAN Excel 发送日志"));
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    layout->addWidget(title);

    ElaText *desc = new ElaText(
        QStringLiteral("记录每次 CAN 表格发送的详细过程。\n")
        + QStringLiteral("包含发送帧、接收帧及时间戳。"));
    desc->setTextPixelSize(15);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    QGridLayout *cardRow = new QGridLayout();
    cardRow->setSpacing(12);

    m_page->m_logSentCountCard = new StatCard(QStringLiteral("总计发送"), QStringLiteral("0"));
    m_page->m_logRecvCountCard = new StatCard(QStringLiteral("总计接收"), QStringLiteral("0"));
    m_page->m_logStartTimeCard = new StatCard(QStringLiteral("开始时间"), QStringLiteral("--:--:--"));

    cardRow->addWidget(m_page->m_logSentCountCard, 0, 0, 2, 1);
    cardRow->addWidget(m_page->m_logRecvCountCard, 0, 1, 2, 1);
    cardRow->addWidget(m_page->m_logStartTimeCard, 0, 2, 2, 1);
    cardRow->setColumnStretch(0, 1);
    cardRow->setColumnStretch(1, 1);
    cardRow->setColumnStretch(2, 1);

    QVBoxLayout *btnCol = new QVBoxLayout();
    btnCol->setSpacing(8);
    m_page->m_logClearBtn = new ElaPushButton(QStringLiteral("清空日志"));
    m_page->m_logClearBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_page->m_logClearBtn);
    m_page->m_logPauseBtn = new ElaPushButton(QStringLiteral("暂停日志"));
    m_page->m_logPauseBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_page->m_logPauseBtn);
    btnCol->addStretch();
    cardRow->addLayout(btnCol, 0, 3, 2, 1, Qt::AlignTop);

    layout->addLayout(cardRow);

    QHBoxLayout *logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    QVBoxLayout *sendArea = new QVBoxLayout();
    QHBoxLayout *sendTitleRow = new QHBoxLayout();
    sendTitleRow->setSpacing(8);
    ElaText *sendLabel = new ElaText(QStringLiteral("发送日志"));
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

    QVBoxLayout *recvArea = new QVBoxLayout();
    ElaText *recvLabel = new ElaText(QStringLiteral("接收日志"));
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

void CANPageUI::createErrorLogPage()
{
    m_page->_CANErrorLogPage = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(m_page->_CANErrorLogPage);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);

    auto createCard = [](QWidget *parent) -> QWidget * {
        QWidget *card = new QWidget(parent);
        card->setObjectName("errorCard");
        card->setStyleSheet(
            "QWidget#errorCard {"
            "  background: transparent;"
            "  border: 1px solid rgba(130,130,130,55);"
            "  border-radius: 10px;"
            "}");
        return card;
    };
    auto cardTitle = [](const QString &text, QWidget *parent) -> ElaText * {
        ElaText *t = new ElaText(text, parent);
        t->setTextPixelSize(15);
        t->setTextStyle(ElaTextType::BodyStrong);
        return t;
    };

    ElaText *pageTitle = new ElaText(QStringLiteral("CAN 错误日志"));
    pageTitle->setTextPixelSize(24);
    pageTitle->setTextStyle(ElaTextType::Title);
    root->addWidget(pageTitle);

    QWidget *statsCard = createCard(m_page->_CANErrorLogPage);
    QHBoxLayout *statsLayout = new QHBoxLayout(statsCard);
    statsLayout->setContentsMargins(24, 20, 24, 20);
    statsLayout->setSpacing(16);
    m_page->m_errorTotalCard = new StatCard(QStringLiteral("总错误"), QStringLiteral("0"));
    m_page->m_errorTimeoutCard = new StatCard(QStringLiteral("超时错误"), QStringLiteral("0"));
    m_page->m_errorContentCard = new StatCard(QStringLiteral("内容错误"), QStringLiteral("0"));
    statsLayout->addWidget(m_page->m_errorTotalCard);
    statsLayout->addWidget(m_page->m_errorTimeoutCard);
    statsLayout->addWidget(m_page->m_errorContentCard);
    statsLayout->addStretch();

    QWidget *buttonColumn = new QWidget(statsCard);
    QVBoxLayout *buttonColumnLayout = new QVBoxLayout(buttonColumn);
    buttonColumnLayout->setContentsMargins(0, 0, 0, 0);
    buttonColumnLayout->setSpacing(8);
    m_page->m_errorClearBtn = new ElaPushButton(QStringLiteral("清空记录"));
    m_page->m_errorClearBtn->setFixedWidth(150);
    m_page->m_errorClearBtn->setMinimumHeight(36);
    m_page->m_errorExportBtn = new ElaPushButton(QStringLiteral("导出数据到excel"));
    m_page->m_errorExportBtn->setFixedWidth(150);
    m_page->m_errorExportBtn->setMinimumHeight(36);
    buttonColumnLayout->addWidget(m_page->m_errorClearBtn);
    buttonColumnLayout->addWidget(m_page->m_errorExportBtn);
    statsLayout->addWidget(buttonColumn, 0, Qt::AlignVCenter);
    root->addWidget(statsCard);

    QWidget *tableCard = createCard(m_page->_CANErrorLogPage);
    QVBoxLayout *tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(20, 16, 20, 16);
    tableCardLayout->setSpacing(10);
    QHBoxLayout *tableHeader = new QHBoxLayout();
    tableHeader->addWidget(cardTitle(QStringLiteral("错误列表"), tableCard));
    tableHeader->addStretch();
    ElaText *autoScrollLabel = new ElaText(QStringLiteral("自动滚动："));
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
        QStringList() << QStringLiteral("序号") << QStringLiteral("时间") << QStringLiteral("错误类型")
                      << QStringLiteral("发送命令") << QStringLiteral("期望值") << QStringLiteral("实际值"));
    QHeaderView *hHeader = m_page->m_errorTable->horizontalHeader();
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
    m_page->m_errorTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("—")));
    m_page->m_errorTable->setItem(0, 1, new QTableWidgetItem(QStringLiteral("尚未记录错误")));
    m_page->m_errorTable->setSpan(0, 1, 1, 5);
    tableCardLayout->addWidget(m_page->m_errorTable, 1);
    root->addWidget(tableCard, 1);

    connect(m_page->m_errorClearBtn, &ElaPushButton::clicked,
            m_page, &CANPage::clearErrors);
    connect(m_page->m_errorExportBtn, &ElaPushButton::clicked,
            m_page, &CANPage::exportErrorsToExcel);
}
