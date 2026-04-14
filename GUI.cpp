#include "gui.h"

GUI::GUI(QWidget *parent,Qt::WindowFlags f): QDialog(parent,f)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    //创建线程对象
    NetworkThread = new QThread;
    excelSendThread = new QThread;
    //设置主窗口
    setWindowTitle(tr("Ainuo通用通讯可靠性测试软件"));
    resize(1000,600);
    mainLayout = new QGridLayout(this);

    //初始化label
    excelReadLabel = new QLabel(tr("读取到的Excel表格的数据"));
    contentLabel = new QLabel(tr("接受的数据"));
    sendLabel = new QLabel(tr("发送的数据"));
    serverIPLabel = new QLabel(tr("电源的网络地址:"));
    portLabel = new QLabel(tr("端口号:"));
    delayLabel = new QLabel(tr("每条命令发送延时(ms)："));
    limitSendDataNumLabel = new QLabel(tr("发送条数(0为一直循环):"));
    SendDataNumStatisLabel = new QLabel(tr("总计发送命令："));
    DisplaySendDataNumStatisLabel = new QLabel(tr("0"));
    SerialPortLabel = new QLabel(tr("串口端口号:"));
    SerialbaudRateLabel = new QLabel(tr("串口波特率:"));
    SerialdataBitsLabel = new QLabel(tr("串口数据位:"));
    SerialstopBitsLabel = new QLabel(tr("串口停止位:"));
    SerialparityLabel = new QLabel(tr("串口校验位:"));
    SendDataErrorNumLabel = new QLabel(tr("返回错误次数:"));
    DisplaySendDataErrorNumLabel = new QLabel(tr("0"));

    //初始化下拉菜单
    SerialPortComboBox = new QComboBox(this);
    SerialbaudRateComboBox = new QComboBox(this);
    SerialdataBitsComboBox = new QComboBox(this);
    SerialstopBitsComboBox = new QComboBox(this);
    SerialparityComboBox = new QComboBox(this);

    //这里获取当前串口端口号
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        SerialPortComboBox->addItem(tr("无可用串口"));
    } else {
        for (const QSerialPortInfo &info : portList) {
            //将已有的端口号显示出来
            SerialPortComboBox->addItem(info.portName());
        }
    }
    //串口列表内容
    QStringList baudRates = {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"};
    QStringList dataBits = {"5", "6", "7", "8"};
    QStringList stopBits = {"1", "1.5", "2"};
    QStringList parities = {"None", "Even", "Odd", "Space", "Mark"};
    //将列表内容添加到Combobox当中
    SerialbaudRateComboBox->addItems(baudRates);
    SerialdataBitsComboBox->addItems(dataBits);
    SerialstopBitsComboBox->addItems(stopBits);
    SerialparityComboBox->addItems(parities);
    //初始化串口参数
    SerialbaudRateComboBox->setCurrentText("115200"); // 默认值
    SerialdataBitsComboBox->setCurrentText("8");
    SerialstopBitsComboBox->setCurrentText("1");
    SerialparityComboBox->setCurrentText("None");

    //初始化LineEdit
    serverIPLineEdit = new QLineEdit;
    portLineEdit = new QLineEdit;
    delayLineEdit = new QLineEdit;
    limitSendDataNumLineEdit = new QLineEdit;

    //设置参数
    serverIPLineEdit->setText("127.0.0.1");
    portLineEdit->setText("20108");
    delayLineEdit->setText("50");
    limitSendDataNumLineEdit->setText("0");

    //初始化按钮
    contentButton = new QPushButton(tr("连接到电源"));
    stopButton = new QPushButton(tr("断开链接"));
    openExcelButton = new QPushButton(tr("打开excel并读取"));
    enterButton = new QPushButton(tr("发送excel命令到电源"));
    stopEnterButton = new QPushButton(tr("停止发送excel命令到电源"));
    openSerialButton = new QPushButton(tr("打开串口"));
    closeSerialButton = new QPushButton(tr("关闭串口"));
    openEnterSerialButton = new QPushButton(tr("通过串口发送excel命令"));
    stopEnterSerialButton = new QPushButton(tr("停止串口发送excel命令"));

    stopButton->setEnabled(false);
    contentButton->setEnabled(true);
    portLineEdit->setEnabled(true);
    serverIPLineEdit->setEnabled(true);
    openExcelButton->setEnabled(false);
    enterButton->setEnabled(false);
    stopEnterButton->setEnabled(false);
    openSerialButton->setEnabled(true);
    closeSerialButton->setEnabled(false);

    connect(openExcelButton,SIGNAL(clicked()),this,SLOT(openExcelClickedSlot()));//信号量链接
    connect(contentButton,SIGNAL(clicked()),this,SLOT(contentServerSlot()));//信号量链接
    connect(enterButton,SIGNAL(clicked()),this,SLOT(enterExcelClickedSlot()));//信号量链接
    connect(stopButton,SIGNAL(clicked()),this,SLOT(stopServerSlot()));//信号量链接
    connect(stopEnterButton,SIGNAL(clicked()),this,SLOT(stopEnterExcelClickedSlot()));//信号量链接

    contentListWidge = new QListWidget;                             //接受命令显示窗口
    sendListWidge = new QListWidget;                                //发送命令显示窗口
    readExcelTable = new QTableWidget;                              //读取excel数据窗口

    Exceldata = new readExcelData;

    readExcelTable->setColumnCount(2);                              //初始化为2列
    readExcelTable->setColumnWidth(1,300);                          //设置第二列的列宽为300像素
    readExcelTable->setHorizontalHeaderLabels(QStringList()<<"需发送的命令"<<"正确的返回值");
    readExcelTable->setRowCount(10);                                //初始化为10行
    readExcelTable->setItem(0,0,new QTableWidgetItem("等待读取excel表格"));

    mainLayout->addWidget(excelReadLabel,0,0,1,2);
    mainLayout->addWidget(contentLabel,0,2,1,2);
    mainLayout->addWidget(sendLabel,0,4,1,2);

    mainLayout->addWidget(readExcelTable,1,0,1,2);
    mainLayout->addWidget(contentListWidge,1,2,1,2);
    mainLayout->addWidget(sendListWidge,1,4,1,2);

    mainLayout->addWidget(openExcelButton,2,0,1,2);
    mainLayout->addWidget(serverIPLabel,2,2,1,1);
    mainLayout->addWidget(serverIPLineEdit,2,3,1,1);
    mainLayout->addWidget(SerialPortLabel,2,4,1,1);
    mainLayout->addWidget(SerialPortComboBox,2,5,1,1);

    mainLayout->addWidget(portLabel,3,2,1,1);
    mainLayout->addWidget(portLineEdit,3,3,1,1);
    mainLayout->addWidget(limitSendDataNumLabel,3,0,1,1);
    mainLayout->addWidget(limitSendDataNumLineEdit,3,1,1,1);
    mainLayout->addWidget(SerialbaudRateLabel,3,4,1,1);
    mainLayout->addWidget(SerialbaudRateComboBox,3,5,1,1);

    mainLayout->addWidget(delayLabel,4,0,1,1);
    mainLayout->addWidget(delayLineEdit,4,1,1,1);
    mainLayout->addWidget(SendDataErrorNumLabel,4,2,1,1);
    mainLayout->addWidget(DisplaySendDataErrorNumLabel,4,3,1,1);
    mainLayout->addWidget(SerialdataBitsLabel,4,4,1,1);
    mainLayout->addWidget(SerialdataBitsComboBox,4,5,1,1);

    mainLayout->addWidget(enterButton,5,0,1,1);
    mainLayout->addWidget(stopEnterButton,5,1,1,1);
    mainLayout->addWidget(SendDataNumStatisLabel,5,2,1,1);
    mainLayout->addWidget(DisplaySendDataNumStatisLabel,5,3,1,1);
    mainLayout->addWidget(SerialparityLabel,5,4,1,1);
    mainLayout->addWidget(SerialparityComboBox,5,5,1,1);

    mainLayout->addWidget(openEnterSerialButton,6,0,1,1);
    mainLayout->addWidget(stopEnterSerialButton,6,1,1,1);
    mainLayout->addWidget(stopButton,6,2,1,1);
    mainLayout->addWidget(contentButton,6,3,1,1);
    mainLayout->addWidget(SerialstopBitsLabel,6,4,1,1);
    mainLayout->addWidget(SerialstopBitsComboBox,6,5,1,1);

    mainLayout->addWidget(openSerialButton,7,4,1,1);
    mainLayout->addWidget(closeSerialButton,7,5,1,1);

    // 设置第 0 列和第 1 列可拉伸，比例为 1:1
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 1);
    // 第 2 列和第 3 列不拉伸或比例较小
    mainLayout->setColumnStretch(2, 0);
    mainLayout->setColumnStretch(3, 0);
}

//读取excel表格
void GUI::openExcelClickedSlot(){

    // 弹出文件选择对话框，获取 Excel 文件路径
    fdialog.setFileMode(QFileDialog::ExistingFile);
    fdialog.setViewMode(QFileDialog::Detail);
    fdialog.setOption(QFileDialog::ReadOnly,true);
    fdialog.setDirectory(QString("C:/"));
    fdialog.setNameFilter(QString("所有文件(*.*);;Microsoft Excel工作表(*.xlsx);;Microsoft Excel 97-2003工作表(*.xls)"));

    if(fdialog.exec()){
        QStringList files = fdialog.selectedFiles();
        for(auto fname:files){
            if (Exceldata->loadExcelToTable(fname, readExcelTable, this)) {
                QMessageBox::information(this, "提示", QStringLiteral("Excel 文件读取完成！\r\n总行数：") + QString::number(Exceldata->ExceltotalRows));
                //只有正确读取才能允许发送！
                enterButton->setEnabled(true);
            }
        }
    }
}

//链接服务器
void GUI::contentServerSlot(){
    qDebug()<< "debug:contentServerSlot已经触发";
    //创建工作对象
    Network = new connectNetwork();
    //告诉Network接受和发送的数据在什么地方
    Network->contentListWidge = contentListWidge;
    Network->sendListWidge = sendListWidge;
    //移动到子线程当中
    Network->moveToThread(NetworkThread);
    //链接信号函数，从GUI到connectNetwork
    connect(this,&GUI::StartConnectNetwork,Network,&connectNetwork::NetworkConnectedSlot);
    connect(this,&GUI::NetworkSendDataSignals,Network,&connectNetwork::NetworkSendData);
    //链接信号函数，从NetWork到GUI
    connect(Network, &connectNetwork::DisplaSendData, this, &GUI::GUIDisplaySendData);
    connect(Network, &connectNetwork::DisplaConnectData, this, &GUI::GUIDisplayConnectData);

    //传递信号，启动Network
    emit StartConnectNetwork(portLineEdit->text().toInt(),QHostAddress(serverIPLineEdit->text()));

    //子线程启动
    NetworkThread->start();

    stopButton->setEnabled(true);
    contentButton->setEnabled(false);
    portLineEdit->setEnabled(false);
    serverIPLineEdit->setEnabled(false);
    openExcelButton->setEnabled(true);
}

//停止链接到电源
void GUI::stopServerSlot(){
    qDebug()<< "debug:stopServerSlot已经触发";
    if(Network){
        // 请求Network断开连接并清理，然后删除自身
        // 可以添加一个信号槽，让Network自己删除
        // 或者调用Network->deleteLater()，并退出线程
        Network->deleteLater();
        Network = nullptr;
    }
    if(NetworkThread && NetworkThread->isRunning()){
        NetworkThread->quit();
        NetworkThread->wait();
    }
    // 重置按钮状态
    stopButton->setEnabled(false);
    contentButton->setEnabled(true);
    portLineEdit->setEnabled(true);
    serverIPLineEdit->setEnabled(true);
    openExcelButton->setEnabled(false);
    enterButton->setEnabled(false);
    stopEnterButton->setEnabled(false);
}

//非阻塞式发送线程
void GUI::enterExcelClickedSlot(){
    qDebug() << "debug:enterExcelClickedSlot 非阻塞线程启动";

    // 禁止重复点击发送
    enterButton->setEnabled(false);
    stopEnterButton->setEnabled(true);
    limitSendDataNumLineEdit->setEnabled(false);
    delayLineEdit->setEnabled(false);
    //不允许在点击发送之后还可以读取excel！
    openExcelButton->setEnabled(false);

    // 创建新线程和工作对象
    ExcelSendwork = new ExcelSendWorker();
    //传递要发送的数据到excel发送work
    ExcelSendwork->m_table = readExcelTable;                                    //传递发送的表格table
    ExcelSendwork->m_totalRows = Exceldata->ExceltotalRows;                     //传递表格的行数
    ExcelSendwork->m_delayMs = delayLineEdit->text().toInt();                   //传递延时时间
    ExcelSendwork->m_repeatLimit = limitSendDataNumLineEdit->text().toInt();    //传递发送次数限制

    //移动到子线程当中
    ExcelSendwork->moveToThread(excelSendThread);

    //链接信号与槽
    connect(this,&GUI::StartSendData,ExcelSendwork,&ExcelSendWorker::startWork);
    // 关键：使用直接连接，让 stopWork 在 GUI 线程被调用
    //    Qt 有几种连接方式（Qt::ConnectionType）：
    //    Qt::AutoConnection（默认）：若发送者与接收者在同一线程，则为直接连接（Qt::DirectConnection）；若在不同线程，则为队列连接（Qt::QueuedConnection）。
    //    Qt::DirectConnection：发送信号时，立即在当前线程调用槽函数，就像直接调用普通函数一样。
    //    Qt::QueuedConnection：将槽函数调用封装为事件，投递到接收者所在线程的事件队列中，等待该线程的事件循环处理。
    //因为发送任务是死循环的，如果此时停止信号使用队列链接，那么将永远也不会执行到，那么会造成主线程阻塞等待停止且一直不会停止
    connect(this, &GUI::StopSendData, ExcelSendwork, &ExcelSendWorker::stopWork, Qt::DirectConnection);
    connect(ExcelSendwork,&ExcelSendWorker::sendCommand,Network,&connectNetwork::NetworkSendData);
    connect(ExcelSendwork, &ExcelSendWorker::finished, this, &GUI::onEnterExcelFinished);
    connect(ExcelSendwork,&ExcelSendWorker::sendDataNumSignals,this,&GUI::GUIDisplaySendDataNum);

    //子线程启动
    excelSendThread->start();

    //发送信号，启动发送
    emit StartSendData();
}

//停止Excel发送
void GUI::stopEnterExcelClickedSlot(){
    qDebug() << "手动停止发送";
    if (ExcelSendwork) {
        emit StopSendData();                // 仅发送停止信号，不清理
    }
    stopEnterButton->setEnabled(false);     // 防止重复点击
}

void GUI::onEnterExcelFinished()
{
    qDebug() << "发送线程结束，开始清理资源";
    if (ExcelSendwork) {
        ExcelSendwork->deleteLater();
        ExcelSendwork = nullptr;
    }
    if (excelSendThread && excelSendThread->isRunning()) {
        excelSendThread->quit();
        excelSendThread->wait();
    }

    // 恢复界面控件
    limitSendDataNumLineEdit->setEnabled(true);
    delayLineEdit->setEnabled(true);
    stopEnterButton->setEnabled(false);
    enterButton->setEnabled(true);
    openExcelButton->setEnabled(true);
}


//GUI显示发送的数据
void GUI::GUIDisplaySendData(QString msg){
    // 添加新条目
    sendListWidge->addItem(msg);

    // 限制最多显示 1000 条
    const int maxItems = MAXDisplayItems;
    while (sendListWidge->count() > maxItems) {
        delete sendListWidge->takeItem(0);  // 删除最旧的一条（索引0）
    }
}

//GUI显示收到的数据
void GUI::GUIDisplayConnectData(QString msg){
    contentListWidge->addItem(msg);

    const int maxItems = MAXDisplayItems;
    while (contentListWidge->count() > maxItems) {
        delete contentListWidge->takeItem(0);
    }
}

//GUI显示统计发送了多少次
void GUI::GUIDisplaySendDataNum(int Num){
    DisplaySendDataNumStatisLabel->setText(QString::number(Num));
}

GUI::~GUI()
{

}
