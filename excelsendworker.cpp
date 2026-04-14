// ExcelSendWorker.cpp
#include "ExcelSendWorker.h"
#include <QThread>

ExcelSendWorker::ExcelSendWorker() : m_stopFlag(false)
{
}

void ExcelSendWorker::startNetworkWork()
{
    m_stopFlag = false;
    m_sentCount = 0;
    qDebug() << "网络发送函数已触发";
    // 可中断的忙等待函数，响应精度可达微秒级
    auto interruptibleWait = [this](int ms) {
        if (ms <= 0) return;
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < ms && !m_stopFlag) {
            QThread::yieldCurrentThread();   // 避免 100% 占用核心
        }
    };

    if (m_repeatLimit == 0) {
        // 无限循环模式
        while (!m_stopFlag) {
            for (int i = 0; i < m_totalRows && !m_stopFlag; ++i) {
                //通知TCP发送命令
                emit sendCommand(m_table->item(i, 0)->text(), m_delayMs);
                //延时XXms
                interruptibleWait(m_delayMs);
                //显示发送了多少条命令
                emit sentCountChanged(++m_sentCount);
            }
        }
    } else {
        // 有限次数模式
        while (m_sentCount < m_repeatLimit && !m_stopFlag) {
            for (int i = 0; i < m_totalRows && m_sentCount < m_repeatLimit && !m_stopFlag; ++i) {
                //通知TCP发送命令
                emit sendCommand(m_table->item(i, 0)->text(), m_delayMs);
                //延时XXms
                interruptibleWait(m_delayMs);
                //显示发送了多少条命令
                emit sentCountChanged(++m_sentCount);
            }
        }
    }
    emit finished();  // 通知 GUI 发送结束
}

//注！本函数将在GUI线程中被使用！
void ExcelSendWorker::stopNetworkWork()
{
    qDebug() << "网络发送停止Flag设置完成";
    m_stopFlag = true;
}

void ExcelSendWorker::serialStartWork()
{
    m_stopFlag = false;      // 复用同一个停止标志，或者您可以再定义一个串口专用标志
    m_sentCount = 0;
    qDebug() << "串口发送函数已触发";
    // 可中断的忙等待函数，响应精度可达微秒级
    auto interruptibleWait = [this](int ms) {
        if (ms <= 0) return;
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < ms && !m_stopFlag) {
            QThread::yieldCurrentThread();   // 避免 100% 占用核心
        }
    };

    if (m_repeatLimit == 0) {
        // 无限循环模式
        while (!m_stopFlag) {
            for (int i = 0; i < m_totalRows && !m_stopFlag; ++i) {
                //通知串口发送命令
                emit sendSerialCommand(m_table->item(i, 0)->text(), m_delayMs);
                //延时XXms
                interruptibleWait(m_delayMs);
                //显示发送了多少条命令
                emit serialSentCountChanged(++m_sentCount);
            }
        }
    } else {
        // 有限次数模式
        while (m_sentCount < m_repeatLimit && !m_stopFlag) {
            for (int i = 0; i < m_totalRows && m_sentCount < m_repeatLimit && !m_stopFlag; ++i) {
                //通知串口发送命令
                emit sendSerialCommand(m_table->item(i, 0)->text(), m_delayMs);
                //延时XXms
                interruptibleWait(m_delayMs);
                //显示发送了多少条命令
                emit serialSentCountChanged(++m_sentCount);
            }
        }
    }
    emit serialFinished();  // 通知 GUI 发送结束
}

//注！本函数将在GUI线程中被使用！
void ExcelSendWorker::serialStopWork()
{
    qDebug() << "串口发送停止Flag设置完成";
    m_stopFlag = true;
}
