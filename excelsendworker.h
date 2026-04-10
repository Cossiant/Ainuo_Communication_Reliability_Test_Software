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

    int m_delayMs;      // 命令之间的延时时间
    int m_repeatLimit;  // 总发送条数限制，0 表示无限循环
    int m_totalRows;    // 发送表格的行数，用于限制其不会跑飞
    int m_sendDataNum = 0;  // 总计发送了多少条，用于统计

public slots:
    void startWork();   // 开始发送
    void stopWork();    // 停止发送

signals:
    void sendCommand(QString cmd, int delayMs);     // 请求发送一条命令
    void finished();                                // 发送完成（自然结束或被停止）
    void sendDataNumSignals(int Num);                             // 当前发送了多少次，发送给主线程显示

private:
    //定义原子bool变量 m_stopFlag 保证不会发生半个字节被修改的中间状态
    std::atomic<bool> m_stopFlag;
};

#endif // EXCELSENDWORKER_H
