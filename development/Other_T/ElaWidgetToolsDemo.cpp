#include "ElaWidgetToolsDemo.h"

#include <QCloseEvent>
#include <QHostAddress>          // ★ 修复：qRegisterMetaType<QHostAddress> 需要完整定义
#include <QSerialPort>
#include <QDebug>


// ═══════════════════════════════════════════════════════════════
//  构造函数
// ═══════════════════════════════════════════════════════════════
ElaWidgetToolsDemo::ElaWidgetToolsDemo(QWidget *parent)
    : ElaWindow(parent)
{
    // 注册元类型（供跨线程信号使用）
    qRegisterMetaType<QHostAddress>("QHostAddress");
    qRegisterMetaType<QSerialPort::DataBits>("QSerialPort::DataBits");
    qRegisterMetaType<QSerialPort::Parity>("QSerialPort::Parity");
    qRegisterMetaType<QSerialPort::StopBits>("QSerialPort::StopBits");
    qRegisterMetaType<QSerialPort::FlowControl>("QSerialPort::FlowControl");

    // ═══════════════════════════════════════════════════
    //  创建各通讯模块页面（各自独立管理线程与UI）
    // ═══════════════════════════════════════════════════
    m_mainPage    = new MainPage(this, this);
    m_serialPage  = new SerialPage(this, this);
    m_networkPage = new NetworkPage(this, this);
    m_CANPage     = new CANPage(this, this);
    m_GPIBPage    = new GPIBPage(this);
    m_USERPage    = new USERPage(this, this);
}

ElaWidgetToolsDemo::~ElaWidgetToolsDemo()
{
    // 各 Page 在其析构函数中独立处理线程退出
    // Qt 父子关系自动保证析构顺序
    qDebug() << "ElaWidgetToolsDemo: 窗口已销毁";
}

// ═══════════════════════════════════════════════════════════════
//  窗口关闭事件
// ═══════════════════════════════════════════════════════════════
void ElaWidgetToolsDemo::closeEvent(QCloseEvent* event)
{
    qDebug() << "窗口正在关闭...";

    // 各 Page 的析构函数会自动处理各自线程的退出
    // 无需在此手动清理

    event->accept();
}
