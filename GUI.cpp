#include "gui.h"

GUI::GUI(QWidget *parent,Qt::WindowFlags f): QDialog(parent,f)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    //创建线程对象
    NetworkThread = new QThread;

    setWindowTitle(tr("Ainuo通用通讯可靠性测试软件"));
    resize(1000,600);
    mainLayout = new QGridLayout(this);

    excelReadLabel = new QLabel(tr("读取到的Excel表格的数据"));
    contentLabel = new QLabel(tr("接受的数据"));
    sendLabel = new QLabel(tr("发送的数据"));
    serverIPLabel = new QLabel(tr("电源的网络地址:"));
    portLabel = new QLabel(tr("端口号:"));
    delayLabel = new QLabel(tr("每条命令发送延时(ms)："));

    serverIPLineEdit = new QLineEdit;
    portLineEdit = new QLineEdit;
    delayLineEdit = new QLineEdit;

    serverIPLineEdit->setText("127.0.0.1");
    portLineEdit->setText("20108");
    delayLineEdit->setText("50");

    contentButton = new QPushButton(tr("连接到电源"));
    stopButton = new QPushButton(tr("断开链接"));
    openExcelButton = new QPushButton(tr("打开excel并读取"));
    enterButton = new QPushButton(tr("发送excel命令到电源"));
    stopEnterButton = new QPushButton(tr("停止发送excel命令到电源"));

    stopButton->setEnabled(false);
    contentButton->setEnabled(true);
    openExcelButton->setEnabled(true);
    enterButton->setEnabled(false);
    stopEnterButton->setEnabled(false);

    connect(openExcelButton,SIGNAL(clicked()),this,SLOT(openExcelClickedSlot()));//信号量链接
    connect(contentButton,SIGNAL(clicked()),this,SLOT(contentServerSlot()));//信号量链接
    connect(enterButton,SIGNAL(clicked()),this,SLOT(enterExcelClickedSlot()));//信号量链接
    connect(stopButton,SIGNAL(clicked()),this,SLOT(stopServerSlot()));//信号量链接
    connect(stopEnterButton,SIGNAL(clicked()),this,SLOT(stopEnterExcelClickedSlot()));//信号量链接

    contentListWidge = new QListWidget;//接受命令显示窗口
    sendListWidge = new QListWidget;//发送命令显示窗口
    readExcelTable = new QTableWidget;//读取excel数据窗口

    Exceldata = new readExcelData;

    readExcelTable->setColumnCount(2);//初始化为2列
    readExcelTable->setColumnWidth(1,300);//设置第二列的列宽为300像素
    readExcelTable->setHorizontalHeaderLabels(QStringList()<<"需发送的命令"<<"正确的返回值");
    readExcelTable->setRowCount(10);    //初始化为10行
    readExcelTable->setItem(0,0,new QTableWidgetItem("等待读取excel表格"));

    mainLayout->addWidget(excelReadLabel,0,0,1,2);
    mainLayout->addWidget(contentLabel,0,2,1,2);
    mainLayout->addWidget(sendLabel,0,4,1,1);

    mainLayout->addWidget(readExcelTable,1,0,1,2);
    mainLayout->addWidget(contentListWidge,1,2,1,2);
    mainLayout->addWidget(sendListWidge,1,4,1,1);

    mainLayout->addWidget(openExcelButton,2,0,1,2);
    mainLayout->addWidget(serverIPLabel,2,2,1,1);
    mainLayout->addWidget(serverIPLineEdit,2,3,1,1);

    mainLayout->addWidget(portLabel,3,2,1,1);
    mainLayout->addWidget(portLineEdit,3,3,1,1);
    mainLayout->addWidget(delayLabel,3,0,1,1);
    mainLayout->addWidget(delayLineEdit,3,1,1,1);

    mainLayout->addWidget(enterButton,4,0,1,1);
    mainLayout->addWidget(stopEnterButton,4,1,1,1);
    mainLayout->addWidget(contentButton,4,3,1,1);
    mainLayout->addWidget(stopButton,4,2,1,1);
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
            }
        }
    }
    enterButton->setEnabled(true);
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
    connect(Network, &connectNetwork::DisplaSendData, this, &GUI::GUIDisplaSendData);
    connect(Network, &connectNetwork::DisplaConnectData, this, &GUI::GUIDisplaConnectData);

    //传递信号，启动Network
    emit StartConnectNetwork(portLineEdit->text().toInt(),QHostAddress(serverIPLineEdit->text()));

    //子线程启动
    NetworkThread->start();

    stopButton->setEnabled(true);
    contentButton->setEnabled(false);
    portLineEdit->setEnabled(false);
    serverIPLineEdit->setEnabled(false);
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
}

//GUI显示发送的数据
void GUI::GUIDisplaSendData(QString msg){
    sendListWidge->addItem(msg);
}

//GUI显示收到的数据
void GUI::GUIDisplaConnectData(QString msg){
    contentListWidge->addItem(msg);
}

//非阻塞式发送线程
void GUI::enterExcelClickedSlot(){
    qDebug()<<"debug:enterExcelClickedSlot已经触发";
    //发送数据通过信号量传递
    for(int i =0;i<Exceldata->ExceltotalRows;i++){
        emit NetworkSendDataSignals(readExcelTable->item(i,0)->text(),delayLineEdit->text().toInt());
    }
    stopEnterButton->setEnabled(true);
    enterButton->setEnabled(false);
}

//停止Excel发送
void GUI::stopEnterExcelClickedSlot(){
    QMessageBox::information(this,"debug","debug:stopEnterExcelClickedSlot已经触发");


    stopEnterButton->setEnabled(false);
    enterButton->setEnabled(true);
}


GUI::~GUI()
{

}
