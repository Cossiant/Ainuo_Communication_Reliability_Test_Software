#include "gui.h"

GUI::GUI(QWidget *parent,Qt::WindowFlags f): QDialog(parent,f)
{
    setWindowTitle(tr("Ainuo通用通讯可靠性测试软件"));
    resize(800,600);
    mainLayout = new QGridLayout(this);

    excelReadLabel = new QLabel(tr("读取到的Excel表格的数据"));
    contentLabel = new QLabel(tr("通过LAN发送、接受的数据"));
    serverIPLabel = new QLabel(tr("电源的网络地址:"));
    portLabel = new QLabel(tr("端口号:"));
    delayLabel = new QLabel(tr("每条命令发送延时(s)："));

    serverIPLineEdit = new QLineEdit;
    portLineEdit = new QLineEdit;
    delayLineEdit = new QLineEdit;

    serverIPLineEdit->setText("127.0.0.1");
    portLineEdit->setText("20108");

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

    contentListWidge = new QListWidget;//发送、接受命令显示窗口
    readExcelTable = new QTableWidget;//读取excel数据窗口

    readExcelTable->setColumnCount(2);//初始化为2列
    readExcelTable->setColumnWidth(1,300);//设置第二列的列宽为300像素
    readExcelTable->setHorizontalHeaderLabels(QStringList()<<"编号"<<"需发送的命令");
    readExcelTable->setRowCount(10);    //初始化为10行
    readExcelTable->setItem(0,0,new QTableWidgetItem("等待读取excel表格"));

    mainLayout->addWidget(excelReadLabel,0,0,1,2);
    mainLayout->addWidget(contentLabel,0,2,1,1);

    mainLayout->addWidget(readExcelTable,1,0,1,2);
    mainLayout->addWidget(contentListWidge,1,2,1,2);

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
            if (readExcelData::loadExcelToTable(fname, readExcelTable, this)) {
                QMessageBox::information(this, tr("提示"), tr("Excel 文件读取完成！"));
            }
        }
    }
    enterButton->setEnabled(true);
}

//链接服务器
void GUI::contentServerSlot(){
    qDebug()<< "debug:contentServerSlot已经触发";
    Network = new connectNetwork(portLineEdit->text().toInt(),QHostAddress(serverIPLineEdit->text()));
    Network->ListWidge = contentListWidge;

    Network->NetworkSendData("Hello world!");
    stopButton->setEnabled(true);
    contentButton->setEnabled(false);
    portLineEdit->setEnabled(false);
    serverIPLineEdit->setEnabled(false);
}

void GUI::stopServerSlot(){
    qDebug()<< "debug:stopServerSlot已经触发";
    if(Network){
        delete Network;
        Network = nullptr;
        qDebug() << "清理Network完成！";
    }
    stopButton->setEnabled(false);
    contentButton->setEnabled(true);
    portLineEdit->setEnabled(true);
    serverIPLineEdit->setEnabled(true);
}

void GUI::enterExcelClickedSlot(){
    QMessageBox::information(this,"debug","debug:enterExcelClickedSlot已经触发");
    stopEnterButton->setEnabled(true);
    enterButton->setEnabled(false);
}

void GUI::stopEnterExcelClickedSlot(){
    QMessageBox::information(this,"debug","debug:stopEnterExcelClickedSlot已经触发");
    stopEnterButton->setEnabled(false);
    enterButton->setEnabled(true);
}

GUI::~GUI()
{

}
