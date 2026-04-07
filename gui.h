#ifndef GUI_H
#define GUI_H

#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>

#include "readexceldata.h"

class GUI : public QDialog
{
    Q_OBJECT

public:
    GUI(QWidget *parent = 0,Qt::WindowFlags f =0);
    ~GUI();
private:
    QGridLayout *mainLayout;
    QTableWidget *readExcelTable;
    QPushButton *openExcelButton;
    readExcelData *Exceldata;//被读取的excel对象
    QFileDialog fdialog; //读取的excel地址
    QLabel *excelReadLabel;
    //发送、接受命令显示窗口
    QListWidget *contentListWidge;
    QLabel *contentLabel;
    //LAN通讯设置
    QLabel *serverIPLabel;
    QLineEdit *serverIPLineEdit;
    QLabel *portLabel;
    QLineEdit *portLineEdit;
    QLabel *delayLabel;
    QLineEdit *delayLineEdit;
    //按钮
    QPushButton *enterButton;       //发送excel命令按钮
    QPushButton *stopButton;        //停止链接按钮
    QPushButton *contentButton;     //链接到电源按钮
    QPushButton *stopEnterButton;   //  关闭发送excel命令按钮
private slots:
    void openExcelClickedSlot();        //打开excel信号量
    void contentServerSlot();           //链接到电源信号量
    void stopServerSlot();              //关闭链接电源信号量
    void enterExcelClickedSlot();       //发送excel表格当中命令信号量
    void stopEnterExcelClickedSlot();       //关闭发送excel命令信号量
};

#endif // GUI_H
