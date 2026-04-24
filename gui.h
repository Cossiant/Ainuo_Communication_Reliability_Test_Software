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
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QCheckBox>
#include <QQueue>

#include "readexceldata.h"
#include "connectnetwork.h"
#include "excelsendworker.h"
#include "serialworker.h"
#include "saveworker.h"
#include "led.h"

class GUI : public QDialog
{
    Q_OBJECT

public:
    GUI(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~GUI();
private:
    //创建子线程
    QThread *m_networkThread;           //网络发送子线程
    QThread *m_excelSendThread;         //excel表格子线程
    QThread *m_serialThread;            //串口工作线程
    QThread *m_savedataThread;
    //主界面
    QGridLayout *m_mainLayout;          //主界面
    QTableWidget *m_excelTableWidget;   //读取excel的table
    ExcelReader *m_excelReader;         //被读取的excel对象
    ExcelSendWorker *m_excelSendWorker; //excel发送数据对象

    QQueue<QByteArray> m_expectedResponseQueue;   // 期望返回值队列
    int m_errorCount = 0;                         // 错误计数
    int m_maxDisplayItems = 300;        //限制最大显示条数为300

    enum class ConnectionType {
        None,
        Network,
        Serial
    };
    ConnectionType m_currentConnectionType = ConnectionType::None;
    //定义使用的链接的类型，分为空、网络、串口

    QFileDialog m_fileDialog;           //读取的excel地址
    QLabel *m_excelReadLabel;           //读取excel提示Label
    NetworkClient *m_networkClient;     //Network对象
    SerialWorker *m_serialWorker;       //串口工作对象
    saveworker *m_savedataWorker;       //自动保存到文件工作对象
    //LED对象及其显示窗口
    QLabel *m_NetWorkLED;               //网口状态显示LED
    QLabel *m_SerialLED;                //串口状态显示LED
    QLabel *m_NetWorkLEDLabel;          //网口状态提示Label
    QLabel *m_SerialLEDLabel;           //串口状态提示Label
    //发送、接受命令显示窗口
    QListWidget *m_receiveListWidget;   //接受命令窗口
    QLabel *m_receiveLabel;             //接受命令提示Label
    QListWidget *m_sendListWidget;      //发送命令窗口
    QLabel *m_sendLabel;                //发送命令提示Label
    //LAN通讯设置
    QLabel *m_serverIpLabel;            //电源IP提示Label
    QLineEdit *m_serverIpLineEdit;      //电源IP输入框
    QLabel *m_portLabel;                //电源端口提示Label
    QLineEdit *m_portLineEdit;          //电源端口输入框
    //excel按钮
    QPushButton *m_openExcelButton;     //打开excel按钮
    QPushButton *m_sendExcelButton;     //发送excel命令按钮
    QPushButton *m_stopSendExcelButton; //关闭发送excel命令按钮
    QPushButton *m_sendSerialButton;    //通过串口发送按钮
    QPushButton *m_stopSendSerialButton;//关闭串口发送按钮
    //网络连接按钮
    QPushButton *m_disconnectButton;    //停止链接按钮
    QPushButton *m_connectButton;       //链接到电源按钮
    //串口按钮
    QPushButton *m_openSerialButton;    //打开串口按钮
    QPushButton *m_closeSerialButton;   //关闭串口按钮
    //其他按钮
    QPushButton *m_GUIClearButton;       //清空发送与接受按钮
    //选择是否AN3.0发送勾选框和是否只保存错误数据勾选框
    QCheckBox *m_sendWithAN3CheckBox;
    QCheckBox *m_tcpNoDelayCheckBox;
    static QString toHexDisplay(const QByteArray &data);    //当使用AN3.0的时候，显示函数用这个
    //数据显示框，例如执行了多少次这样的
    QLabel *m_delayLabel;               //命令发送延时提示Label
    QLineEdit *m_delayLineEdit;         //命令发送延时输入框
    QLabel *m_sendLimitLabel;           //限制发送条数（0一直跑），用于确保数据发送到一定程度自动停止
    QLineEdit *m_sendLimitLineEdit;     //限制的发送次数输入框
    QLabel *m_sentCountLabel;           //统计的发送命令提示Label
    QLabel *m_sentCountDisplayLabel;    //统计的发送命令总数显示Label
    QLabel *m_errorCountLabel;          //统计的返回错误数量提示Label
    QLabel *m_errorCountDisplayLabel;   //统计的返回错误数量显示
    //使用串口进行通讯
    QLabel *m_serialPortLabel;          //串口Label
    QComboBox *m_serialPortComboBox;    //串口端口号选择
    QLabel *m_baudRateLabel;            //波特率label
    QComboBox *m_baudRateComboBox;      //波特率
    QLabel *m_dataBitsLabel;            //数据位Label
    QComboBox *m_dataBitsComboBox;      //数据位
    QLabel *m_stopBitsLabel;            //停止位label
    QComboBox *m_stopBitsComboBox;      //停止位
    QLabel *m_parityLabel;              //校验位Label
    QComboBox *m_parityComboBox;        //校验位

signals:
    void requestNetworkConnect(int port, QHostAddress serverIP);                //通知子线程网络链接启动了
    void requestSendNetworkData(QByteArray msg, int delayMS = 0);               //通知子线程发送数据
    void requestStartSend();                                                    //通知子线程准备发送了
    void requestStopSend();                                                     //通知子线程停止发送了

    void requestSerialOpen(const QString &portName,
                           qint32 baudRate,
                           QSerialPort::DataBits dataBits,
                           QSerialPort::Parity parity,
                           QSerialPort::StopBits stopBits,
                           QSerialPort::FlowControl flowControl);               //通知串口可以创建了
    void requestSerialClose();

    void requestInitSaveFile(const QString &baseDir);                           //通知保存线程可以工作了
    void requestWriteSaveFile(const QString &text);                             //通知保存线程写入XX内容
    void requestCloseSaveFile();                                                //通知保存线程关闭

private slots:
    void onOpenExcelClicked();                                                  //打开excel信号量
    void onConnectServer();                                                     //网络链接到电源信号量
    void onDisconnectServer();                                                  //关闭网络链接电源信号量
    void successConnectServer();                                                //链接网络成功信号量，用来打开按钮

    void onStartSendExcel();                                                    //发送excel表格当中命令信号量（网络）
    void onStopSendExcel();                                                     //关闭发送excel命令信号量（手动，只负责发送信号）（网络）
    void onSendFinished();                                                      //负责清理excel发送完成后的变量（网络）

    void onStartSendSerial();                                                   //发送excel表格当中命令信号量（串口）
    void onStopSendSerial();                                                    //关闭发送excel命令信号量（串口）
    void onSerialSendFinished();                                                //负责清理excel发送完成后的变量（串口）

    void onCommandSent(int row, QByteArray expectedResponse);                   //命令发送之后，传递该命令对应的期望返回值，用来做判断错误

    void onOpenSerial();                                                        //打开串口槽
    void onCloseSerial();                                                       //点击关闭信号槽
    void onSerialClosed();                                                      //关闭串口释放资源

    void onDisplayReceivedData(QByteArray msg);                                 //接收到的数据显示信号量
    void onDisplaySentData(QByteArray msg);                                     //发送过去的数据显示信号量
    void onUpdateSentCount(int count);                                          //显示发送了多少条命令

    void clearGUI();                                                            //清空发送与接受区域的数据
};

#endif // GUI_H
