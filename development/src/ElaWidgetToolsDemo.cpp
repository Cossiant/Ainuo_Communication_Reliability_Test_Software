#include "../include/ElaWidgetToolsDemo.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QDebug>

#include "ElaText.h"
#include "ElaPushButton.h"
#include "ElaComboBox.h"
#include "ElaCheckBox.h"
#include "ElaLineEdit.h"
#include "ElaToggleSwitch.h"
#include "ElaApplication.h"
#include "ElaAcrylicUrlCard.h"

// ═══════════════════════════════════════════════════════════════
//  构造函数
// ═══════════════════════════════════════════════════════════════
ElaWidgetToolsDemo::ElaWidgetToolsDemo(QWidget *parent)
    : ElaWindow(parent)
{
    // 注册元类型（不变）
    qRegisterMetaType<QHostAddress>("QHostAddress");
    qRegisterMetaType<QSerialPort::DataBits>("QSerialPort::DataBits");
    qRegisterMetaType<QSerialPort::Parity>("QSerialPort::Parity");
    qRegisterMetaType<QSerialPort::StopBits>("QSerialPort::StopBits");
    qRegisterMetaType<QSerialPort::FlowControl>("QSerialPort::FlowControl");

    // 创建子线程
    m_networkThread = new QThread(this);
    m_excelSendThread = new QThread(this);
    m_serialThread = new QThread(this);
    m_savedataThread = new QThread(this);
    m_validatorThread = new QThread(this);

    m_mainPage = new MainPage(this ,this);
    m_serialPage = new SerialPage(this, this);
    m_networkPage = new NetworkPage(this,this);
    m_CANPage = new CANPage(this,this);
    m_GPIBPage = new GPIBPage(this);
    m_USERPage = new USERPage(this,this);
    // initWindow();
}

ElaWidgetToolsDemo::~ElaWidgetToolsDemo()
{
    // 线程安全退出
    if (m_networkThread->isRunning()) { m_networkThread->quit(); m_networkThread->wait(); }
    if (m_excelSendThread->isRunning()) { m_excelSendThread->quit(); m_excelSendThread->wait(); }
    if (m_serialThread->isRunning()) { m_serialThread->quit(); m_serialThread->wait(); }
    if (m_savedataThread->isRunning()) { m_savedataThread->quit(); m_savedataThread->wait(); }
    if (m_validatorThread->isRunning()) { m_validatorThread->quit(); m_validatorThread->wait(); }
}

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::initPages()
{
    createSettingsPage();
    createSingleSendPage();
    createErrorLogPage();
    //这里createDataPage必须要在createErrorLogPage下面，因为错误统计变量是在createErrorLogPage创建的，而createDataPage只是调用！
    //如果先createDataPage会出现空指针访问直接闪退！
    createDataPage();
    createDebugPage();

    // ──── 关于页面 ────
    _aboutPage = new QWidget();
    QVBoxLayout* aboutLayout = new QVBoxLayout(_aboutPage);
    aboutLayout->setContentsMargins(30, 30, 30, 30);

    ElaText* aboutTitle = new ElaText("Ainuo 通用通讯可靠性测试软件V3.1");
    aboutTitle->setTextPixelSize(24);
    aboutTitle->setTextStyle(ElaTextType::Title);

    ElaText* aboutInfo = new ElaText(
        "版本: v3.1.2\n"
        "作者: Cossiant\n\n"
        "基于 ElaWidgetTools 现代化 UI 框架\n"
        "基于 QXlsx 高性能 excel 读写框架\n"
        "支持网络 (TCP) 和串口通讯\n"
        "支持 Excel 命令批量发送与返回值验证");
    aboutInfo->setTextPixelSize(14);

    aboutLayout->addWidget(aboutTitle);
    aboutLayout->addSpacing(12);
    aboutLayout->addWidget(aboutInfo);
    aboutLayout->addStretch();
}


// ═══════════════════════════════════════════════════════════════
//  初始化：导航节点
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::initNavigation()
{
    addPageNode("单条发送", _singleSendPage, ElaIconType::Terminal);
    addPageNode("数据收发", _dataPage, ElaIconType::PaperPlane);
    addPageNode("错误日志", _errorLogPage, ElaIconType::CircleExclamation);
    addPageNode("通讯设置", _settingsPage, ElaIconType::Gear);
    addPageNode("软件调试", _debugPage, ElaIconType::Bug);
    addPageNode("关于软件", _aboutPage, ElaIconType::CircleInfo);
}

// ═══════════════════════════════════════════════════════════════
//  初始化：窗口外观配置
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::initWindowConfig()
{
    setNavigationBarDisplayMode(ElaNavigationType::Auto);
    setNavigationBarWidth(240);

    setWindowButtonFlags(
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint |
        ElaAppBarType::RouteBackButtonHint |
        ElaAppBarType::NavigationButtonHint |
        ElaAppBarType::StayTopButtonHint
    );

    setIsAllowPageOpenInNewWindow(false);
    moveToCenter();
}

// ═══════════════════════════════════════════════════════════════
//  总入口
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::initWindow()
{
    resize(1200, 750);
    setUserInfoCardTitle("Ainuo 通讯可靠性");
    setUserInfoCardSubTitle("Excel SCPI Sender");
    setUserInfoCardVisible(true);
    setWindowTitle("Ainuo 通用通讯可靠性测试软件V3.1.2");

    initPages();
    initNavigation();
    initWindowConfig();
}
// ═══════════════════════════════════════════════════════════════
//  数据收发页面
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::createDataPage()
{
    _dataPage = new QWidget();
    //一定要记得创建excelreader变量！否则就会引发崩溃
    m_excelReader = new ExcelReader();

    QVBoxLayout* layout = new QVBoxLayout(_dataPage);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 12, 16, 12);

    // ════════════ 上部：表格 + 收发日志 ════════════
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(10);

    // --- Excel 表格 ---
    QVBoxLayout* tableArea = new QVBoxLayout();
    m_excelReadLabel = new ElaText("读取到的 Excel 表格数据");
    m_excelReadLabel->setTextPixelSize(13);
    m_excelTableWidget = new QTableWidget();
    m_excelTableWidget->setColumnCount(2);
    m_excelTableWidget->setColumnWidth(1, 250);
    m_excelTableWidget->setHorizontalHeaderLabels({"需发送的命令", "正确的返回值"});
    m_excelTableWidget->setRowCount(10);
    m_excelTableWidget->setItem(0, 0, new QTableWidgetItem("等待读取excel表格"));
    tableArea->addWidget(m_excelReadLabel);
    tableArea->addWidget(m_excelTableWidget);

    // --- 发送日志 ---
    m_sendLabel = new ElaText("发送的数据");
    m_sendLabel->setTextPixelSize(13);
    m_sendListWidget = new QListWidget();

    // --- 接收日志 ---
    m_receiveLabel = new ElaText("接收的数据");
    m_receiveLabel->setTextPixelSize(13);
    m_receiveListWidget = new QListWidget();

    topRow->addLayout(tableArea, 3);

    QVBoxLayout* logArea = new QVBoxLayout();
    logArea->setSpacing(6);
    logArea->addWidget(m_sendLabel);
    logArea->addWidget(m_sendListWidget, 1);
    logArea->addWidget(m_receiveLabel);
    logArea->addWidget(m_receiveListWidget, 1);
    topRow->addLayout(logArea, 2);

    // ════════════ 统计信息栏 ════════════
    QHBoxLayout* statsRow = new QHBoxLayout();
    statsRow->setSpacing(16);
    m_sentCountLabel = new ElaText("总计发送:");
    m_sentCountLabel->setTextPixelSize(13);
    m_sentCountDisplayLabel = new ElaText("0");
    m_sentCountDisplayLabel->setTextPixelSize(13);
    m_errorCountLabel = new ElaText("返回错误:");
    m_errorCountLabel->setTextPixelSize(13);
    m_dataPageErrorCountLabel = new ElaText("0");      // ← 独立副本
    m_dataPageErrorCountLabel->setTextPixelSize(13);   //   与发送计数一致
    m_errorTimeOutLabel = new ElaText("超时错误:");
    m_errorTimeOutLabel->setTextPixelSize(13);
    m_dataPageErrorTimeOutLabel = new ElaText("0");    // ← 独立副本
    m_dataPageErrorTimeOutLabel->setTextPixelSize(13); //   与发送计数一致
    m_timePrecisionLabel = new ElaText("时间精度:");
    m_timePrecisionLabel->setTextPixelSize(13);
    m_timePrecisionComboBox = new ElaComboBox();
    m_timePrecisionComboBox->addItems({"毫秒 (hh:mm:ss.zzz)", "微秒 (hh:mm:ss.zzzzzz)"});
    m_timePrecisionComboBox->setCurrentIndex(0);
    statsRow->addWidget(m_sentCountLabel);
    statsRow->addWidget(m_sentCountDisplayLabel);
    statsRow->addSpacing(16);
    statsRow->addWidget(m_errorCountLabel);
    statsRow->addWidget(m_dataPageErrorCountLabel);        // ← 用新变量
    statsRow->addSpacing(16);
    statsRow->addWidget(m_errorTimeOutLabel);
    statsRow->addWidget(m_dataPageErrorTimeOutLabel);      // ← 用新变量
    statsRow->addSpacing(16);
    statsRow->addWidget(m_timePrecisionLabel);
    statsRow->addWidget(m_timePrecisionComboBox);
    statsRow->addStretch();

    // ═══════════════════════════════════════════════
    //  操作按钮区（按流程分组）
    // ═══════════════════════════════════════════════

    // ──── ① 文件准备 ────
    QGroupBox* fileGroup = new QGroupBox("① 文件准备");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);
    fileLayout->setSpacing(10);

    m_openExcelButton = new ElaPushButton("打开 Excel 并读取");
    m_openExcelButton->setFixedSize(180, 38);
    m_openExcelButton->setEnabled(false);

    fileLayout->addWidget(m_openExcelButton);
    fileLayout->addStretch();

    // ──── ② 网络发送 ────
    QGroupBox* netGroup = new QGroupBox("② 网络发送");
    QHBoxLayout* netLayout = new QHBoxLayout(netGroup);
    netLayout->setSpacing(10);

    m_sendExcelButton = new ElaPushButton("开始发送");
    m_sendExcelButton->setFixedSize(140, 38);
    m_sendExcelButton->setEnabled(false);

    m_stopSendExcelButton = new ElaPushButton("停止发送");
    m_stopSendExcelButton->setFixedSize(140, 38);
    m_stopSendExcelButton->setEnabled(false);

    m_sendNetWorkAndReadRecordButton = new ElaPushButton("发送并保存回读参数");
    m_sendNetWorkAndReadRecordButton->setFixedSize(180, 38);
    m_sendNetWorkAndReadRecordButton->setEnabled(false);

    netLayout->addWidget(m_sendExcelButton);
    netLayout->addWidget(m_stopSendExcelButton);
    netLayout->addWidget(m_sendNetWorkAndReadRecordButton);
    netLayout->addStretch();

    // ──── ③ 串口发送 ────
    QGroupBox* serialSendGroup = new QGroupBox("③ 串口发送");
    QHBoxLayout* serialSendLayout = new QHBoxLayout(serialSendGroup);
    serialSendLayout->setSpacing(10);

    m_sendSerialButton = new ElaPushButton("开始发送");
    m_sendSerialButton->setFixedSize(140, 38);
    m_sendSerialButton->setEnabled(false);

    m_stopSendSerialButton = new ElaPushButton("停止发送");
    m_stopSendSerialButton->setFixedSize(140, 38);
    m_stopSendSerialButton->setEnabled(false);

    m_sendSerialAndReadRecordButton = new ElaPushButton("发送并保存回读参数");
    m_sendSerialAndReadRecordButton->setFixedSize(180, 38);
    m_sendSerialAndReadRecordButton->setEnabled(false);

    serialSendLayout->addWidget(m_sendSerialButton);
    serialSendLayout->addWidget(m_stopSendSerialButton);
    serialSendLayout->addWidget(m_sendSerialAndReadRecordButton);
    serialSendLayout->addStretch();

    // ──── ④ 工具 ────
    QGroupBox* toolGroup = new QGroupBox("④ 工具");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    toolLayout->setSpacing(10);

    m_GUIClearButton = new ElaPushButton("清空发送与接收区");
    m_GUIClearButton->setFixedSize(180, 38);

    toolLayout->addWidget(m_GUIClearButton);
    toolLayout->addStretch();

    // ════════════ 组装 ════════════
    layout->addLayout(topRow, 1);          // 表格+日志（可伸缩）
    layout->addLayout(statsRow);           // 统计栏
    layout->addWidget(fileGroup);          // ①
    layout->addWidget(netGroup);           // ②
    layout->addWidget(serialSendGroup);    // ③
    layout->addWidget(toolGroup);          // ④

    // ════════════ 信号槽 ════════════
    connect(m_openExcelButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onOpenExcelClicked);
    connect(m_sendExcelButton, &ElaPushButton::clicked,
            this, [this]() { onStartSendExcel(false); });
    connect(m_stopSendExcelButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onStopSendExcel);
    connect(m_sendSerialButton, &ElaPushButton::clicked,
            this, [this]() { onStartSendSerial(false); });
    connect(m_stopSendSerialButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onStopSendSerial);
    connect(m_GUIClearButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::clearGUI);
    connect(m_sendNetWorkAndReadRecordButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::sendNetWorkAndReadRecordSlot);
    connect(m_sendSerialAndReadRecordButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::sendSerialAndReadRecordSlot);
}

// ═══════════════════════════════════════════════════════════════
//  通讯设置页面
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::createSettingsPage()
{
    _settingsPage = new QWidget();

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(scrollContent);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 12, 16, 12);

    // ──── 网络设置 ────
    QGroupBox* networkGroup = new QGroupBox("网络设置");
    QGridLayout* networkGrid = new QGridLayout(networkGroup);
    networkGrid->setSpacing(8);

    m_serverIpLabel = new ElaText("电源网络地址:");
    m_serverIpLabel->setTextPixelSize(13);
    m_serverIpLineEdit = new ElaLineEdit();
    m_serverIpLineEdit->setText("127.0.0.1");

    m_portLabel = new ElaText("端口号:");
    m_portLabel->setTextPixelSize(13);
    m_portLineEdit = new ElaLineEdit();
    m_portLineEdit->setText("20108");

    m_NetWorkLEDLabel = new ElaText("网络状态:");
    m_NetWorkLEDLabel->setTextPixelSize(13);
    m_NetWorkLED = new QLabel();
    LED::setLED(m_NetWorkLED, 0, 16);

    m_connectButton = new ElaPushButton("连接到电源");
    m_connectButton->setFixedHeight(35);
    m_disconnectButton = new ElaPushButton("断开链接");
    m_disconnectButton->setFixedHeight(35);
    m_disconnectButton->setEnabled(false);

    m_tcpNoDelayCheckBox = new ElaCheckBox("TCP 禁用 Nagle 算法");

    networkGrid->addWidget(m_serverIpLabel, 0, 0);
    networkGrid->addWidget(m_serverIpLineEdit, 0, 1);
    networkGrid->addWidget(m_portLabel, 1, 0);
    networkGrid->addWidget(m_portLineEdit, 1, 1);
    networkGrid->addWidget(m_NetWorkLEDLabel, 2, 0);
    networkGrid->addWidget(m_NetWorkLED, 2, 1);
    networkGrid->addWidget(m_connectButton, 3, 0);
    networkGrid->addWidget(m_disconnectButton, 3, 1);
    networkGrid->addWidget(m_tcpNoDelayCheckBox, 4, 0, 1, 2);

    layout->addWidget(networkGroup);

    // ──── 串口设置 ────
    QGroupBox* serialGroup = new QGroupBox("串口设置");
    QGridLayout* serialGrid = new QGridLayout(serialGroup);
    serialGrid->setSpacing(8);

    m_serialPortLabel = new ElaText("串口端口号:");
    m_serialPortLabel->setTextPixelSize(13);
    m_serialPortComboBox = new ElaComboBox();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        m_serialPortComboBox->addItem("无可用串口");
    } else {
        for (const QSerialPortInfo &info : portList)
            m_serialPortComboBox->addItem(info.portName());
    }

    m_baudRateLabel = new ElaText("波特率:");
    m_baudRateLabel->setTextPixelSize(13);
    m_baudRateComboBox = new ElaComboBox();
    m_baudRateComboBox->addItems({"1200","2400","4800","9600","19200","38400","57600","115200"});
    m_baudRateComboBox->setCurrentText("115200");

    m_dataBitsLabel = new ElaText("数据位:");
    m_dataBitsLabel->setTextPixelSize(13);
    m_dataBitsComboBox = new ElaComboBox();
    m_dataBitsComboBox->addItems({"5","6","7","8"});
    m_dataBitsComboBox->setCurrentText("8");

    m_stopBitsLabel = new ElaText("停止位:");
    m_stopBitsLabel->setTextPixelSize(13);
    m_stopBitsComboBox = new ElaComboBox();
    m_stopBitsComboBox->addItems({"1","1.5","2"});
    m_stopBitsComboBox->setCurrentText("1");

    m_parityLabel = new ElaText("校验位:");
    m_parityLabel->setTextPixelSize(13);
    m_parityComboBox = new ElaComboBox();
    m_parityComboBox->addItems({"None","Even","Odd","Space","Mark"});
    m_parityComboBox->setCurrentText("None");

    m_SerialLEDLabel = new ElaText("串口状态:");
    m_SerialLEDLabel->setTextPixelSize(13);
    m_SerialLED = new QLabel();
    LED::setLED(m_SerialLED, 0, 16);

    m_openSerialButton = new ElaPushButton("打开串口");
    m_openSerialButton->setFixedHeight(35);
    m_closeSerialButton = new ElaPushButton("关闭串口");
    m_closeSerialButton->setFixedHeight(35);
    m_closeSerialButton->setEnabled(false);

    m_serialBufferCheckBox = new ElaCheckBox("合并串口接收数据（20ms超时）");

    serialGrid->addWidget(m_serialPortLabel, 0, 0);
    serialGrid->addWidget(m_serialPortComboBox, 0, 1);
    serialGrid->addWidget(m_baudRateLabel, 0, 2);
    serialGrid->addWidget(m_baudRateComboBox, 0, 3);
    serialGrid->addWidget(m_dataBitsLabel, 1, 0);
    serialGrid->addWidget(m_dataBitsComboBox, 1, 1);
    serialGrid->addWidget(m_stopBitsLabel, 1, 2);
    serialGrid->addWidget(m_stopBitsComboBox, 1, 3);
    serialGrid->addWidget(m_parityLabel, 2, 0);
    serialGrid->addWidget(m_parityComboBox, 2, 1);
    serialGrid->addWidget(m_SerialLEDLabel, 2, 2);
    serialGrid->addWidget(m_SerialLED, 2, 3);
    serialGrid->addWidget(m_openSerialButton, 3, 0);
    serialGrid->addWidget(m_closeSerialButton, 3, 1);
    serialGrid->addWidget(m_serialBufferCheckBox, 3, 2, 1, 2);

    layout->addWidget(serialGroup);

    // ──── 发送参数 ────
    QGroupBox* paramGroup = new QGroupBox("发送参数设置");
    QGridLayout* paramGrid = new QGridLayout(paramGroup);
    paramGrid->setSpacing(8);

    m_delayLabel = new ElaText("命令发送延时(ms):");
    m_delayLabel->setTextPixelSize(13);
    m_delayLineEdit = new ElaLineEdit();
    m_delayLineEdit->setText("50");

    m_timeoutLabel = new ElaText("命令超时时间(ms):");
    m_timeoutLabel->setTextPixelSize(13);
    m_timeoutLineEdit = new ElaLineEdit();
    m_timeoutLineEdit->setText("50");

    m_sendLimitLabel = new ElaText("发送条数(0=无限):");
    m_sendLimitLabel->setTextPixelSize(13);
    m_sendLimitLineEdit = new ElaLineEdit();
    m_sendLimitLineEdit->setText("0");

    paramGrid->addWidget(m_delayLabel, 0, 0);
    paramGrid->addWidget(m_delayLineEdit, 0, 1);
    paramGrid->addWidget(m_timeoutLabel, 1, 0);
    paramGrid->addWidget(m_timeoutLineEdit, 1, 1);
    paramGrid->addWidget(m_sendLimitLabel, 2, 0);
    paramGrid->addWidget(m_sendLimitLineEdit, 2, 1);

    layout->addWidget(paramGroup);

    // ──── 功能选项 ────
    QGroupBox* optionGroup = new QGroupBox("功能选项");
    QVBoxLayout* optionLayout = new QVBoxLayout(optionGroup);
    optionLayout->setSpacing(6);

    m_sendWithAN3CheckBox = new ElaCheckBox("使用 HEX 发送 (AN3.0)");
    m_testPacketLossCheckBox = new ElaCheckBox("测试接收丢包情况");
    m_onlySendDataModeCheckBox = new ElaCheckBox("只发送，不统计错误率");

    optionLayout->addWidget(m_sendWithAN3CheckBox);
    optionLayout->addWidget(m_testPacketLossCheckBox);
    optionLayout->addWidget(m_onlySendDataModeCheckBox);

    layout->addWidget(optionGroup);
    layout->addStretch();

    scrollArea->setWidget(scrollContent);

    QVBoxLayout* outerLayout = new QVBoxLayout(_settingsPage);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // ──── 设置页面信号槽 ────
    connect(m_connectButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onConnectServer);
    connect(m_disconnectButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onDisconnectServer);
    connect(m_openSerialButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onOpenSerial);
    connect(m_closeSerialButton, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onCloseSerial);

    connect(m_testPacketLossCheckBox, &ElaCheckBox::toggled,
            this, &ElaWidgetToolsDemo::updateValidatorMode);
    connect(m_onlySendDataModeCheckBox, &ElaCheckBox::toggled,
            this, &ElaWidgetToolsDemo::updateValidatorMode);
}
// ═══════════════════════════════════════════════════════════════
//  单条发送页面
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::createSingleSendPage()
{
    _singleSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_singleSendPage);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    // ──── 标题 ────
    ElaText* title = new ElaText("单条命令发送");
    title->setTextPixelSize(20);
    title->setTextStyle(ElaTextType::Title);

    ElaText* desc = new ElaText(
        "在此输入单条命令，通过网络或串口发送到设备。\n"
        "发送和接收的结果将显示在下方日志区域。");
    desc->setTextPixelSize(13);

    // ──── 输入区域 ────
    QGroupBox* inputGroup = new QGroupBox("命令输入");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->setSpacing(8);

    m_singleSendInput = new ElaLineEdit();
    m_singleSendInput->setPlaceholderText("在此输入要发送的命令...");
    m_singleSendInput->setFixedHeight(38);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_singleSendNetBtn = new ElaPushButton("通过网络发送");
    m_singleSendNetBtn->setFixedSize(160, 38);
    m_singleSendNetBtn->setEnabled(false);

    m_singleSendSerialBtn = new ElaPushButton("通过串口发送");
    m_singleSendSerialBtn->setFixedSize(160, 38);
    m_singleSendSerialBtn->setEnabled(false);

    btnRow->addWidget(m_singleSendNetBtn);
    btnRow->addWidget(m_singleSendSerialBtn);
    btnRow->addStretch();

    inputLayout->addWidget(m_singleSendInput);
    inputLayout->addLayout(btnRow);

    // ──── 日志区域 ────
    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    QVBoxLayout* sendArea = new QVBoxLayout();
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(13);
    m_singleSendLog = new QListWidget();
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_singleSendLog);

    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(13);
    m_singleRecvLog = new QListWidget();
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_singleRecvLog);

    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);

    // ──── 清空按钮 ────
    QHBoxLayout* clearRow = new QHBoxLayout();
    m_singleSendClearBtn = new ElaPushButton("清空日志");
    m_singleSendClearBtn->setFixedSize(120, 36);
    clearRow->addWidget(m_singleSendClearBtn);
    clearRow->addStretch();

    // ──── 组装 ────
    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(inputGroup);
    layout->addLayout(logRow, 1);
    layout->addLayout(clearRow);

    // ════════════ 信号槽 ════════════
    connect(m_singleSendNetBtn, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onSingleSendNetwork);
    connect(m_singleSendSerialBtn, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onSingleSendSerial);
    connect(m_singleSendClearBtn, &ElaPushButton::clicked, this, [=]() {
        m_singleSendLog->clear();
        m_singleRecvLog->clear();
    });
}
// ═══════════════════════════════════════════════════════════════
//  错误日志页面
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::createErrorLogPage()
{
    _errorLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_errorLogPage);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    ElaText* title = new ElaText("错误日志");
    title->setTextPixelSize(13);
    title->setTextStyle(ElaTextType::Title);

    ElaText* desc = new ElaText(
        "记录所有发送过程中出现的返回值错误和超时。\n"
        "每条日志包含时间、行号、期望值和实际值。");
    desc->setTextPixelSize(16);

    // ========== 三张统计卡片 ==========
    QHBoxLayout* cardRow = new QHBoxLayout();
    cardRow->setSpacing(16);

    m_errorCard     = new StatCard("返回值错误", "0", _errorLogPage);
    m_timeoutCard   = new StatCard("超时错误",   "0", _errorLogPage);
    m_totalSendCard = new StatCard("总计发送",   "0", _errorLogPage);

    cardRow->addWidget(m_errorCard);
    cardRow->addWidget(m_timeoutCard);
    cardRow->addWidget(m_totalSendCard);
    cardRow->addStretch();

    // 日志列表
    ElaText* logLabel = new ElaText("错误详情");
    logLabel->setTextPixelSize(13);
    m_errorLogList = new QListWidget();
    m_errorLogList->setAlternatingRowColors(true);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_exportErrorBtn = new ElaPushButton("导出到文件");
    m_exportErrorBtn->setFixedSize(140, 36);
    m_clearErrorBtn = new ElaPushButton("清除日志");
    m_clearErrorBtn->setFixedSize(120, 36);

    btnRow->addWidget(m_exportErrorBtn);
    btnRow->addWidget(m_clearErrorBtn);
    btnRow->addStretch();

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addLayout(cardRow);
    layout->addWidget(logLabel);
    layout->addWidget(m_errorLogList, 1);
    layout->addLayout(btnRow);

    connect(m_exportErrorBtn, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onExportErrorLog);
    connect(m_clearErrorBtn, &ElaPushButton::clicked,
            this, &ElaWidgetToolsDemo::onClearErrorLog);
}

// ═══════════════════════════════════════════════════════════════
//  导出错误日志
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::onExportErrorLog()
{
    if (!m_errorLogList || m_errorLogList->count() == 0) {
        QMessageBox::information(this, "提示", "没有错误日志可导出。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出错误日志",
        QString("error_log_%1.txt")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "文本文件 (*.txt)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件:\n" + filePath);
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);

    stream << "===== 错误日志导出 ["
           << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
           << "] =====\n\n";

    for (int i = 0; i < m_errorLogList->count(); ++i) {
        stream << m_errorLogList->item(i)->text() << "\n";
    }

    stream << "\n===== 共 " << m_errorLogList->count() << " 条错误记录 =====\n";
    file.close();

    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条错误记录到:\n%2")
            .arg(m_errorLogList->count()).arg(filePath));
}

// ═══════════════════════════════════════════════════════════════
//  清除错误日志
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::onClearErrorLog()
{
    if (m_errorLogList) {
        m_errorLogList->clear();
    }
    m_errorCount = 0;
    m_errorTimeOut = 0;
    updateAllErrorDisplayLabels();
}

// ═══════════════════════════════════════════════════════════════
//  软件调试页面
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::createDebugPage()
{
    _debugPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_debugPage);
    layout->setContentsMargins(30, 30, 30, 30);
    ElaText* title = new ElaText("软件调试模式");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    ElaText* desc = new ElaText(
        "开启后，数据收发界面的所有按钮将解除限制，\n"
        "无需连接网络或串口即可点击操作。\n"
        "此模式仅用于软件调试，正常使用时请关闭。");
    desc->setTextPixelSize(14);
    // 开关行
    QHBoxLayout* switchRow = new QHBoxLayout();
    ElaText* switchLabel = new ElaText("调试模式");
    switchLabel->setTextPixelSize(16);
    ElaToggleSwitch* debugSwitch = new ElaToggleSwitch();
    debugSwitch->setIsToggled(false);
    switchRow->addWidget(switchLabel);
    switchRow->addSpacing(12);
    switchRow->addWidget(debugSwitch);
    switchRow->addStretch();
    // 状态提示
    ElaText* statusLabel = new ElaText("当前状态：正常模式");
    statusLabel->setTextPixelSize(14);
    statusLabel->setObjectName("DebugStatusLabel");
    layout->addWidget(title);
    layout->addSpacing(8);
    layout->addWidget(desc);
    layout->addSpacing(20);
    layout->addLayout(switchRow);
    layout->addSpacing(8);
    layout->addWidget(statusLabel);
    layout->addStretch();
    // 信号连接
    connect(debugSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked) {
        m_debugMode = checked;
        applyDebugMode(checked);
        ElaText* label = _debugPage->findChild<ElaText*>("DebugStatusLabel");
        if (label) {
            label->setText(checked
                ? "当前状态：调试模式（所有按钮已解锁）"
                : "当前状态：正常模式");
        }
    });
}
// ═══════════════════════════════════════════════════════════════
//  应用调试模式
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::applyDebugMode(bool enabled)
{
    if (enabled) {
        m_openExcelButton->setEnabled(true);
        m_sendExcelButton->setEnabled(true);
        m_stopSendExcelButton->setEnabled(true);
        m_sendNetWorkAndReadRecordButton->setEnabled(true);
        m_sendSerialButton->setEnabled(true);
        m_stopSendSerialButton->setEnabled(true);
        m_sendSerialAndReadRecordButton->setEnabled(true);
        m_GUIClearButton->setEnabled(true);
        m_singleSendNetBtn->setEnabled(true);       // ← 新增
        m_singleSendSerialBtn->setEnabled(true);    // ← 新增
    } else {
        m_openExcelButton->setEnabled(false);
        m_sendExcelButton->setEnabled(false);
        m_stopSendExcelButton->setEnabled(false);
        m_sendNetWorkAndReadRecordButton->setEnabled(false);
        m_sendSerialButton->setEnabled(false);
        m_stopSendSerialButton->setEnabled(false);
        m_sendSerialAndReadRecordButton->setEnabled(false);
        m_GUIClearButton->setEnabled(true);
        m_singleSendNetBtn->setEnabled(false);      // ← 新增
        m_singleSendSerialBtn->setEnabled(false);   // ← 新增

        if (m_excelReader && m_excelReader->totalRows > 0) {
            if (m_currentConnectionType == ConnectionType::Network) {
                m_sendExcelButton->setEnabled(true);
                m_sendNetWorkAndReadRecordButton->setEnabled(true);
            } else if (m_currentConnectionType == ConnectionType::Serial) {
                m_sendSerialButton->setEnabled(true);
                m_sendSerialAndReadRecordButton->setEnabled(true);
            }
        }
        if (m_currentConnectionType != ConnectionType::None) {
            m_openExcelButton->setEnabled(true);
        }
    }
}
// ═══════════════════════════════════════════════════════════════
//  ============  以下所有业务逻辑方法从原 GUI.cpp 搬运  ==========
//  ============  只改类名 GUI:: → ElaWidgetToolsDemo::  ==========
// ═══════════════════════════════════════════════════════════════

// 读取 Excel 表格
void ElaWidgetToolsDemo::onOpenExcelClicked()
{
    qDebug()<<"打开excel按钮被触发";
    m_fileDialog.setFileMode(QFileDialog::ExistingFile);
    m_fileDialog.setViewMode(QFileDialog::Detail);
    m_fileDialog.setOption(QFileDialog::ReadOnly, true);
    m_fileDialog.setDirectory("C:/");
    m_fileDialog.setNameFilter("所有文件(*.*);;Microsoft Excel工作表(*.xlsx);;Microsoft Excel 97-2003工作表(*.xls)");

    /*先确保在这里不会崩溃*/
    if (m_fileDialog.exec()) {
        QStringList files = m_fileDialog.selectedFiles();
        for (auto fname : files) {
            if (m_excelReader->loadExcelToTable(fname, m_excelTableWidget, this)) {
                QMessageBox::information(this, "提示",
                    QStringLiteral("Excel 文件读取完成！\n总行数：")
                    + QString::number(m_excelReader->totalRows));
                if (m_debugMode) return;   // ← debug 模式不做任何限制
                if (m_currentConnectionType == ConnectionType::Network) {
                    m_sendExcelButton->setEnabled(true);
                    m_sendNetWorkAndReadRecordButton->setEnabled(true);
                } else if (m_currentConnectionType == ConnectionType::Serial) {
                    m_sendSerialButton->setEnabled(true);
                    m_sendSerialAndReadRecordButton->setEnabled(true);
                }
            }
        }
    }
}

// 发送命令并读取返回值作为比较值 Network
void ElaWidgetToolsDemo::sendNetWorkAndReadRecordSlot()
{
    qDebug() << "发送并读取结果按钮已经点击！";
    m_sendNetWorkAndReadRecordButton->setEnabled(false);
    onStartSendExcel(true);
}

// 发送命令并读取返回值作为比较值 Serial
void ElaWidgetToolsDemo::sendSerialAndReadRecordSlot()
{
    qDebug() << "发送并读取结果按钮已经点击！";
    m_sendSerialAndReadRecordButton->setEnabled(false);
    onStartSendSerial(true);
}

// ═══════════════════════════════════════════════════════════════
//  单条发送 — 网络
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::onSingleSendNetwork()
{
    QString text = m_singleSendInput->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (m_currentConnectionType != ConnectionType::Network || !m_networkClient) {
        QMessageBox::warning(this, "提示", "请先在\"通讯设置\"中连接到网络！");
        return;
    }

    QByteArray cmdToSend = m_sendWithAN3CheckBox->isChecked()
        ? hexStringToBytes(text)   // ← 复用公共方法
        : text.toUtf8();

    emit requestSendNetworkData(cmdToSend);

    QString displayText = m_sendWithAN3CheckBox->isChecked()
        ? toHexDisplay(cmdToSend) : QString::fromUtf8(cmdToSend);
    m_singleSendLog->addItem("[" + currentTimeString() + "] " + displayText);

    while (m_singleSendLog->count() > m_maxDisplayItems) {
        delete m_singleSendLog->takeItem(0);
    }
}
// ═══════════════════════════════════════════════════════════════
//  单条发送 — 串口
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::onSingleSendSerial()
{
    QString text = m_singleSendInput->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (m_currentConnectionType != ConnectionType::Serial || !m_serialWorker) {
        QMessageBox::warning(this, "提示", "请先在\"通讯设置\"中打开串口！");
        return;
    }

    QByteArray cmdToSend = m_sendWithAN3CheckBox->isChecked()
        ? hexStringToBytes(text)   // ← 复用公共方法
        : text.toUtf8();

    QMetaObject::invokeMethod(m_serialWorker, "writeData",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, cmdToSend));

    QString displayText = m_sendWithAN3CheckBox->isChecked()
        ? toHexDisplay(cmdToSend) : QString::fromUtf8(cmdToSend);
    m_singleSendLog->addItem("[" + currentTimeString() + "] " + displayText);

    while (m_singleSendLog->count() > m_maxDisplayItems) {
        delete m_singleSendLog->takeItem(0);
    }
}
// 连接服务器
void ElaWidgetToolsDemo::onConnectServer()
{
    qDebug() << "debug:onConnectServer已经触发";

    m_networkClient = new NetworkClient();
    m_savedataWorker = new saveworker();
    m_responseValidator = new ResponseValidator();

    m_networkClient->contentListWidget = m_receiveListWidget;
    m_networkClient->sendListWidget = m_sendListWidget;
    m_networkClient->m_disableNagle = m_tcpNoDelayCheckBox->isChecked();

    m_networkClient->moveToThread(m_networkThread);
    m_savedataWorker->moveToThread(m_savedataThread);
    m_responseValidator->moveToThread(m_validatorThread);

    // GUI → NetworkClient
    connect(this, &ElaWidgetToolsDemo::requestNetworkConnect,
            m_networkClient, &NetworkClient::onNetworkConnected);
    connect(this, &ElaWidgetToolsDemo::requestSendNetworkData,
            m_networkClient, &NetworkClient::sendNetworkData);

    // GUI → Validator
    connect(this, &ElaWidgetToolsDemo::requestEnqueueExpected,
            m_responseValidator, &ResponseValidator::onCommandSent);
    connect(this, &ElaWidgetToolsDemo::requestValidateData,
            m_responseValidator, &ResponseValidator::onDataReceived);
    connect(this, &ElaWidgetToolsDemo::requestResetValidator,
            m_responseValidator, &ResponseValidator::onReset);

    // NetworkClient → GUI
    connect(m_networkClient, &NetworkClient::displaySentData,
            this, &ElaWidgetToolsDemo::onDisplaySentData);
    connect(m_networkClient, &NetworkClient::displayReceivedData,
            this, &ElaWidgetToolsDemo::onDisplayReceivedData);
    connect(m_networkClient, &NetworkClient::disConnectServer,
            this, &ElaWidgetToolsDemo::onDisconnectServer);
    connect(m_networkClient, &NetworkClient::successConnectServer,
            this, &ElaWidgetToolsDemo::successConnectServer);

    connect(m_networkClient, &NetworkClient::connectionError,
            this, [this](const QString &errorMsg) {
        QMessageBox::critical(this, "连接错误",
            QString("网络连接失败：%1").arg(errorMsg));
    });
    connect(m_networkClient, &NetworkClient::disConnectWithServer,
            this, [this] {
        QMessageBox::warning(this, "连接错误", "电源主动断开网络连接！");
    });

    // Validator
    connect(this, &ElaWidgetToolsDemo::requestSetValidatorMode,
            m_responseValidator, &ResponseValidator::onSetTestPacketLossMode);
    connect(m_responseValidator, &ResponseValidator::errorDetected,
            this, &ElaWidgetToolsDemo::onErrorDetected);

    // SaveWorker
    connect(this, &ElaWidgetToolsDemo::requestInitSaveFile,
            m_savedataWorker, &saveworker::initWriteFile);
    connect(this, &ElaWidgetToolsDemo::requestWriteSaveFile,
            m_savedataWorker, &saveworker::writeFile);
    connect(this, &ElaWidgetToolsDemo::requestCloseSaveFile,
            m_savedataWorker, &saveworker::closeWriteFile);
    connect(this, &ElaWidgetToolsDemo::requestResetTableWrite,
            m_savedataWorker, &saveworker::resetTableWritePosition);

    // 子线程启动
    m_networkThread->start();
    m_savedataThread->start();
    m_validatorThread->start();

    emit requestNetworkConnect(m_portLineEdit->text().toInt(),
                               QHostAddress(m_serverIpLineEdit->text()));
    emit requestInitSaveFile(QString());

    // 状态切换
    m_currentConnectionType = ConnectionType::Network;
    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_portLineEdit->setEnabled(false);
    m_serverIpLineEdit->setEnabled(false);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_tcpNoDelayCheckBox->setEnabled(false);
    m_sendWithAN3CheckBox->setEnabled(false);
}

// 连接成功
void ElaWidgetToolsDemo::successConnectServer()
{
    m_currentConnectionType = ConnectionType::Network;
    LED::setLED(m_NetWorkLED, 2, 16);
    if (!m_debugMode) {
        m_disconnectButton->setEnabled(true);
        m_connectButton->setEnabled(false);
        m_portLineEdit->setEnabled(false);
        m_serverIpLineEdit->setEnabled(false);
        m_openExcelButton->setEnabled(true);
        m_openSerialButton->setEnabled(false);
        m_tcpNoDelayCheckBox->setEnabled(false);
        m_sendWithAN3CheckBox->setEnabled(false);
        m_singleSendNetBtn->setEnabled(true);       // ← 新增
        m_singleSendSerialBtn->setEnabled(false);   // ← 新增
    }
}

// 断开连接
void ElaWidgetToolsDemo::onDisconnectServer()
{
    qDebug() << "debug:onDisconnectServer已经触发";

    if (m_networkClient) {
        m_networkClient->deleteLater();
        m_networkClient = nullptr;
    }
    if (m_networkThread && m_networkThread->isRunning()) {
        m_networkThread->quit();
        m_networkThread->wait();
    }

    if (m_savedataWorker) {
        emit requestCloseSaveFile();
    }
    if (m_savedataThread && m_savedataThread->isRunning()) {
        m_savedataThread->quit();
        m_savedataThread->wait();
    }
    if (m_savedataWorker) {
        m_savedataWorker->deleteLater();
        m_savedataWorker = nullptr;
    }

    if (m_responseValidator) {
        m_responseValidator->deleteLater();
        m_responseValidator = nullptr;
    }
    if (m_validatorThread && m_validatorThread->isRunning()) {
        m_validatorThread->quit();
        m_validatorThread->wait();
    }

    m_currentConnectionType = ConnectionType::None;
    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(true);
    m_portLineEdit->setEnabled(true);
    m_serverIpLineEdit->setEnabled(true);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(true);
    m_sendSerialButton->setEnabled(false);
    m_tcpNoDelayCheckBox->setEnabled(true);
    m_sendWithAN3CheckBox->setEnabled(true);
    m_sendNetWorkAndReadRecordButton->setEnabled(false);
    m_singleSendNetBtn->setEnabled(false);
    LED::setLED(m_NetWorkLED, 0, 16);
}

// 启动网络发送
void ElaWidgetToolsDemo::onStartSendExcel(bool data)
{
    qDebug() << "debug:onStartSendExcel 非阻塞线程启动";
    m_sendAndReadRecordBool = data;

    if (data) {
        resetTableForReadRecord();
    }

    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(true);
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_stopSendSerialButton->setEnabled(false);
    m_sendLimitLineEdit->setEnabled(false);
    m_delayLineEdit->setEnabled(false);

    m_expectedResponseQueue.clear();
    m_errorCount = 0;

    m_excelSendWorker = new ExcelSendWorker();
    m_excelSendWorker->m_table = m_excelTableWidget;
    m_excelSendWorker->m_totalRows = m_excelReader->totalRows;
    m_excelSendWorker->m_delayMs = m_delayLineEdit->text().toInt();
    m_excelSendWorker->m_timeoutMs = m_timeoutLineEdit->text().toInt();
    m_excelSendWorker->m_useHexSend = m_sendWithAN3CheckBox->isChecked();

    if (data) {
        m_excelSendWorker->m_repeatLimit = m_excelReader->totalRows;
    } else {
        m_excelSendWorker->m_repeatLimit = m_sendLimitLineEdit->text().toInt();
    }

    m_excelSendWorker->moveToThread(m_excelSendThread);

    connect(this, &ElaWidgetToolsDemo::requestStartSend,
            m_excelSendWorker, &ExcelSendWorker::startNetworkWork);
    connect(this, &ElaWidgetToolsDemo::requestStopSend,
            m_excelSendWorker, &ExcelSendWorker::stopNetworkWork, Qt::DirectConnection);
    connect(m_excelSendWorker, &ExcelSendWorker::sendCommand,
            m_networkClient, &NetworkClient::sendNetworkData);
    connect(m_excelSendWorker, &ExcelSendWorker::finished,
            this, &ElaWidgetToolsDemo::onSendFinished);
    connect(m_excelSendWorker, &ExcelSendWorker::sentCountChanged,
            this, &ElaWidgetToolsDemo::onUpdateSentCount);
    connect(m_excelSendWorker, &ExcelSendWorker::commandSent,
            this, &ElaWidgetToolsDemo::onCommandSent);
    connect(m_excelSendWorker, &ExcelSendWorker::sendTimeOut,
            this, &ElaWidgetToolsDemo::onErrorTimeOut);
    connect(m_networkClient, &NetworkClient::displayReceivedData,
            m_excelSendWorker, &ExcelSendWorker::onExternalResponseReceived,
            Qt::DirectConnection);

    m_excelSendThread->start();
    emit requestStartSend();
}

// 停止网络发送
void ElaWidgetToolsDemo::onStopSendExcel()
{
    qDebug() << "手动停止发送";
    if (m_excelSendWorker) {
        emit requestStopSend();
    }
    m_stopSendExcelButton->setEnabled(false);
}

// 网络发送完成清理
void ElaWidgetToolsDemo::onSendFinished()
{
    qDebug() << "发送线程结束，开始清理资源";

    if (m_networkClient && m_excelSendWorker) {
        disconnect(m_networkClient, &NetworkClient::displayReceivedData,
                   m_excelSendWorker, &ExcelSendWorker::onExternalResponseReceived);
    }
    if (m_excelSendWorker) {
        m_excelSendWorker->deleteLater();
        m_excelSendWorker = nullptr;
    }
    if (m_excelSendThread && m_excelSendThread->isRunning()) {
        m_excelSendThread->quit();
        m_excelSendThread->wait();
    }

    m_sendLimitLineEdit->setEnabled(true);
    m_delayLineEdit->setEnabled(true);
    m_stopSendExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(true);
    m_openExcelButton->setEnabled(true);
    m_disconnectButton->setEnabled(true);
}

// 打开串口
void ElaWidgetToolsDemo::onOpenSerial()
{
    qDebug() << "debug:onOpenSerial已经触发";

    QString portName = m_serialPortComboBox->currentText();
    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(this, "警告", "请选择有效的串口端口！");
        return;
    }

    qint32 baudRate = m_baudRateComboBox->currentText().toInt();

    QSerialPort::DataBits dataBits;
    QString dataBitsStr = m_dataBitsComboBox->currentText();
    if (dataBitsStr == "5")      dataBits = QSerialPort::Data5;
    else if (dataBitsStr == "6") dataBits = QSerialPort::Data6;
    else if (dataBitsStr == "7") dataBits = QSerialPort::Data7;
    else                         dataBits = QSerialPort::Data8;

    QSerialPort::StopBits stopBits;
    QString stopBitsStr = m_stopBitsComboBox->currentText();
    if (stopBitsStr == "1.5")         stopBits = QSerialPort::OneAndHalfStop;
    else if (stopBitsStr == "2")      stopBits = QSerialPort::TwoStop;
    else                              stopBits = QSerialPort::OneStop;

    QSerialPort::Parity parity;
    QString parityStr = m_parityComboBox->currentText();
    if (parityStr == "Even")          parity = QSerialPort::EvenParity;
    else if (parityStr == "Odd")      parity = QSerialPort::OddParity;
    else if (parityStr == "Space")    parity = QSerialPort::SpaceParity;
    else if (parityStr == "Mark")     parity = QSerialPort::MarkParity;
    else                              parity = QSerialPort::NoParity;

    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;

    m_serialWorker = new SerialWorker();
    m_serialWorker->moveToThread(m_serialThread);

    m_savedataWorker = new saveworker();
    m_savedataWorker->moveToThread(m_savedataThread);

    m_responseValidator = new ResponseValidator();
    m_responseValidator->moveToThread(m_validatorThread);

    connect(this, &ElaWidgetToolsDemo::requestSerialOpen,
            m_serialWorker, &SerialWorker::onSerialStart);
    connect(this, &ElaWidgetToolsDemo::requestSerialClose,
            m_serialWorker, &SerialWorker::onSerialStop);
    connect(m_serialWorker, &SerialWorker::serialClosed,
            this, &ElaWidgetToolsDemo::onSerialClosed);
    connect(m_serialWorker, &SerialWorker::displaySentData,
            this, &ElaWidgetToolsDemo::onDisplaySentData);
    connect(m_serialWorker, &SerialWorker::displayReceivedData,
            this, &ElaWidgetToolsDemo::onDisplayReceivedData);

    connect(m_serialBufferCheckBox, &ElaCheckBox::toggled,
            m_serialWorker, &SerialWorker::setBufferMode);
    QMetaObject::invokeMethod(m_serialWorker, "setBufferMode",
                              Qt::QueuedConnection,
                              Q_ARG(bool, m_serialBufferCheckBox->isChecked()));

    connect(this, &ElaWidgetToolsDemo::requestEnqueueExpected,
            m_responseValidator, &ResponseValidator::onCommandSent);
    connect(this, &ElaWidgetToolsDemo::requestValidateData,
            m_responseValidator, &ResponseValidator::onDataReceived);
    connect(this, &ElaWidgetToolsDemo::requestResetValidator,
            m_responseValidator, &ResponseValidator::onReset);
    connect(this, &ElaWidgetToolsDemo::requestSetValidatorMode,
            m_responseValidator, &ResponseValidator::onSetTestPacketLossMode);
    connect(m_responseValidator, &ResponseValidator::errorDetected,
            this, &ElaWidgetToolsDemo::onErrorDetected);

    connect(this, &ElaWidgetToolsDemo::requestInitSaveFile,
            m_savedataWorker, &saveworker::initWriteFile);
    connect(this, &ElaWidgetToolsDemo::requestWriteSaveFile,
            m_savedataWorker, &saveworker::writeFile);
    connect(this, &ElaWidgetToolsDemo::requestCloseSaveFile,
            m_savedataWorker, &saveworker::closeWriteFile);
    connect(this, &ElaWidgetToolsDemo::requestResetTableWrite,
            m_savedataWorker, &saveworker::resetTableWritePosition);

    m_serialThread->start();
    m_savedataThread->start();
    m_validatorThread->start();

    emit requestSerialOpen(portName, baudRate, dataBits, parity, stopBits, flowControl);
    emit requestInitSaveFile(QString());

    m_currentConnectionType = ConnectionType::Serial;
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(true);
    m_connectButton->setEnabled(false);
    m_serialPortComboBox->setEnabled(false);
    m_baudRateComboBox->setEnabled(false);
    m_dataBitsComboBox->setEnabled(false);
    m_stopBitsComboBox->setEnabled(false);
    m_parityComboBox->setEnabled(false);
    m_openExcelButton->setEnabled(true);
    m_tcpNoDelayCheckBox->setEnabled(false);
    m_sendWithAN3CheckBox->setEnabled(false);
    m_singleSendNetBtn->setEnabled(false);
    m_singleSendSerialBtn->setEnabled(true);
    LED::setLED(m_SerialLED, 2, 16);
}

// 关闭串口
void ElaWidgetToolsDemo::onCloseSerial()
{
    qDebug() << "手动请求关闭串口";
    if (m_serialWorker) {
        emit requestSerialClose();
    }
}

// 串口关闭后清理
void ElaWidgetToolsDemo::onSerialClosed()
{
    qDebug() << "串口已关闭，清理线程资源";

    if (m_serialThread && m_serialThread->isRunning()) {
        m_serialThread->quit();
        m_serialThread->wait();
    }
    if (m_serialWorker) {
        m_serialWorker->deleteLater();
        m_serialWorker = nullptr;
    }

    if (m_savedataWorker) {
        emit requestCloseSaveFile();
    }
    if (m_savedataThread && m_savedataThread->isRunning()) {
        m_savedataThread->quit();
        m_savedataThread->wait();
    }
    if (m_savedataWorker) {
        m_savedataWorker->deleteLater();
        m_savedataWorker = nullptr;
    }

    if (m_responseValidator) {
        m_responseValidator->deleteLater();
        m_responseValidator = nullptr;
    }
    if (m_validatorThread && m_validatorThread->isRunning()) {
        m_validatorThread->quit();
        m_validatorThread->wait();
    }

    m_currentConnectionType = ConnectionType::None;
    m_openSerialButton->setEnabled(true);
    m_closeSerialButton->setEnabled(false);
    m_serialPortComboBox->setEnabled(true);
    m_baudRateComboBox->setEnabled(true);
    m_dataBitsComboBox->setEnabled(true);
    m_stopBitsComboBox->setEnabled(true);
    m_parityComboBox->setEnabled(true);
    m_openExcelButton->setEnabled(false);
    m_connectButton->setEnabled(true);
    m_sendSerialButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_tcpNoDelayCheckBox->setEnabled(true);
    m_sendWithAN3CheckBox->setEnabled(true);
    m_sendSerialAndReadRecordButton->setEnabled(false);
    m_singleSendSerialBtn->setEnabled(false);
    LED::setLED(m_SerialLED, 0, 16);
}

// 启动串口发送
void ElaWidgetToolsDemo::onStartSendSerial(bool data)
{
    qDebug() << "onStartSendSerial已触发!";
    m_sendAndReadRecordBool = data;

    if (data) {
        resetTableForReadRecord();
    }

    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_stopSendSerialButton->setEnabled(true);
    m_sendLimitLineEdit->setEnabled(false);
    m_delayLineEdit->setEnabled(false);

    m_expectedResponseQueue.clear();
    m_errorCount = 0;

    m_excelSendWorker = new ExcelSendWorker();
    m_excelSendWorker->m_table = m_excelTableWidget;
    m_excelSendWorker->m_totalRows = m_excelReader->totalRows;
    m_excelSendWorker->m_delayMs = m_delayLineEdit->text().toInt();
    m_excelSendWorker->m_timeoutMs = m_timeoutLineEdit->text().toInt();
    m_excelSendWorker->m_useHexSend = m_sendWithAN3CheckBox->isChecked();

    if (data) {
        m_excelSendWorker->m_repeatLimit = m_excelReader->totalRows;
    } else {
        m_excelSendWorker->m_repeatLimit = m_sendLimitLineEdit->text().toInt();
    }

    m_excelSendWorker->moveToThread(m_excelSendThread);

    connect(this, &ElaWidgetToolsDemo::requestStartSend,
            m_excelSendWorker, &ExcelSendWorker::serialStartWork);
    connect(this, &ElaWidgetToolsDemo::requestStopSend,
            m_excelSendWorker, &ExcelSendWorker::serialStopWork, Qt::DirectConnection);
    connect(m_excelSendWorker, &ExcelSendWorker::sendSerialCommand,
            m_serialWorker, &SerialWorker::writeData);
    connect(m_excelSendWorker, &ExcelSendWorker::serialFinished,
            this, &ElaWidgetToolsDemo::onSerialSendFinished);
    connect(m_excelSendWorker, &ExcelSendWorker::serialSentCountChanged,
            this, &ElaWidgetToolsDemo::onUpdateSentCount);
    connect(m_excelSendWorker, &ExcelSendWorker::commandSent,
            this, &ElaWidgetToolsDemo::onCommandSent);
    connect(m_excelSendWorker, &ExcelSendWorker::sendTimeOut,
            this, &ElaWidgetToolsDemo::onErrorTimeOut);
    connect(m_serialWorker, &SerialWorker::displayReceivedData,
            m_excelSendWorker, &ExcelSendWorker::onExternalResponseReceived,
            Qt::DirectConnection);

    m_excelSendThread->start();
    emit requestStartSend();
}

// 停止串口发送
void ElaWidgetToolsDemo::onStopSendSerial()
{
    qDebug() << "onStopSendSerial已触发!";
    if (m_excelSendWorker) {
        emit requestStopSend();
    }
    m_stopSendSerialButton->setEnabled(false);
}

// 串口发送完成清理
void ElaWidgetToolsDemo::onSerialSendFinished()
{
    qDebug() << "onSerialSendFinished已触发!";

    if (m_serialWorker && m_excelSendWorker) {
        disconnect(m_serialWorker, &SerialWorker::displayReceivedData,
                   m_excelSendWorker, &ExcelSendWorker::onExternalResponseReceived);
    }
    if (m_excelSendWorker) {
        m_excelSendWorker->deleteLater();
        m_excelSendWorker = nullptr;
    }
    if (m_excelSendThread && m_excelSendThread->isRunning()) {
        m_excelSendThread->quit();
        m_excelSendThread->wait();
    }

    m_sendLimitLineEdit->setEnabled(true);
    m_delayLineEdit->setEnabled(true);
    m_stopSendSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(true);
    m_openExcelButton->setEnabled(true);
    m_closeSerialButton->setEnabled(true);
}

// GUI 显示发送的数据
void ElaWidgetToolsDemo::onDisplaySentData(QByteArray msg)
{
    QString displayText;
    if (m_sendWithAN3CheckBox->isChecked()) {
        displayText = toHexDisplay(msg);
    } else {
        displayText = QString::fromUtf8(msg);
    }
    m_sendListWidget->addItem("[" + currentTimeString() + "] " + displayText);

    const int maxItems = m_maxDisplayItems;
    while (m_sendListWidget->count() > maxItems) {
        delete m_sendListWidget->takeItem(0);
    }
}

// GUI 显示接收的数据
void ElaWidgetToolsDemo::onDisplayReceivedData(QByteArray data)
{
    QString displayText;
    if (m_sendWithAN3CheckBox->isChecked()) {
        displayText = toHexDisplay(data);
    } else {
        displayText = QString::fromUtf8(data);
    }

    if (m_savedataWorker && m_savedataThread && m_savedataThread->isRunning()) {
        emit requestWriteSaveFile(displayText);
    }

    if (m_savedataWorker && m_sendAndReadRecordBool) {
        m_savedataWorker->setPendingTableData(displayText);
        m_savedataWorker->writeDataToTable(m_excelTableWidget);
    }

    m_receiveListWidget->addItem("[" + currentTimeString() + "] " + displayText);
    const int maxItems = m_maxDisplayItems;
    while (m_receiveListWidget->count() > maxItems) {
        delete m_receiveListWidget->takeItem(0);
    }

    if (m_singleRecvLog) {
        m_singleRecvLog->addItem("[" + currentTimeString() + "] " + displayText);
        while (m_singleRecvLog->count() > m_maxDisplayItems) {
            delete m_singleRecvLog->takeItem(0);
        }
    }
    emit requestValidateData(data);
}

// 十六进制显示
QString ElaWidgetToolsDemo::toHexDisplay(const QByteArray &data)
{
    QString result;
    for (unsigned char byte : data) {
        result.append(QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper());
    }
    return result.trimmed();
}
// ═══════════════════════════════════════════════════════════════
//  公共 HEX 转换工具（消除重复代码）
// ═══════════════════════════════════════════════════════════════
QByteArray ElaWidgetToolsDemo::hexStringToBytes(const QString& hexText) const
{
    QByteArray bytes;
    QStringList parts = hexText.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        bool ok;
        quint8 byte = static_cast<quint8>(part.toUInt(&ok, 16));
        if (ok) {
            bytes.append(static_cast<char>(byte));
        }
    }
    return bytes;
}
// ═══════════════════════════════════════════════════════════════
//  根据当前 HEX 模式决定字节如何显示
// ═══════════════════════════════════════════════════════════════
QString ElaWidgetToolsDemo::bytesToDisplayText(const QByteArray& data) const
{
    if (m_sendWithAN3CheckBox && m_sendWithAN3CheckBox->isChecked()) {
        return toHexDisplay(data);
    }
    return QString::fromUtf8(data);
}
// ═══════════════════════════════════════════════════════════════
//  同步更新所有页面上的错误统计显示
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::updateAllErrorDisplayLabels()
{
    QString errText = QString::number(m_errorCount);
    QString toText  = QString::number(m_errorTimeOut);

    if (m_errorCountDisplayLabel) {
        m_errorCountDisplayLabel->setText(errText);
    }
    if (m_errorTimeOutDisplayLabel) {
        m_errorTimeOutDisplayLabel->setText(toText);
    }
    if (m_dataPageErrorCountLabel) {
        m_dataPageErrorCountLabel->setText(errText);
    }
    if (m_dataPageErrorTimeOutLabel) {
        m_dataPageErrorTimeOutLabel->setText(toText);
    }

    // ========== 新增：同步三张卡片 ==========
    // 同步卡片
    if (m_errorCard)
        m_errorCard->setValue(QString::number(m_errorCount));
    if (m_timeoutCard)
        m_timeoutCard->setValue(QString::number(m_errorTimeOut));
    if (m_totalSendCard && m_sentCountDisplayLabel)
        m_totalSendCard->setValue(m_sentCountDisplayLabel->text());
}

// 更新发送计数
void ElaWidgetToolsDemo::onUpdateSentCount(int count)
{
    m_sentCountDisplayLabel->setText(QString::number(count));
    if (m_totalSendCard && m_sentCountDisplayLabel)
        m_totalSendCard->setValue(m_sentCountDisplayLabel->text());
}

// 错误检测
void ElaWidgetToolsDemo::onErrorDetected(QByteArray expected, QByteArray actual)
{
    m_errorCount++;
    updateAllErrorDisplayLabels();

    // 写入错误详情日志
    QString expectedDisplay = bytesToDisplayText(expected);
    QString actualDisplay   = bytesToDisplayText(actual);

    QString entry = QString("[%1] 第%2行 | 期望:\"%3\" | 实际:\"%4\" | 出错前已经发送:%5条")
        .arg(currentTimeString())
        .arg(m_currentRow + 1)     // 用户视角：第1行起
        .arg(expectedDisplay)
        .arg(actualDisplay)
        .arg(m_sentCountDisplayLabel->text());

    if (m_errorLogList) {
        m_errorLogList->addItem(entry);
        while (m_errorLogList->count() > m_maxDisplayItems) {
            delete m_errorLogList->takeItem(0);
        }
    }
    // 同时记录到文件
    emit requestWriteSaveFile("[错误] " + entry);
}


// 超时错误
void ElaWidgetToolsDemo::onErrorTimeOut()
{
    m_errorTimeOut++;
    updateAllErrorDisplayLabels();

    QString expectedDisplay = bytesToDisplayText(m_currentExpected);

    QString entry = QString("[%1] 第%2行 | 超时! 期望:\"%3\"")
        .arg(currentTimeString())
        .arg(m_currentRow + 1)
        .arg(expectedDisplay);

    if (m_errorLogList) {
        m_errorLogList->addItem(entry);
        while (m_errorLogList->count() > m_maxDisplayItems) {
            delete m_errorLogList->takeItem(0);
        }
    }

    emit requestWriteSaveFile("[超时] " + entry);
}


// 命令发送后传递期望返回值
void ElaWidgetToolsDemo::onCommandSent(int row, QByteArray expectedResponse)
{
    m_currentRow = row;
    m_currentExpected = expectedResponse;
    emit requestEnqueueExpected(expectedResponse);
}


// 清空发送与接收区域
void ElaWidgetToolsDemo::clearGUI()
{
    qDebug() << "clearGUI已触发";
    m_sendListWidget->clear();
    m_receiveListWidget->clear();
    emit requestResetValidator();
    m_errorCount = 0;
    m_errorTimeOut = 0;
    m_currentRow = -1;
    m_currentExpected.clear();
    updateAllErrorDisplayLabels();   // ← 一行搞定
    if (m_errorLogList) { m_errorLogList->clear(); }
}


// 清理第二列返回值
void ElaWidgetToolsDemo::resetTableForReadRecord()
{
    for (int row = 0; row < m_excelTableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_excelTableWidget->item(row, 1);
        if (item) {
            item->setText(QString());
        } else {
            m_excelTableWidget->setItem(row, 1, new QTableWidgetItem(QString()));
        }
    }
    emit requestResetTableWrite();
}

// 传递验证器模式
void ElaWidgetToolsDemo::updateValidatorMode()
{
    bool lossMode = m_testPacketLossCheckBox->isChecked();
    bool onlySend = m_onlySendDataModeCheckBox->isChecked();
    emit requestSetValidatorMode(lossMode, onlySend);
}

// 生成当前时间字符串
QString ElaWidgetToolsDemo::currentTimeString() const
{
    if (!m_timePrecisionComboBox)
        return QTime::currentTime().toString("hh:mm:ss.zzz");

    bool useMicro = (m_timePrecisionComboBox->currentIndex() == 1);

    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_us = duration_cast<microseconds>(now.time_since_epoch());
    auto total_sec = now_us.count() / 1000000;
    int hours   = (total_sec / 3600) % 24;
    int minutes = (total_sec / 60) % 60;
    int seconds = total_sec % 60;

    if (useMicro) {
        int microseconds = now_us.count() % 1000000;
        return QString("%1:%2:%3.%4")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(microseconds, 6, 10, QChar('0'));
    } else {
        auto now_ms = duration_cast<milliseconds>(now.time_since_epoch());
        int milliseconds = now_ms.count() % 1000;
        return QString("%1:%2:%3.%4")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
    }
}
// ═══════════════════════════════════════════════════════════════
//  窗口关闭事件：确保所有子线程安全退出 只需要重写 closeEvent，在里面做收尾工作，剩下的 Qt 全部自动处理
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::closeEvent(QCloseEvent* event)
{
    qDebug() << "窗口正在关闭，开始清理子线程...";

    // ──── ① 停止发送线程 ────
    if (m_excelSendWorker) {
        // 先设置 stopFlag，让 while(!m_stopFlag) 退出
        m_excelSendWorker->m_stopFlag = true;
        // 再通知事件循环退出
        emit requestStopSend();
    }

    // ──── ② 断开网络连接 ────
    if (m_networkClient && m_currentConnectionType == ConnectionType::Network) {
        emit requestStopSend();
    }

    // ──── ③ 关闭串口 ────
    if (m_serialWorker && m_currentConnectionType == ConnectionType::Serial) {
        emit requestSerialClose();
    }

    // ──── ④ 等待所有线程退出 ────
    // 给线程 2 秒时间结束当前循环
    if (m_excelSendThread && m_excelSendThread->isRunning()) {
        m_excelSendThread->quit();
        m_excelSendThread->wait(2000);
    }
    if (m_networkThread && m_networkThread->isRunning()) {
        m_networkThread->quit();
        m_networkThread->wait(2000);
    }
    if (m_serialThread && m_serialThread->isRunning()) {
        m_serialThread->quit();
        m_serialThread->wait(2000);
    }
    if (m_savedataThread && m_savedataThread->isRunning()) {
        m_savedataThread->quit();
        m_savedataThread->wait(2000);
    }
    if (m_validatorThread && m_validatorThread->isRunning()) {
        m_validatorThread->quit();
        m_validatorThread->wait(2000);
    }

    qDebug() << "所有子线程已退出，关闭窗口。";
    event->accept();
}
