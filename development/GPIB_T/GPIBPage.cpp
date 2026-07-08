// GPIBPage.cpp
// GPIB 通讯页面实现
// 对齐 SerialPage / NetworkPage 架构

#include "GPIBPage.h"
#include "ElaWindow.h"
#include "ElaText.h"
#include "QMessageBox"
#include "ElaIcon.h"

GPIBPage::GPIBPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent),
      m_mainWindow(mainWindow)
{
    initGpibPage();
    initNavigation();
    initwindowConfig();

    // ═══════════════════════════════════════════════════════
    //  ★ 多线程：创建线程 + 将 GPIBWork 移入工作线程
    // ═══════════════════════════════════════════════════════
    m_gpibThread = new QThread(this);
    m_gpibWork   = new GPIBWork();

    m_gpibWork->moveToThread(m_gpibThread);

    connect(m_gpibThread, &QThread::finished,
            m_gpibWork,   &QObject::deleteLater);

    m_gpibThread->start();

    qDebug() << "GPIBPage: 工作线程已启动，ID =" << m_gpibThread;

    // ═══════════════════════════════════════════════════════
    //  ★ 连接超时定时器（主线程，5 秒）
    // ═══════════════════════════════════════════════════════
    m_connectTimeoutTimer = new QTimer(this);
    m_connectTimeoutTimer->setSingleShot(true);
    connect(m_connectTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_isConnecting) {
            m_isConnecting = false;
            QMessageBox::warning(m_mainWindow, "GPIB 连接超时",
                                 "GPIB 连接超时，请检查设备连接。\n"
                                 "请确认板卡号、主地址是否正确，仪器是否上电。");
            QMetaObject::invokeMethod(m_gpibWork, "closeGPIBPort",
                                      Qt::QueuedConnection);
        }
    });

    // ═══════════════════════════════════════════════════════════
    //  连接 UI 控件 → GPIBWork
    // ═══════════════════════════════════════════════════════════

    // ① 打开 GPIB 按钮
    connect(m_openGpibButton, &ElaPushButton::clicked, this, [this]() {
        bool ok;
        int boardIndex = m_boardIndexEdit->text().toInt(&ok);
        if (!ok || boardIndex < 0) {
            QMessageBox::warning(m_mainWindow, "警告", "请输入有效的板卡号（0-3）！");
            return;
        }

        int primaryAddr = m_primaryAddrEdit->text().toInt(&ok);
        if (!ok || primaryAddr < 0 || primaryAddr > 30) {
            QMessageBox::warning(m_mainWindow, "警告", "请输入有效的主地址（0-30）！");
            return;
        }

        int secondaryAddr = m_secondaryAddrEdit->text().toInt(&ok);
        if (!ok) secondaryAddr = 0;
        if (secondaryAddr < 0 || secondaryAddr > 30) {
            QMessageBox::warning(m_mainWindow, "警告", "请输入有效的副地址（0-30，0 表示不使用）！");
            return;
        }

        int timeoutMs = m_timeoutEdit->text().toInt(&ok);
        if (!ok || timeoutMs < 100) timeoutMs = 3000;

        bool termCharEnabled = m_termCharEnabledCheckBox->isChecked();
        QString termCharStr = m_termCharEdit->text();
        char termChar = termCharStr.isEmpty() ? '\n' : termCharStr.at(0).toLatin1();
        bool sendEndEnabled = m_sendEndEnabledCheckBox->isChecked();
        bool hexMode = m_gpibHexSendCheckBox->isChecked();

        m_isConnecting = true;
        m_connectTimeoutTimer->start(5000);

        QMetaObject::invokeMethod(m_gpibWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, hexMode));
        QMetaObject::invokeMethod(m_gpibWork, "openGPIBPort",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, boardIndex),
                                  Q_ARG(int, primaryAddr),
                                  Q_ARG(int, secondaryAddr),
                                  Q_ARG(int, timeoutMs),
                                  Q_ARG(bool, termCharEnabled),
                                  Q_ARG(char, termChar),
                                  Q_ARG(bool, sendEndEnabled));
    });

    // ② 关闭 GPIB 按钮
    connect(m_closeGpibButton, &ElaPushButton::clicked, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();
        QMetaObject::invokeMethod(m_gpibWork, "closeGPIBPort",
                                  Qt::QueuedConnection);
    });

    // ③ 单条发送按钮
    connect(m_singleSendBtn, &ElaPushButton::clicked, this, [this]() {
        QString text = m_singleSendInput->text();
        bool hexMode = m_gpibHexSendCheckBox->isChecked();
        QMetaObject::invokeMethod(m_gpibWork, "sendString",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, text),
                                  Q_ARG(bool, hexMode));
    });

    // ④ HEX 勾选框 → 同步显示模式
    connect(m_gpibHexSendCheckBox, &ElaCheckBox::toggled, this, [this](bool checked) {
        QMetaObject::invokeMethod(m_gpibWork, "setHexDisplayMode",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, checked));
    });

    // ═══════════════════════════════════════════════════════════
    //  连接 GPIBWork 信号 → UI 更新
    // ═══════════════════════════════════════════════════════════

    // ⑤ GPIB 打开成功
    connect(m_gpibWork, &GPIBWork::gpibOpened, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        m_openGpibButton->setEnabled(false);
        m_closeGpibButton->setEnabled(true);

        m_boardIndexEdit->setEnabled(false);
        m_primaryAddrEdit->setEnabled(false);
        m_secondaryAddrEdit->setEnabled(false);
        m_timeoutEdit->setEnabled(false);
        m_termCharEdit->setEnabled(false);
        m_termCharEnabledCheckBox->setEnabled(false);
        m_sendEndEnabledCheckBox->setEnabled(false);

        m_singleSendBtn->setEnabled(true);
        m_excelOpenBtn->setEnabled(true);

        bool hasData = (m_excelTableWidget->rowCount() > 0);
        m_excelCaptureBtn->setEnabled(hasData);
        m_excelSendBtn->setEnabled(hasData);

        LED::setLED(m_gpibLED, 2, 16);   // 绿色
    });

    // ⑥ GPIB 关闭
    connect(m_gpibWork, &GPIBWork::gpibClosed, this, [this]() {
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        m_openGpibButton->setEnabled(true);
        m_closeGpibButton->setEnabled(false);

        m_boardIndexEdit->setEnabled(true);
        m_primaryAddrEdit->setEnabled(true);
        m_secondaryAddrEdit->setEnabled(true);
        m_timeoutEdit->setEnabled(true);
        m_termCharEdit->setEnabled(true);
        m_termCharEnabledCheckBox->setEnabled(true);
        m_sendEndEnabledCheckBox->setEnabled(true);

        m_singleSendBtn->setEnabled(false);
        m_excelOpenBtn->setEnabled(false);
        m_excelCaptureBtn->setEnabled(false);
        m_excelSendBtn->setEnabled(false);

        LED::setLED(m_gpibLED, 0, 16);   // 灰色
    });

    // ⑦ 错误提示
    connect(m_gpibWork, &GPIBWork::errorOccurred, this, [this](const QString &msg) {
        bool wasConnecting = m_isConnecting;
        m_isConnecting = false;
        m_connectTimeoutTimer->stop();

        if (wasConnecting) {
            QMessageBox::critical(m_mainWindow, "GPIB 错误", msg);
        } else {
            qDebug() << "GPIBPage: 非连接阶段错误 -" << msg;
        }
    });

    // ⑧ 发送日志行
    connect(m_gpibWork, &GPIBWork::sendLogLine, this, [this](const QString &line) {
        if (m_logPaused) return;
        if (m_singleSendLog) {
            m_singleSendLog->addItem(line);
            while (m_singleSendLog->count() > 200)
                delete m_singleSendLog->takeItem(0);
        }
        if (m_logSendList) {
            m_logSendList->addItem(line);
            while (m_logSendList->count() > 200)
                delete m_logSendList->takeItem(0);
        }
    });

    // ⑨ 接收日志行
    connect(m_gpibWork, &GPIBWork::recvLogLine, this, [this](const QString &line) {
        if (m_logPaused) return;
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
    connect(m_gpibWork, &GPIBWork::recvCountChanged, this, [this](int count) {
        if (m_logRecvCountCard)
            m_logRecvCountCard->setValue(QString::number(count));
    });

    // ⑪ 清空按钮
    connect(m_logClearBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_singleSendLog)   m_singleSendLog->clear();
        if (m_singleRecvLog)   m_singleRecvLog->clear();
        if (m_logSendList)     m_logSendList->clear();
        if (m_logRecvList)     m_logRecvList->clear();
        QMetaObject::invokeMethod(m_gpibWork, "resetRecvCount",
                                  Qt::QueuedConnection);
    });

    // ⑫ 暂停/恢复日志
    connect(m_logPauseBtn, &ElaPushButton::clicked, this, [this]() {
        m_logPaused = !m_logPaused;
        if (m_logPaused) {
            m_logPauseBtn->setText("恢复日志");
            LED::setLED(m_logLED, 0, 14);
            qDebug() << "GPIBPage: 日志更新已暂停";
        } else {
            m_logPauseBtn->setText("暂停日志");
            LED::setLED(m_logLED, 2, 14);
            qDebug() << "GPIBPage: 日志更新已恢复";
        }
    });

    // ═══════════════════════════════════════════════════════
    //  创建 GPIBExcel
    // ═══════════════════════════════════════════════════════
    m_gpibFunc = new GPIBExcel(this, this);
}

GPIBPage::~GPIBPage()
{
    m_isConnecting = false;
    m_connectTimeoutTimer->stop();

    if (m_gpibWork) {
        QMetaObject::invokeMethod(m_gpibWork, "closeGPIBPort",
                                  Qt::QueuedConnection);
    }

    m_gpibThread->quit();

    if (!m_gpibThread->wait(3000)) {
        qWarning() << "GPIBPage: 工作线程未能在 3 秒内退出，强制终止";
        m_gpibThread->terminate();
        m_gpibThread->wait();
    }
}

// ═══════════════════════════════════════════════════════════════
//  初始化：页面创建
// ═══════════════════════════════════════════════════════════════
void GPIBPage::initGpibPage() {
    createSettingsPage();
    createSendPage();
    createExcelSendPage();
    createLogPage();
    createErrorLogPage();
}

// ═══════════════════════════════════════════════════════════════
//  初始化：注册所有导航节点
// ═══════════════════════════════════════════════════════════════
void GPIBPage::initNavigation()
{
    m_mainWindow->addExpanderNode("GPIB通讯", GpibMainPageKey, ElaIconType::Microchip);

    m_mainWindow->addPageNode("GPIB设置", _GpibSettingPage,   GpibMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode("单条发送", _GpibSendPage,      GpibMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode("表格发送", _GpibExcelSendPage, GpibMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode("发送日志", _GpibLogPage,       GpibMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode("错误统计", _GpibErrorLogPage,  GpibMainPageKey, ElaIconType::CircleExclamation);
}

void GPIBPage::initwindowConfig() {
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
//  页面：GPIB 设置
// ═══════════════════════════════════════════════════════════════
void GPIBPage::createSettingsPage() {
    _GpibSettingPage = new QWidget();
    QVBoxLayout *_GpibSettingLayout = new QVBoxLayout(_GpibSettingPage);
    _GpibSettingLayout->setContentsMargins(30, 30, 30, 30);

    // ──── 标题 ────
    ElaText *title = new ElaText("GPIB 设置界面");
    title->setTextPixelSize(24);
    title->setTextStyle(ElaTextType::Title);
    _GpibSettingLayout->addWidget(title);

    // ════════════════════════════════════════════════════════
    //  GPIB 参数 GroupBox
    // ════════════════════════════════════════════════════════
    QGroupBox* group = new QGroupBox("GPIB 参数");
    QGridLayout* grid = new QGridLayout(group);
    grid->setSpacing(10);
    grid->setContentsMargins(30, 30, 30, 30);

    // ──── 第 0 行：板卡号 | 主地址 ────
    ElaText* boardLabel = new ElaText("板卡号:");
    boardLabel->setTextPixelSize(15);
    m_boardIndexEdit = new ElaLineEdit();
    m_boardIndexEdit->setText("0");
    m_boardIndexEdit->setPlaceholderText("GPIB 板卡号 (0-3)");

    ElaText* primaryLabel = new ElaText("主地址:");
    primaryLabel->setTextPixelSize(15);
    m_primaryAddrEdit = new ElaLineEdit();
    m_primaryAddrEdit->setText("1");
    m_primaryAddrEdit->setPlaceholderText("仪器主地址 (0-30)");

    grid->addWidget(boardLabel,          0, 0);
    grid->addWidget(m_boardIndexEdit,    0, 1);
    grid->addWidget(primaryLabel,        0, 2);
    grid->addWidget(m_primaryAddrEdit,   0, 3);

    // ──── 第 1 行：副地址 | 超时 ────
    ElaText* secondaryLabel = new ElaText("副地址:");
    secondaryLabel->setTextPixelSize(15);
    m_secondaryAddrEdit = new ElaLineEdit();
    m_secondaryAddrEdit->setText("0");
    m_secondaryAddrEdit->setPlaceholderText("副地址 (0=不使用)");

    ElaText* timeoutLabel = new ElaText("超时(ms):");
    timeoutLabel->setTextPixelSize(15);
    m_timeoutEdit = new ElaLineEdit();
    m_timeoutEdit->setText("499");
    m_timeoutEdit->setPlaceholderText("超时时间 (ms)");

    grid->addWidget(secondaryLabel,       1, 0);
    grid->addWidget(m_secondaryAddrEdit,  1, 1);
    grid->addWidget(timeoutLabel,         1, 2);
    grid->addWidget(m_timeoutEdit,        1, 3);

    // ──── 第 2 行：结束字符 | GPIB 状态 LED ────
    ElaText* termCharLabel = new ElaText("结束字符:");
    termCharLabel->setTextPixelSize(15);
    m_termCharEdit = new ElaLineEdit();
    m_termCharEdit->setText("\\n");
    m_termCharEdit->setPlaceholderText("如 \\n 表示换行");

    ElaText* statusLabel = new ElaText("GPIB 状态:");
    statusLabel->setTextPixelSize(15);
    m_gpibLED = new QLabel();
    LED::setLED(m_gpibLED, 0, 16);

    grid->addWidget(termCharLabel,    2, 0);
    grid->addWidget(m_termCharEdit,   2, 1);
    grid->addWidget(statusLabel,      2, 2);
    grid->addWidget(m_gpibLED,        2, 3);

    // ──── 第 3 行：勾选框 ────
    m_termCharEnabledCheckBox = new ElaCheckBox("启用结束字符检测");
    m_termCharEnabledCheckBox->setChecked(true);
    m_termCharEnabledCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    m_sendEndEnabledCheckBox = new ElaCheckBox("发送时附加 EOI 信号");
    m_sendEndEnabledCheckBox->setChecked(true);
    m_sendEndEnabledCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");

    grid->addWidget(m_termCharEnabledCheckBox, 3, 0, 1, 2);
    grid->addWidget(m_sendEndEnabledCheckBox,  3, 2, 1, 2);

    // ──── 第 4 行：HEX 发送勾选框 ────
    m_gpibHexSendCheckBox = new ElaCheckBox("以HEX格式发送（AN3.0）");
    m_gpibHexSendCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_gpibHexSendCheckBox, 4, 0, 1, 2);

    // ──── ★ 第 5 行：去除 \r\n 勾选框 ────
    m_gpibStripCRLFCheckBox = new ElaCheckBox("比对时去除返回值中的 \\r\\n");
    m_gpibStripCRLFCheckBox->setStyleSheet("ElaCheckBox { font-size: 14px; }");
    grid->addWidget(m_gpibStripCRLFCheckBox, 5, 0, 1, 4);

    // ──── 第 6 行：打开/关闭按钮 ────
    m_openGpibButton = new ElaPushButton("打开 GPIB");
    m_openGpibButton->setFixedHeight(35);
    m_closeGpibButton = new ElaPushButton("关闭 GPIB");
    m_closeGpibButton->setFixedHeight(35);
    m_closeGpibButton->setEnabled(false);
    grid->addWidget(m_openGpibButton,   6, 1);
    grid->addWidget(m_closeGpibButton,  6, 3);

    _GpibSettingLayout->addWidget(group);
    _GpibSettingLayout->addStretch();
}

// ═══════════════════════════════════════════════════════════════
//  页面：单条发送
// ═══════════════════════════════════════════════════════════════
void GPIBPage::createSendPage() {
    _GpibSendPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(_GpibSendPage);
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

    m_singleSendInput = new ElaLineEdit();
    m_singleSendInput->setPlaceholderText("在此输入要发送的 SCPI 命令，如 *IDN? ...");
    m_singleSendInput->setFixedHeight(42);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_singleSendBtn = new ElaPushButton("通过 GPIB 发送");
    m_singleSendBtn->setFixedSize(160, 42);
    m_singleSendBtn->setEnabled(false);

    m_singleSendClearBtn = new ElaPushButton("清空发送日志");
    m_singleSendClearBtn->setFixedSize(120, 38);

    btnRow->addWidget(m_singleSendBtn);
    btnRow->addWidget(m_singleSendClearBtn);
    btnRow->addStretch();

    inputLayout->addWidget(m_singleSendInput);
    inputLayout->addLayout(btnRow);
    layout->addWidget(inputGroup);

    // 日志区域（发送左 + 接收右）
    QHBoxLayout* logRow = new QHBoxLayout();
    logRow->setSpacing(12);

    QVBoxLayout* leftCol = new QVBoxLayout();
    ElaText* sendLogLabel = new ElaText("发送日志:");
    sendLogLabel->setTextPixelSize(14);
    m_singleSendLog = new QListWidget();
    leftCol->addWidget(sendLogLabel);
    leftCol->addWidget(m_singleSendLog);

    QVBoxLayout* rightCol = new QVBoxLayout();
    ElaText* recvLogLabel = new ElaText("接收日志:");
    recvLogLabel->setTextPixelSize(14);
    m_singleRecvLog = new QListWidget();
    rightCol->addWidget(recvLogLabel);
    rightCol->addWidget(m_singleRecvLog);

    logRow->addLayout(leftCol);
    logRow->addLayout(rightCol);
    layout->addLayout(logRow);

    connect(m_singleSendClearBtn, &ElaPushButton::clicked, this, &GPIBPage::clearSingleSendLog);
}

// ═══════════════════════════════════════════════════════════════
//  页面：表格发送（对齐 NetworkPage）
// ═══════════════════════════════════════════════════════════════
void GPIBPage::createExcelSendPage() {
    _GpibExcelSendPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_GpibExcelSendPage);
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
//  页面：发送日志（对齐 NetworkPage）
// ═══════════════════════════════════════════════════════════════
void GPIBPage::createLogPage() {
    _GpibLogPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_GpibLogPage);
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
//  页面：错误日志（对齐 NetworkPage）
// ═══════════════════════════════════════════════════════════════
void GPIBPage::createErrorLogPage() {
    _GpibErrorLogPage = new QWidget();
    QVBoxLayout* root = new QVBoxLayout(_GpibErrorLogPage);
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

    QWidget* statsCard = createCard(_GpibErrorLogPage);
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

    QWidget* tableCard = createCard(_GpibErrorLogPage);
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

    connect(m_errorClearBtn, &ElaPushButton::clicked, this, &GPIBPage::clearErrors);
}
// ═══════════════════════════════════════════════════════════════
//  字节数组 → 错误日志显示文本
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
            return data.toHex(' ').toUpper();
    }
}

// ═══════════════════════════════════════════════════════════════
//  添加超时错误（对齐 NetworkPage）
// ═══════════════════════════════════════════════════════════════
void GPIBPage::addTimeoutError(const QString &command, const QByteArray &expected)
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

    bool hexMode = m_gpibHexSendCheckBox && m_gpibHexSendCheckBox->isChecked();

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
//  添加内容错误（对齐 NetworkPage）
// ═══════════════════════════════════════════════════════════════
void GPIBPage::addContentError(const QString &command,
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

    bool hexMode = m_gpibHexSendCheckBox && m_gpibHexSendCheckBox->isChecked();

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
void GPIBPage::clearErrors()
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

void GPIBPage::clearSingleSendLog()
{
    if (m_singleSendLog) m_singleSendLog->clear();
    if (m_singleRecvLog) m_singleRecvLog->clear();
}

void GPIBPage::clearExcelSendLog()
{
    m_logSendList->clear();
    m_logRecvList->clear();

    m_logSentCountCard->setValue("0");
    m_logRecvCountCard->setValue("0");
    m_logStartTimeCard->setValue("--:--:--");

    if (m_gpibWork)
        m_gpibWork->resetRecvCount();
}

