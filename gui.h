#ifndef GUI_H
#define GUI_H

#include <QHostAddress>
#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QThread>

#include "readexceldata.h"
#include "connectnetwork.h"
#include "excelsendworker.h"

class GUI : public QDialog
{
    Q_OBJECT

public:
    GUI(QWidget *parent = 0,Qt::WindowFlags f =0);
    ~GUI();
private:
    //创建子线程
    QThread *NetworkThread;             //网络发送子线程
    QThread *excelSendThread;           //excel表格子线程
    //主界面
    QGridLayout *mainLayout;            //主界面
    QTableWidget *readExcelTable;       //读取excel的table
    readExcelData *Exceldata;           //被读取的excel对象
    ExcelSendWorker *ExcelSendwork;     //excel发送数据对象

    int MAXDisplayItems = 300;          //限制最大显示条数为300

    QFileDialog fdialog;                //读取的excel地址
    QLabel *excelReadLabel;             //读取excel提示Label
    connectNetwork *Network;            //Network对象
    //发送、接受命令显示窗口
    QListWidget *contentListWidge;  //接受命令窗口
    QLabel *contentLabel;           //接受命令提示Label
    QListWidget *sendListWidge;     //发送命令窗口
    QLabel *sendLabel;              //发送命令提示Label
    //LAN通讯设置
    QLabel *serverIPLabel;          //电源IP提示Label
    QLineEdit *serverIPLineEdit;    //电源IP输入框
    QLabel *portLabel;              //电源端口提示Label
    QLineEdit *portLineEdit;        //电源端口输入框
    //按钮
    QPushButton *openExcelButton;   //打开excel按钮
    QPushButton *enterButton;       //发送excel命令按钮
    QPushButton *stopEnterButton;   //关闭发送excel命令按钮
    QPushButton *stopButton;        //停止链接按钮
    QPushButton *contentButton;     //链接到电源按钮
    //数据显示框，例如执行了多少次这样的
    QLabel *delayLabel;                         //命令发送延时提示Label
    QLineEdit *delayLineEdit;                   //命令发送延时输入框
    QLabel *limitSendDataNumLabel;              //限制发送条数（0一直跑），用于确保数据发送到一定程度自动停止
    QLineEdit *limitSendDataNumLineEdit;        //限制的发送次数输入框
    QLabel *SendDataNumStatisLabel;             //统计的发送命令提示Label
    QLabel *DisplaySendDataNumStatisLabel;      //统计的发送命令总数显示Label

signals:
    void StartConnectNetwork(int port,QHostAddress serverIP);                   //通知子线程网络链接启动了
    void NetworkSendDataSignals(QString msg,int delayMS = 0);                   //通知子线程发送数据
    void StartSendData();                                                       //通知子线程准备发送了
    void StopSendData();                                                        //通知子线程停止发送了
private slots:
    void openExcelClickedSlot();                                                //打开excel信号量
    void contentServerSlot();                                                   //链接到电源信号量
    void stopServerSlot();                                                      //关闭链接电源信号量
    void enterExcelClickedSlot();                                               //发送excel表格当中命令信号量
    void stopEnterExcelClickedSlot();                                           //关闭发送excel命令信号量（手动，只负责发送信号）
    void onEnterExcelFinished();                                                //负责清理excel发送完成后的变量
    void GUIDisplayConnectData(QString msg);                                    //接收到的数据显示信号量
    void GUIDisplaySendData(QString msg);                                       //发送过去的数据显示信号量
    void GUIDisplaySendDataNum(int Num);                                        //显示发送了多少条命令
};

#endif // GUI_H
