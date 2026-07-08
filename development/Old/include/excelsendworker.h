#ifndef EXCELSENDWORKER_H
#define EXCELSENDWORKER_H

#include <QObject>
#include <QTableWidget>
#include <QThread>
#include <atomic>
#include <QTimer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QByteArray>
#include <QElapsedTimer>
#include <QDebug>

class ExcelSendWorker : public QObject
{
    Q_OBJECT
public:
    explicit ExcelSendWorker(QObject *parent = nullptr);

    QTableWidget *m_table;

    std::atomic<bool> m_stopFlag;            //手动停止位
    std::atomic<bool> m_timeoutFlag;         //外部线程控制的超时停止位

    int m_delayMs;          // 命令之间的延时时间（毫秒）（最小延时）
    int m_repeatLimit;      // 总发送条数限制，0 表示无限循环
    int m_totalRows;        // 发送表格的行数，用于限制其不会跑飞
    int m_sentCount;        // 总计发送了多少条，用于统计
    int m_timeoutMs;        // 超时时间（从 GUI 传入）

    bool m_useHexSend;      // 是否使用十六进制发送

    QElapsedTimer timer;    //精准定时器的timer变量
public slots:
    void startNetworkWork();           // 原网络发送启动槽
    void stopNetworkWork();            // 原网络发送停止槽
    void serialStartWork();     // 串口发送启动槽
    void serialStopWork();      // 串口发送停止槽
    void onExternalResponseReceived(const QByteArray &data);   // 收到应答时置超时标志

    signals:
        void sendCommand(QByteArray data);       // 网络发送命令（字节流）
    void finished();
    void sentCountChanged(int count);

    void sendSerialCommand(QByteArray data); // 串口发送命令（字节流）
    void serialFinished();
    void serialSentCountChanged(int count);

    void commandSent(int row, QByteArray expectedResponse);   // 发送命令时通知期望的返回值
    void sendTimeOut();                                 //超时信号发送

private:

    //高精度延时
    void interruptibleWait(int msec);
    //超时延时
    void timeoutWait(int msec);

    void integrationWait(int delay,int timeout);
};

#endif // EXCELSENDWORKER_H
