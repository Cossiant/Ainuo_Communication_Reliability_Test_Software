#include "gui.h"

GUI::GUI(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    // 注册串口枚举类型
    qRegisterMetaType<QSerialPort::DataBits>("QSerialPort::DataBits");
    qRegisterMetaType<QSerialPort::Parity>("QSerialPort::Parity");
    qRegisterMetaType<QSerialPort::StopBits>("QSerialPort::StopBits");
    qRegisterMetaType<QSerialPort::FlowControl>("QSerialPort::FlowControl");

    //创建线程对象
    m_networkThread = new QThread;
    m_excelSendThread = new QThread;
    m_serialThread = new QThread;
    //设置主窗口
    setWindowTitle(tr("Ainuo通用通讯可靠性测试软件"));
    resize(1000, 600);
    m_mainLayout = new QGridLayout(this);

    //初始化label
    m_excelReadLabel = new QLabel(tr("读取到的Excel表格的数据"));
    m_receiveLabel = new QLabel(tr("接受的数据"));
    m_sendLabel = new QLabel(tr("发送的数据"));
    m_serverIpLabel = new QLabel(tr("电源的网络地址:"));
    m_portLabel = new QLabel(tr("端口号:"));
    m_delayLabel = new QLabel(tr("每条命令发送延时(ms)："));
    m_sendLimitLabel = new QLabel(tr("发送条数(0为一直循环):"));
    m_sentCountLabel = new QLabel(tr("总计发送命令："));
    m_sentCountDisplayLabel = new QLabel(tr("0"));
    m_serialPortLabel = new QLabel(tr("串口端口号:"));
    m_baudRateLabel = new QLabel(tr("串口波特率:"));
    m_dataBitsLabel = new QLabel(tr("串口数据位:"));
    m_stopBitsLabel = new QLabel(tr("串口停止位:"));
    m_parityLabel = new QLabel(tr("串口校验位:"));
    m_errorCountLabel = new QLabel(tr("返回错误次数:"));
    m_errorCountDisplayLabel = new QLabel(tr("0"));

    //初始化下拉菜单
    m_serialPortComboBox = new QComboBox(this);
    m_baudRateComboBox = new QComboBox(this);
    m_dataBitsComboBox = new QComboBox(this);
    m_stopBitsComboBox = new QComboBox(this);
    m_parityComboBox = new QComboBox(this);

    //这里获取当前串口端口号
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        m_serialPortComboBox->addItem(tr("无可用串口"));
    } else {
        for (const QSerialPortInfo &info : portList) {
            //将已有的端口号显示出来
            m_serialPortComboBox->addItem(info.portName());
        }
    }
    //串口列表内容
    QStringList baudRates = {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"};
    QStringList dataBits = {"5", "6", "7", "8"};
    QStringList stopBits = {"1", "1.5", "2"};
    QStringList parities = {"None", "Even", "Odd", "Space", "Mark"};
    //将列表内容添加到Combobox当中
    m_baudRateComboBox->addItems(baudRates);
    m_dataBitsComboBox->addItems(dataBits);
    m_stopBitsComboBox->addItems(stopBits);
    m_parityComboBox->addItems(parities);
    //初始化串口参数
    m_baudRateComboBox->setCurrentText("115200"); // 默认值
    m_dataBitsComboBox->setCurrentText("8");
    m_stopBitsComboBox->setCurrentText("1");
    m_parityComboBox->setCurrentText("None");

    //初始化LineEdit
    m_serverIpLineEdit = new QLineEdit;
    m_portLineEdit = new QLineEdit;
    m_delayLineEdit = new QLineEdit;
    m_sendLimitLineEdit = new QLineEdit;

    //设置参数
    m_serverIpLineEdit->setText("127.0.0.1");
    m_portLineEdit->setText("20108");
    m_delayLineEdit->setText("50");
    m_sendLimitLineEdit->setText("0");

    //初始化按钮
    m_connectButton = new QPushButton(tr("连接到电源"));
    m_disconnectButton = new QPushButton(tr("断开链接"));
    m_openExcelButton = new QPushButton(tr("打开excel并读取"));
    m_sendExcelButton = new QPushButton(tr("发送excel命令到电源"));
    m_stopSendExcelButton = new QPushButton(tr("停止发送excel命令到电源"));
    m_openSerialButton = new QPushButton(tr("打开串口"));
    m_closeSerialButton = new QPushButton(tr("关闭串口"));
    m_sendSerialButton = new QPushButton(tr("通过串口发送excel命令"));
    m_stopSendSerialButton = new QPushButton(tr("停止串口发送excel命令"));

    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(true);
    m_portLineEdit->setEnabled(true);
    m_serverIpLineEdit->setEnabled(true);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(true);
    m_closeSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_stopSendSerialButton->setEnabled(false);

    connect(m_openExcelButton, SIGNAL(clicked()), this, SLOT(onOpenExcelClicked()));//信号量链接
    connect(m_connectButton, SIGNAL(clicked()), this, SLOT(onConnectServer()));//信号量链接
    connect(m_sendExcelButton, SIGNAL(clicked()), this, SLOT(onStartSendExcel()));//信号量链接
    connect(m_disconnectButton, SIGNAL(clicked()), this, SLOT(onDisconnectServer()));//信号量链接
    connect(m_stopSendExcelButton, SIGNAL(clicked()), this, SLOT(onStopSendExcel()));//信号量链接
    connect(m_openSerialButton, SIGNAL(clicked()), this, SLOT(onOpenSerial()));
    connect(m_closeSerialButton, SIGNAL(clicked()), this, SLOT(onCloseSerial()));
    connect(m_sendSerialButton,SIGNAL(clicked()),this,SLOT(onStartSendSerial()));
    connect(m_stopSendSerialButton,SIGNAL(clicked()),this,SLOT(onStopSendSerial()));

    m_receiveListWidget = new QListWidget;                             //接受命令显示窗口
    m_sendListWidget = new QListWidget;                                //发送命令显示窗口
    m_excelTableWidget = new QTableWidget;                              //读取excel数据窗口

    m_excelReader = new ExcelReader;

    m_excelTableWidget->setColumnCount(2);                              //初始化为2列
    m_excelTableWidget->setColumnWidth(1, 300);                          //设置第二列的列宽为300像素
    m_excelTableWidget->setHorizontalHeaderLabels(QStringList() << "需发送的命令" << "正确的返回值");
    m_excelTableWidget->setRowCount(10);                                //初始化为10行
    m_excelTableWidget->setItem(0, 0, new QTableWidgetItem("等待读取excel表格"));

    m_mainLayout->addWidget(m_excelReadLabel, 0, 0, 1, 2);
    m_mainLayout->addWidget(m_receiveLabel, 0, 2, 1, 2);
    m_mainLayout->addWidget(m_sendLabel, 0, 4, 1, 2);

    m_mainLayout->addWidget(m_excelTableWidget, 1, 0, 1, 2);
    m_mainLayout->addWidget(m_receiveListWidget, 1, 2, 1, 2);
    m_mainLayout->addWidget(m_sendListWidget, 1, 4, 1, 2);

    m_mainLayout->addWidget(m_openExcelButton, 2, 0, 1, 2);
    m_mainLayout->addWidget(m_serverIpLabel, 2, 2, 1, 1);
    m_mainLayout->addWidget(m_serverIpLineEdit, 2, 3, 1, 1);
    m_mainLayout->addWidget(m_serialPortLabel, 2, 4, 1, 1);
    m_mainLayout->addWidget(m_serialPortComboBox, 2, 5, 1, 1);

    m_mainLayout->addWidget(m_portLabel, 3, 2, 1, 1);
    m_mainLayout->addWidget(m_portLineEdit, 3, 3, 1, 1);
    m_mainLayout->addWidget(m_sendLimitLabel, 3, 0, 1, 1);
    m_mainLayout->addWidget(m_sendLimitLineEdit, 3, 1, 1, 1);
    m_mainLayout->addWidget(m_baudRateLabel, 3, 4, 1, 1);
    m_mainLayout->addWidget(m_baudRateComboBox, 3, 5, 1, 1);

    m_mainLayout->addWidget(m_delayLabel, 4, 0, 1, 1);
    m_mainLayout->addWidget(m_delayLineEdit, 4, 1, 1, 1);
    m_mainLayout->addWidget(m_errorCountLabel, 4, 2, 1, 1);
    m_mainLayout->addWidget(m_errorCountDisplayLabel, 4, 3, 1, 1);
    m_mainLayout->addWidget(m_dataBitsLabel, 4, 4, 1, 1);
    m_mainLayout->addWidget(m_dataBitsComboBox, 4, 5, 1, 1);

    m_mainLayout->addWidget(m_sendExcelButton, 5, 0, 1, 1);
    m_mainLayout->addWidget(m_stopSendExcelButton, 5, 1, 1, 1);
    m_mainLayout->addWidget(m_sentCountLabel, 5, 2, 1, 1);
    m_mainLayout->addWidget(m_sentCountDisplayLabel, 5, 3, 1, 1);
    m_mainLayout->addWidget(m_parityLabel, 5, 4, 1, 1);
    m_mainLayout->addWidget(m_parityComboBox, 5, 5, 1, 1);

    m_mainLayout->addWidget(m_sendSerialButton, 6, 0, 1, 1);
    m_mainLayout->addWidget(m_stopSendSerialButton, 6, 1, 1, 1);
    m_mainLayout->addWidget(m_disconnectButton, 6, 2, 1, 1);
    m_mainLayout->addWidget(m_connectButton, 6, 3, 1, 1);
    m_mainLayout->addWidget(m_stopBitsLabel, 6, 4, 1, 1);
    m_mainLayout->addWidget(m_stopBitsComboBox, 6, 5, 1, 1);

    m_mainLayout->addWidget(m_openSerialButton, 7, 4, 1, 1);
    m_mainLayout->addWidget(m_closeSerialButton, 7, 5, 1, 1);

    // 设置第 0 列和第 1 列可拉伸，比例为 1:1
    m_mainLayout->setColumnStretch(0, 1);
    m_mainLayout->setColumnStretch(1, 1);
    // 第 2 列和第 3 列不拉伸或比例较小
    m_mainLayout->setColumnStretch(2, 0);
    m_mainLayout->setColumnStretch(3, 0);
}

//读取excel表格
void GUI::onOpenExcelClicked()
{
    // 弹出文件选择对话框，获取 Excel 文件路径
    m_fileDialog.setFileMode(QFileDialog::ExistingFile);
    m_fileDialog.setViewMode(QFileDialog::Detail);
    m_fileDialog.setOption(QFileDialog::ReadOnly, true);
    m_fileDialog.setDirectory(QString("C:/"));
    m_fileDialog.setNameFilter(QString("所有文件(*.*);;Microsoft Excel工作表(*.xlsx);;Microsoft Excel 97-2003工作表(*.xls)"));

    if (m_fileDialog.exec()) {
        QStringList files = m_fileDialog.selectedFiles();
        for (auto fname : files) {
            if (m_excelReader->loadExcelToTable(fname, m_excelTableWidget, this)) {
                QMessageBox::information(this, "提示", QStringLiteral("Excel 文件读取完成！\r\n总行数：") + QString::number(m_excelReader->totalRows));
                //只有正确读取才能允许发送！
                if (m_currentConnectionType == ConnectionType::Network) {
                    m_sendExcelButton->setEnabled(true);
                } else if (m_currentConnectionType == ConnectionType::Serial) {
                    m_sendSerialButton->setEnabled(true);
                }
            }
        }
    }
}

//链接服务器
void GUI::onConnectServer()
{
    qDebug() << "debug:onConnectServer已经触发";
    //创建工作对象
    m_networkClient = new NetworkClient();
    //告诉Network接受和发送的数据在什么地方
    m_networkClient->contentListWidget = m_receiveListWidget;
    m_networkClient->sendListWidget = m_sendListWidget;
    //移动到子线程当中
    m_networkClient->moveToThread(m_networkThread);
    //链接信号函数，从GUI到NetworkClient
    connect(this, &GUI::requestNetworkConnect, m_networkClient, &NetworkClient::onNetworkConnected);
    connect(this, &GUI::requestSendNetworkData, m_networkClient, &NetworkClient::sendNetworkData);
    //链接信号函数，从NetWork到GUI
    connect(m_networkClient, &NetworkClient::displaySentData, this, &GUI::onDisplaySentData);
    connect(m_networkClient, &NetworkClient::displayReceivedData, this, &GUI::onDisplayReceivedData);

    //传递信号，启动Network
    emit requestNetworkConnect(m_portLineEdit->text().toInt(), QHostAddress(m_serverIpLineEdit->text()));

    //子线程启动
    m_networkThread->start();

    //设置发送方式和按钮控件
    m_currentConnectionType = ConnectionType::Network;
    m_disconnectButton->setEnabled(true);
    m_connectButton->setEnabled(false);
    m_portLineEdit->setEnabled(false);
    m_serverIpLineEdit->setEnabled(false);
    m_openExcelButton->setEnabled(true);
    m_openSerialButton->setEnabled(false);
}

//停止链接到电源
void GUI::onDisconnectServer()
{
    qDebug() << "debug:onDisconnectServer已经触发";
    if (m_networkClient) {
        // 请求Network断开连接并清理，然后删除自身
        // 可以添加一个信号槽，让Network自己删除
        // 或者调用Network->deleteLater()，并退出线程
        m_networkClient->deleteLater();
        m_networkClient = nullptr;
    }
    if (m_networkThread && m_networkThread->isRunning()) {
        m_networkThread->quit();
        m_networkThread->wait();
    }
    qDebug() << "已断开网络连接";
    // 重置选择发送方式状态
    m_currentConnectionType = ConnectionType::None;
    // 重置按钮状态
    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(true);
    m_portLineEdit->setEnabled(true);
    m_serverIpLineEdit->setEnabled(true);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(true);
    m_sendSerialButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
}

//非阻塞式发送线程
void GUI::onStartSendExcel()
{
    qDebug() << "debug:onStartSendExcel 非阻塞线程启动";

    // 首先禁止其他按钮状态
    //不允许在点击发送之后还可以读取excel！
    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(true);
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_stopSendSerialButton->setEnabled(false);
    // 禁止重复点击发送
    m_sendLimitLineEdit->setEnabled(false);
    m_delayLineEdit->setEnabled(false);

    // 创建新线程和工作对象
    m_excelSendWorker = new ExcelSendWorker();
    //传递要发送的数据到excel发送work
    m_excelSendWorker->m_table = m_excelTableWidget;                                    //传递发送的表格table
    m_excelSendWorker->m_totalRows = m_excelReader->totalRows;                     //传递表格的行数
    m_excelSendWorker->m_delayMs = m_delayLineEdit->text().toInt();                   //传递延时时间
    m_excelSendWorker->m_repeatLimit = m_sendLimitLineEdit->text().toInt();    //传递发送次数限制

    //移动到子线程当中
    m_excelSendWorker->moveToThread(m_excelSendThread);

    //链接信号与槽
    connect(this, &GUI::requestStartSend, m_excelSendWorker, &ExcelSendWorker::startNetworkWork);
    // 关键：使用直接连接，让 stopWork 在 GUI 线程被调用
    //    Qt 有几种连接方式（Qt::ConnectionType）：
    //    Qt::AutoConnection（默认）：若发送者与接收者在同一线程，则为直接连接（Qt::DirectConnection）；若在不同线程，则为队列连接（Qt::QueuedConnection）。
    //    Qt::DirectConnection：发送信号时，立即在当前线程调用槽函数，就像直接调用普通函数一样。
    //    Qt::QueuedConnection：将槽函数调用封装为事件，投递到接收者所在线程的事件队列中，等待该线程的事件循环处理。
    //因为发送任务是死循环的，如果此时停止信号使用队列链接，那么将永远也不会执行到，那么会造成主线程阻塞等待停止且一直不会停止
    connect(this, &GUI::requestStopSend, m_excelSendWorker, &ExcelSendWorker::stopNetworkWork, Qt::DirectConnection);
    connect(m_excelSendWorker, &ExcelSendWorker::sendCommand, m_networkClient, &NetworkClient::sendNetworkData);
    connect(m_excelSendWorker, &ExcelSendWorker::finished, this, &GUI::onSendFinished);
    connect(m_excelSendWorker, &ExcelSendWorker::sentCountChanged, this, &GUI::onUpdateSentCount);

    //子线程启动
    m_excelSendThread->start();

    //发送信号，启动发送
    emit requestStartSend();
}

//停止Excel发送
void GUI::onStopSendExcel()
{
    qDebug() << "手动停止发送";
    if (m_excelSendWorker) {
        emit requestStopSend();                // 仅发送停止信号，不清理
    }
    m_stopSendExcelButton->setEnabled(false);     // 防止重复点击
}

//清理发送线程
void GUI::onSendFinished()
{
    qDebug() << "发送线程结束，开始清理资源";
    if (m_excelSendWorker) {
        m_excelSendWorker->deleteLater();
        m_excelSendWorker = nullptr;
    }
    if (m_excelSendThread && m_excelSendThread->isRunning()) {
        m_excelSendThread->quit();
        m_excelSendThread->wait();
    }

    // 恢复界面控件
    m_sendLimitLineEdit->setEnabled(true);
    m_delayLineEdit->setEnabled(true);
    m_stopSendExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(true);
    m_openExcelButton->setEnabled(true);
    m_disconnectButton->setEnabled(true);
}

//GUI显示发送的数据
void GUI::onDisplaySentData(QString msg)
{
    // 添加新条目
    m_sendListWidget->addItem(msg);

    // 限制最多显示 1000 条
    const int maxItems = m_maxDisplayItems;
    while (m_sendListWidget->count() > maxItems) {
        delete m_sendListWidget->takeItem(0);  // 删除最旧的一条（索引0）
    }
}

//GUI显示收到的数据
void GUI::onDisplayReceivedData(QString msg)
{
    m_receiveListWidget->addItem(msg);

    const int maxItems = m_maxDisplayItems;
    while (m_receiveListWidget->count() > maxItems) {
        delete m_receiveListWidget->takeItem(0);
    }
}

//GUI显示统计发送了多少次
void GUI::onUpdateSentCount(int count)
{
    m_sentCountDisplayLabel->setText(QString::number(count));
}

//点击按钮执行操作，创建对象，然后发送数据，启动子线程，最后在子线程完成串口链接
void GUI::onOpenSerial()
{
    qDebug() << "debug:onOpenSerial已经触发";
    // ---------- 1. 获取用户选择的参数 ----------
    QString portName = m_serialPortComboBox->currentText();
    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(this, "警告", "请选择有效的串口端口！");
        return;
    }

    qint32 baudRate = m_baudRateComboBox->currentText().toInt();

    // 转换数据位
    QSerialPort::DataBits dataBits;
    QString dataBitsStr = m_dataBitsComboBox->currentText();
    if (dataBitsStr == "5")      dataBits = QSerialPort::Data5;
    else if (dataBitsStr == "6") dataBits = QSerialPort::Data6;
    else if (dataBitsStr == "7") dataBits = QSerialPort::Data7;
    else                         dataBits = QSerialPort::Data8;  // 默认8位

    // 转换停止位
    QSerialPort::StopBits stopBits;
    QString stopBitsStr = m_stopBitsComboBox->currentText();
    if (stopBitsStr == "1.5")         stopBits = QSerialPort::OneAndHalfStop;
    else if (stopBitsStr == "2")      stopBits = QSerialPort::TwoStop;
    else                              stopBits = QSerialPort::OneStop;  // 默认1位

    // 转换校验位
    QSerialPort::Parity parity;
    QString parityStr = m_parityComboBox->currentText();
    if (parityStr == "Even")          parity = QSerialPort::EvenParity;
    else if (parityStr == "Odd")      parity = QSerialPort::OddParity;
    else if (parityStr == "Space")    parity = QSerialPort::SpaceParity;
    else if (parityStr == "Mark")     parity = QSerialPort::MarkParity;
    else                              parity = QSerialPort::NoParity;   // None

    // 流控目前 UI 无对应控件，使用默认值 None
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;

    // ---------- 2. 创建工作对象并移动到子线程 ----------
    m_serialWorker = new SerialWorker;
    m_serialWorker->moveToThread(m_serialThread);

    // 连接信号：GUI 的 requestSerialOpen（带参） -> SerialWorker 的槽
    connect(this, &GUI::requestSerialOpen, m_serialWorker, &SerialWorker::onSerialStart);
    connect(this, &GUI::requestSerialClose, m_serialWorker, &SerialWorker::onSerialStop);
    //链接槽函数
    connect(m_serialWorker, &SerialWorker::serialClosed, this, &GUI::onSerialClosed);
    connect(m_serialWorker, &SerialWorker::displaySentData, this, &GUI::onDisplaySentData);
    connect(m_serialWorker, &SerialWorker::displayReceivedData, this, &GUI::onDisplayReceivedData);

//    //链接信号函数，从NetWork到GUI
//    connect(m_networkClient, &NetworkClient::displaySentData, this, &GUI::onDisplaySentData);
//    connect(m_networkClient, &NetworkClient::displayReceivedData, this, &GUI::onDisplayReceivedData);

    // 启动子线程
    m_serialThread->start();

    // ---------- 3. 发射信号，传递参数到工作线程 ----------
    emit requestSerialOpen(portName, baudRate, dataBits, parity, stopBits, flowControl);

    // ---------- 4. 更新界面按钮状态 ----------
    //设置发送方式和按钮控件
    m_currentConnectionType = ConnectionType::Serial;
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(true);
    m_connectButton->setEnabled(false);
    // 禁用串口参数配置控件，防止运行时修改
    m_serialPortComboBox->setEnabled(false);
    m_baudRateComboBox->setEnabled(false);
    m_dataBitsComboBox->setEnabled(false);
    m_stopBitsComboBox->setEnabled(false);
    m_parityComboBox->setEnabled(false);
    m_openExcelButton->setEnabled(true);
}

//关闭串口函数
void GUI::onCloseSerial()
{
    qDebug() << "手动请求关闭串口";
    if (m_serialWorker) {
        emit requestSerialClose();   // 通知工作线程关闭串口
    }
    // 注意：不在这里立即清理线程，等待 SerialWorker 发出 serialClosed 信号后再处理
}

void GUI::onSerialClosed()
{
    qDebug() << "串口已关闭，清理线程资源";
    // 退出子线程并等待
    if (m_serialThread && m_serialThread->isRunning()) {
        m_serialThread->quit();
        m_serialThread->wait();
    }
    // 删除工作对象（如果还未被 deleteLater 清理）
    if (m_serialWorker) {
        m_serialWorker->deleteLater();
        m_serialWorker = nullptr;
    }
    // 重置选择发送方式状态
    m_currentConnectionType = ConnectionType::None;
    // 恢复界面控件
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
}

//串口发送启动
void GUI::onStartSendSerial(){
    qDebug()<<"onStartSendSerial已触发!";

    // 首先禁止其他按钮状态
    //不允许在点击发送之后还可以读取excel！
    m_disconnectButton->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_openExcelButton->setEnabled(false);
    m_sendExcelButton->setEnabled(false);
    m_stopSendExcelButton->setEnabled(false);
    m_openSerialButton->setEnabled(false);
    m_closeSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(false);
    m_stopSendSerialButton->setEnabled(true);
    // 禁止重复点击发送
    m_sendLimitLineEdit->setEnabled(false);
    m_delayLineEdit->setEnabled(false);

    // 创建新线程和工作对象
    m_excelSendWorker = new ExcelSendWorker();
    //传递要发送的数据到excel发送work
    m_excelSendWorker->m_table = m_excelTableWidget;                                    //传递发送的表格table
    m_excelSendWorker->m_totalRows = m_excelReader->totalRows;                     //传递表格的行数
    m_excelSendWorker->m_delayMs = m_delayLineEdit->text().toInt();                   //传递延时时间
    m_excelSendWorker->m_repeatLimit = m_sendLimitLineEdit->text().toInt();    //传递发送次数限制

    //移动到子线程当中
    m_excelSendWorker->moveToThread(m_excelSendThread);

    //链接信号与槽
    connect(this, &GUI::requestStartSend, m_excelSendWorker, &ExcelSendWorker::serialStartWork);
    // 关键：使用直接连接，让 stopWork 在 GUI 线程被调用
    //    Qt 有几种连接方式（Qt::ConnectionType）：
    //    Qt::AutoConnection（默认）：若发送者与接收者在同一线程，则为直接连接（Qt::DirectConnection）；若在不同线程，则为队列连接（Qt::QueuedConnection）。
    //    Qt::DirectConnection：发送信号时，立即在当前线程调用槽函数，就像直接调用普通函数一样。
    //    Qt::QueuedConnection：将槽函数调用封装为事件，投递到接收者所在线程的事件队列中，等待该线程的事件循环处理。
    //因为发送任务是死循环的，如果此时停止信号使用队列链接，那么将永远也不会执行到，那么会造成主线程阻塞等待停止且一直不会停止
    connect(this, &GUI::requestStopSend, m_excelSendWorker, &ExcelSendWorker::serialStopWork, Qt::DirectConnection);
    connect(m_excelSendWorker, &ExcelSendWorker::sendSerialCommand, m_serialWorker, &SerialWorker::writeData);
    connect(m_excelSendWorker, &ExcelSendWorker::serialFinished, this, &GUI::onSerialSendFinished);
    connect(m_excelSendWorker, &ExcelSendWorker::serialSentCountChanged, this, &GUI::onUpdateSentCount);

    //子线程启动
    m_excelSendThread->start();

    //发送信号，启动发送
    emit requestStartSend();

}

void GUI::onStopSendSerial(){
    qDebug()<<"onStopSendSerial已触发!";
    //点击停止按钮后，需要发送停止信号，停止发送
    if (m_excelSendWorker) {
        emit requestStopSend();                // 仅发送停止信号，不清理
    }
    m_stopSendSerialButton->setEnabled(false);
}

void GUI::onSerialSendFinished(){
    qDebug()<<"onSerialSendFinished已触发!";
    //收到停止信号后刷新
    if (m_excelSendWorker) {
        m_excelSendWorker->deleteLater();
        m_excelSendWorker = nullptr;
    }
    if (m_excelSendThread && m_excelSendThread->isRunning()) {
        m_excelSendThread->quit();
        m_excelSendThread->wait();
    }
    //随后需要将按钮状态重置
    // 恢复界面控件
    m_sendLimitLineEdit->setEnabled(true);
    m_delayLineEdit->setEnabled(true);
    m_stopSendSerialButton->setEnabled(false);
    m_sendSerialButton->setEnabled(true);
    m_openExcelButton->setEnabled(true);
    m_closeSerialButton->setEnabled(true);
}

GUI::~GUI()
{
}
