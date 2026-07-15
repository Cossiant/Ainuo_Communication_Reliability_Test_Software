//
// Created by Cossiant on 2026/6/18.
//

#include "NetworkPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "QMessageBox"
#include "ElaIcon.h"

NetworkPage::NetworkPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent),
      m_mainWindow(mainWindow)
{
    initNetworkPage();
    initNavigation();
    initwindowConfig();

    // ═══════════════════════════════════════════════════════
    //  ★ 多线程：创建线程 + 将 NetworkWork 移入工作线程
    // ═══════════════════════════════════════════════════════
    m_networkThread = new QThread(this);
    m_networkWork   = new NetworkWork();

    m_networkWork->moveToThread(m_networkThread);

    connect(m_networkThread, &QThread::finished,
            m_networkWork,   &QObject::deleteLater);

    m_networkThread->start();

    qDebug() << "NetworkPage: 工作线程已启动，ID =" << m_networkThread;

    // ═══════════════════════════════════════════════════════
    //  ★ 连接超时定时器（主线程，3 秒）
    // ═══════════════════════════════════════════════════════
    m_connectTimeoutTimer = new QTimer(this);
    m_connectTimeoutTimer->setSingleShot(true);
    connect(m_connectTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_isConnecting) {
            m_isConnecting = false;
            QMessageBox::warning(m_mainWindow, "网络连接超时",
                                 "网络连接超时，请检查网络。\n"
                                 "请确认 IP 地址和端口号是否正确，目标设备是否在线。");
            QMetaObject::invokeMethod(m_networkWork, "disconnectFromHost",
                                      Qt::QueuedConnection);
        }
    });

    // ═══════════════════════════════════════════════════════
    //  连接 UI 控件 → NetworkWork
    // ═══════════════════════════════════════════════════════

    // ① 连接网络按钮
    connect(m_openNetworkButton, &ElaPushButton::clicked, this, [this]() {
        QString ipAddress = m_ipAddressEdit->text().trimmed();
        if (ipAddress.isEmpty()) {
            QMessageBox::warning(m_mainWindow, "警告", "请输入有效的 IP 地址！");
            return;
        }

        bool portOk = false;
        quint16 port = static_cast<quint16>(m_portEdit->text().toUInt(&portOk));
        if (!portOk || port == 0) {
            QMessageBox::warning(m_mainWindow, "警告", "请输入有效的端口号（1-65535）！");
            return;
        }

        bool hexMode      = m_networkHexSendCheckBox->isChecked();
        bool disableNagle = m_nagleCheckBox->isChecked();

        m_isConnecting = true;
        m_connectTimeoutTimer->start(3000);

        QMetaObject::invokeMethod(m_networkWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, hexMode));
        QMetaObject::invokeMethod(m_networkWork, "connectToHost",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, ipAddress),
                                  Q_ARG(quint16, port),
                                  Q_ARG(bool, disableNagle));
    });

    // ② 断开网络按钮
    connect(m_closeNetworkButton, &ElaPushButton::clicked, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();
        QMetaObject::invokeMethod(m_networkWork, "disconnectFromHost",
                                  Qt::QueuedConnection);
    });

    // ③ 单条发送按钮
    connect(m_singleSendBtn, &ElaPushButton::clicked, this, [this]() {
        QString text = m_singleSendInput->text();
        bool hexMode = m_networkHexSendCheckBox->isChecked();
        QMetaObject::invokeMethod(m_networkWork, "sendString",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text),
                                  Q_ARG(bool, hexMode));
    });

    // ④ HEX 勾选框
    connect(m_networkHexSendCheckBox, &ElaCheckBox::toggled, this, [this](bool checked) {
        QMetaObject::invokeMethod(m_networkWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, checked));
    });

    // ★ 新增：后缀选择变更 → 同步到工作线程
    connect(m_suffixComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, [this](int index) {
        QMetaObject::invokeMethod(m_networkWork, "setSuffixMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, index));
    });

    // ═══════════════════════════════════════════════════════════
    //  连接 NetworkWork 信号 → UI 更新
    // ═══════════════════════════════════════════════════════════

    // ⑤ 网络连接成功
    connect(m_networkWork, &NetworkWork::networkConnected, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        m_openNetworkButton->setEnabled(false);
        m_closeNetworkButton->setEnabled(true);

        m_ipAddressEdit->setEnabled(false);
        m_portEdit->setEnabled(false);

        m_singleSendBtn->setEnabled(true);
        m_excelOpenBtn->setEnabled(true);

        bool hasData = (m_excelTableWidget->rowCount() > 0);
        m_excelCaptureBtn->setEnabled(hasData);
        m_excelSendBtn->setEnabled(hasData);

        LED::setLED(m_networkLED, 2, 16);
    });

    // ⑥ 网络断开
    connect(m_networkWork, &NetworkWork::networkDisconnected, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        m_openNetworkButton->setEnabled(true);
        m_closeNetworkButton->setEnabled(false);

        m_ipAddressEdit->setEnabled(true);
        m_portEdit->setEnabled(true);

        m_singleSendBtn->setEnabled(false);
        m_excelOpenBtn->setEnabled(false);
        m_excelCaptureBtn->setEnabled(false);
        m_excelSendBtn->setEnabled(false);

        LED::setLED(m_networkLED, 0, 16);
    });

    // ⑦ 错误提示（静默，不弹窗 — 对齐 GPIB）
    connect(m_networkWork, &NetworkWork::errorOccurred, this, [this](const QString &msg) {
        bool wasConnecting = m_isConnecting;
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        // ★ 静默记录，不弹窗打断用户批量操作
        if (wasConnecting) {
            qDebug() << "NetworkPage: 连接阶段错误 -" << msg;
        } else {
            qDebug() << "NetworkPage: 非连接阶段错误 -" << msg;
        }
    });

    // ⑧ 发送日志行
    connect(m_networkWork, &NetworkWork::sendLogLine, this, [this](const QString &line) {
        if (m_logPaused) return;   // ★ 暂停时不更新日志
        if (m_singleSendLog) {
            m_singleSendLog->addItem(line);
            while (m_singleSendLog->count() > 100)
                delete m_singleSendLog->takeItem(0);
        }
        if (m_logSendList) {
            m_logSendList->addItem(line);
            while (m_logSendList->count() > 100)
                delete m_logSendList->takeItem(0);
        }
    });

    // ⑨ 接收日志行
    connect(m_networkWork, &NetworkWork::recvLogLine, this, [this](const QString &line) {
        if (m_logPaused) return;   // ★ 暂停时不更新日志
        if (m_singleRecvLog) {
            m_singleRecvLog->addItem(line);
            while (m_singleRecvLog->count() > 200)
                delete m_singleRecvLog->takeItem(0);
        }
        if (m_logRecvList) {
            m_logRecvList->addItem(line);
            while (m_logRecvList->count() > 200)
                delete m_logRecvList->takeItem(0);
        }
    });

    // ⑩ 接收计数
    connect(m_networkWork, &NetworkWork::recvCountChanged, this, [this](int count) {
        if (m_logRecvCountCard)
            m_logRecvCountCard->setValue(QString::number(count));
    });

    // ⑪ 清空按钮
    connect(m_logClearBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_singleSendLog)   m_singleSendLog->clear();
        if (m_singleRecvLog)   m_singleRecvLog->clear();
        if (m_logSendList)     m_logSendList->clear();
        if (m_logRecvList)     m_logRecvList->clear();
        QMetaObject::invokeMethod(m_networkWork, "resetRecvCount",
                                  Qt::QueuedConnection);
    });

    // ⑫ 暂停/恢复日志按钮
    connect(m_logPauseBtn, &ElaPushButton::clicked, this, [this]() {
        m_logPaused = !m_logPaused;
        if (m_logPaused) {
            m_logPauseBtn->setText("恢复日志");
            LED::setLED(m_logLED, 0, 14);
            qDebug() << "NetworkPage: 日志更新已暂停";
        } else {
            m_logPauseBtn->setText("暂停日志");
            LED::setLED(m_logLED, 2, 14);
            qDebug() << "NetworkPage: 日志更新已恢复";
        }
    });

    // ═══════════════════════════════════════════════════════
    //  创建 NetworkExcel
    // ═══════════════════════════════════════════════════════
    m_networkFunc = new NetworkExcel(this, this);
}

NetworkPage::~NetworkPage()
{
    m_isConnecting = false;
    m_connectTimeoutTimer->stop();

    if (m_networkWork) {
        QMetaObject::invokeMethod(m_networkWork, "disconnectFromHost",
                                  Qt::QueuedConnection);
    }

    m_networkThread->quit();

    if (!m_networkThread->wait(3000)) {
        qWarning() << "NetworkPage: 工作线程未能在 3 秒内退出，强制终止";
        m_networkThread->terminate();
        m_networkThread->wait();
    }
}

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void NetworkPage::initNetworkPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
// ═══════════════════════════════════════════════════════════════
void NetworkPage::initNavigation()
{
    m_mainWindow->addExpanderNode("网口通讯", NetworkMainPageKey, ElaIconType::NetworkWired);

    m_mainWindow->addPageNode("网络设置", _NetworkSettingPage,   NetworkMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _NetworkSendPage,      NetworkMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _NetworkExcelSendPage, NetworkMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _NetworkLogPage,       NetworkMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _NetworkErrorLogPage,  NetworkMainPageKey, ElaIconType::CircleExclamation);
}

void NetworkPage::initwindowConfig() {
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
//  页面：网络设置
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createSettingsPage() {
    _NetworkSettingPage = new QWidget();
    QVBoxLayout *_NetworkSettingLayout1 = new QVBoxLayout(_NetworkSettingPage);
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
    m_ipAddressEdit = new ElaLineEdit();
    m_ipAddressEdit->setPlaceholderText("例如: 192.168.1.100");
    m_ipAddressEdit->setText("192.168.1.100");

    ElaText* portLabel = new ElaText("端口号:");
    portLabel->setTextPixelSize(15);
    m_portEdit = new ElaLineEdit();
    m_portEdit->setPlaceholderText("例如: 5025");
    m_portEdit->setText("5025");

    grid->addWidget(ipLabel,               0, 0);
    grid->addWidget(m_ipAddressEdit,       0, 1);
    grid->addWidget(portLabel,             0, 2);
    grid->addWidget(m_portEdit,            0, 3);

    ElaText* statusLabel = new ElaText("网络状态:");
    statusLabel->setTextPixelSize(15);
    m_networkLED = new QLabel();
    LED::setLED(m_networkLED, 0, 16);

    grid->addWidget(statusLabel,           1, 0);
    grid->addWidget(m_networkLED,          1, 1);

    m_networkHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_networkHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    m_nagleCheckBox = new ElaCheckBox("禁用 Nagle 算法");
    m_nagleCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_networkHexSendCheckBox,  2, 0, 1, 2);
    grid->addWidget(m_nagleCheckBox,           2, 2, 1, 2);

    // 发送后缀
    ElaText* suffixLabel = new ElaText("发送后缀: ");
    suffixLabel->setTextPixelSize(15);
    m_suffixComboBox = new ElaComboBox();
    m_suffixComboBox->addItems({"无 (None)", "CR (\\r)", "LF (\\n)", "CRLF (\\r\\n)"});
    m_suffixComboBox->setCurrentIndex(0);  // 默认无后缀

    grid->addWidget(suffixLabel,           5, 0);
    grid->addWidget(m_suffixComboBox,      5, 2, 1, 2);

    // 去除 \r\n 勾选框
    m_networkStripCRLFCheckBox = new ElaCheckBox("比对时去除返回值中的 \\r\\n");
    m_networkStripCRLFCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_networkStripCRLFCheckBox, 3, 0, 1, 4);

    // 粘包分割
    m_networkSplitStickyCheckBox = new ElaCheckBox("启用粘包分割（按分隔符拆分返回值）");
    m_networkSplitStickyCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_networkSplitStickyCheckBox, 4, 0, 1, 2);

    m_networkSplitDelimiterComboBox = new ElaComboBox();
    m_networkSplitDelimiterComboBox->addItems({"\\n", "\\r", "\\r\\n"});
    m_networkSplitDelimiterComboBox->setCurrentIndex(0);
    m_networkSplitDelimiterComboBox->setStyleSheet("ElaComboBox { font-size: 14px; }");
    grid->addWidget(m_networkSplitDelimiterComboBox, 4, 2, 1, 2);

    m_openNetworkButton = new ElaPushButton("连接网络");
    m_openNetworkButton->setFixedHeight(35);
    m_closeNetworkButton = new ElaPushButton("断开网络");
    m_closeNetworkButton->setFixedHeight(35);
    m_closeNetworkButton->setEnabled(false);
    grid->addWidget(m_openNetworkButton,       6, 1);
    grid->addWidget(m_closeNetworkButton,      6, 3);

    _NetworkSettingLayout1->addWidget(_NetworkSettingGroup);
    _NetworkSettingLayout1->addStretch();
}

// ═══════════════════════════════════════════════════════════════
//  页面：单条发送
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createSendPage() {
    _NetworkSendPage = new QWidget();
    QVBoxLayout *_NetworkSendLayout = new QVBoxLayout(_NetworkSendPage);
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

    m_singleSendInput = new ElaLineEdit();
    m_singleSendInput->setPlaceholderText("在此输入要发送的命令...");
    m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_singleSendBtn = new ElaPushButton("通过网口发送");
    m_singleSendBtn->setFixedSize(160, 42);
    m_singleSendBtn->setEnabled(false);

    m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_singleSendBtn);
    btnRow->addWidget(m_singleSendClearBtn);
    btnRow->addStretch();
    inputLayout->addWidget(m_singleSendInput);
    inputLayout->addLayout(btnRow);
    _NetworkSendLayout->addWidget(inputGroup);

    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);
    QVBoxLayout* sendArea = new QVBoxLayout();
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    m_singleSendLog = new QListWidget();
    m_singleSendLog->setAlternatingRowColors(true);
    sendArea->addWidget(sendLabel);
    sendArea->addWidget(m_singleSendLog);

    QVBoxLayout* recvArea = new QVBoxLayout();
    ElaText* recvLabel = new ElaText("接收日志");
    recvLabel->setTextPixelSize(15);
    recvLabel->setTextStyle(ElaTextType::Subtitle);
    m_singleRecvLog = new QListWidget();
    m_singleRecvLog->setAlternatingRowColors(true);
    recvArea->addWidget(recvLabel);
    recvArea->addWidget(m_singleRecvLog);
    logRow->addLayout(sendArea, 1);
    logRow->addLayout(recvArea, 1);
    _NetworkSendLayout->addLayout(logRow, 1);

    connect(m_singleSendClearBtn, &ElaPushButton::clicked,
            this, &NetworkPage::clearSingleSendLog);
}

// ═══════════════════════════════════════════════════════════════
//  页面：Excel 表格发送
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createExcelSendPage()
{
    _NetworkExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_NetworkExcelSendPage);
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

    QVBoxLayout* bottomArea = new QVBoxLayout();
    bottomArea->setSpacing(12);

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

// ═══════════════════════════════════════════════════════════════
//  页面：发送日志
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createLogPage()
{
    _NetworkLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_NetworkLogPage);
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

    // ═══════════════════════════════════════════════════════
    //  ★ 使用 QGridLayout：卡片占2行，按钮右侧竖排2个
    // ═══════════════════════════════════════════════════════
    QGridLayout* cardRow = new QGridLayout();
    cardRow->setSpacing(12);

    m_logSentCountCard = new StatCard("总计发送", "0");
    m_logRecvCountCard = new StatCard("总计接收", "0");
    m_logStartTimeCard = new StatCard("开始时间", "--:--:--");

    // 三张卡片各占2行高度（rowSpan=2）
    cardRow->addWidget(m_logSentCountCard, 0, 0, 2, 1);
    cardRow->addWidget(m_logRecvCountCard, 0, 1, 2, 1);
    cardRow->addWidget(m_logStartTimeCard, 0, 2, 2, 1);
    cardRow->setColumnStretch(0, 1);
    cardRow->setColumnStretch(1, 1);
    cardRow->setColumnStretch(2, 1);

    // 右侧按钮区：竖排两个按钮
    QVBoxLayout* btnCol = new QVBoxLayout();
    btnCol->setSpacing(8);

    m_logClearBtn = new ElaPushButton("清空日志");
    m_logClearBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_logClearBtn);

    m_logPauseBtn = new ElaPushButton("暂停日志");
    m_logPauseBtn->setFixedSize(120, 38);
    btnCol->addWidget(m_logPauseBtn);

    btnCol->addStretch();
    cardRow->addLayout(btnCol, 0, 3, 2, 1, Qt::AlignTop);

    layout->addLayout(cardRow);

    // ═══════════════════════════════════════════════════════
    //  发送 / 接收日志区域
    // ═══════════════════════════════════════════════════════
    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    // ──── 左侧：发送日志 ────
    QVBoxLayout* sendArea = new QVBoxLayout();
    QHBoxLayout* sendTitleRow = new QHBoxLayout();        // ★ 标题 + LED
    sendTitleRow->setSpacing(8);
    ElaText* sendLabel = new ElaText("发送日志");
    sendLabel->setTextPixelSize(15);
    sendLabel->setTextStyle(ElaTextType::Subtitle);
    sendTitleRow->addWidget(sendLabel);

    m_logLED = new QLabel();
    m_logLED->setFixedSize(14, 14);
    LED::setLED(m_logLED, 2, 14);
    sendTitleRow->addWidget(m_logLED);
    sendTitleRow->addStretch();

    m_logSendList = new QListWidget();
    m_logSendList->setAlternatingRowColors(true);
    sendArea->addLayout(sendTitleRow);
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

}

// ═══════════════════════════════════════════════════════════════
//  页面：错误日志
// ═══════════════════════════════════════════════════════════════
void NetworkPage::createErrorLogPage()
{
    _NetworkErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(_NetworkErrorLogPage);
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

    QWidget* statsCard = createCard(_NetworkErrorLogPage);
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

    QWidget* tableCard = createCard(_NetworkErrorLogPage);
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

    connect(m_errorClearBtn, &ElaPushButton::clicked, this, &NetworkPage::clearErrors);
}

// ═══════════════════════════════════════════════════════════════
//  字节数组 → 错误日志显示文本
// ═══════════════════════════════════════════════════════════════
static QString bytesToDisplayText(const QByteArray &data, bool isHexMode)
{
    if (data.isEmpty())
        return QString::fromUtf8("—");
    if (isHexMode) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty()) {
            text.replace(QLatin1Char('\r'), QLatin1String("\\r"));
            text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
            return text;
        } else {
            return data.toHex(' ').toUpper();
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  添加超时错误
// ═══════════════════════════════════════════════════════════════
void NetworkPage::addTimeoutError(const QString &command, const QByteArray &expected)
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

    bool hexMode = m_networkHexSendCheckBox && m_networkHexSendCheckBox->isChecked();

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
void NetworkPage::addContentError(const QString &command,
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

    bool hexMode = m_networkHexSendCheckBox && m_networkHexSendCheckBox->isChecked();

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

void NetworkPage::clearErrors()
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

void NetworkPage::clearSingleSendLog()
{
    m_singleSendLog->clear();
    m_singleRecvLog->clear();
}

void NetworkPage::clearExcelSendLog()
{
    m_logSendList->clear();
    m_logRecvList->clear();

    m_logSentCountCard->setValue("0");
    m_logRecvCountCard->setValue("0");
    m_logStartTimeCard->setValue("--:--:--");

    if (m_networkWork)
        m_networkWork->resetRecvCount();
}
