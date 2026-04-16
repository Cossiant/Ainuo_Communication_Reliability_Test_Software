// ExcelSendWorker.h
#ifndef EXCELSENDWORKER_H
#define EXCELSENDWORKER_H

#include <QObject>
#include <QTableWidget>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <atomic>

class ExcelSendWorker : public QObject
{
    Q_OBJECT
public:
    ExcelSendWorker();
    QTableWidget *m_table;

    int m_delayMs;          // 命令之间的延时时间
    int m_repeatLimit;      // 总发送条数限制，0 表示无限循环
    int m_totalRows;        // 发送表格的行数，用于限制其不会跑飞
    int m_sentCount = 0;    // 总计发送了多少条，用于统计

public slots:
    void startNetworkWork();           // 原网络发送启动槽
    void stopNetworkWork();            // 原网络发送停止槽
    void serialStartWork();     // 串口发送启动槽
    void serialStopWork();      // 串口发送停止槽

signals:
    void sendCommand(QString cmd, int delayMs);         // 请求发送一条命令（网络）
    void finished();                                    // 发送完成（自然结束或被停止）
    void sentCountChanged(int count);                   // 当前发送了多少次，发送给主线程显示

    void sendSerialCommand(QString cmd, int delayMs);   // 请求发送一条串口命令
    void serialFinished();                              // 串口发送完成
    void serialSentCountChanged(int count);             // 串口发送次数统计

private:
    //定义原子bool变量 m_stopFlag 保证不会发生半个字节被修改的中间状态
    std::atomic<bool> m_stopFlag;
};

#endif // EXCELSENDWORKER_H
