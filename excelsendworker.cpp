#include "ExcelSendWorker.h"
#include <QThread>

ExcelSendWorker::ExcelSendWorker(QObject *parent)
    : QObject(parent)
{
}

// 将十六进制字符串（如 "AA BB 0F"）转换为原始字节 QByteArray
static QByteArray stringToHexBytes(const QString &input)
{
    QByteArray bytes;
    // 按空白字符分割（空格、制表符、换行等）
    QStringList parts = input.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok;
        quint8 byte = static_cast<quint8>(part.toUInt(&ok, 16));
        if (ok) {
            bytes.append(static_cast<char>(byte));
        } else {
            qWarning() << "Invalid hex string part:" << part;
        }
    }
    return bytes;
}

// 延时函数（可被打断）
//void ExcelSendWorker::interruptibleWait(int msec)
//{
//    QTimer timer;
//    timer.setTimerType(Qt::PreciseTimer);
//    timer.start(msec);
//    while (timer.remainingTime() > 0) {
//        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//    }
//}

//阻塞式延时
//void ExcelSendWorker::interruptibleWait(int msec) {
//    timer.start();
//    qint64 targetNs = static_cast<qint64>(msec) * 1000000LL;
//    while (!m_stopFlag && timer.nsecsElapsed() < targetNs) {
//    }
//}

//高精度延时
//void ExcelSendWorker::interruptibleWait(int msec)
//{
//    if (msec <= 0) return;
//    timer.start();
//    //他的结束条件是m_stopFlag位
//    while (!m_stopFlag && timer.elapsed() < msec) {
//        //        QThread::msleep(1);   // 睡眠1ms，释放CPU，同时足够细粒度
//        QThread::yieldCurrentThread();
//    }
//}

//高精度延时（超时延时）
//void ExcelSendWorker::timeoutWait(int msec)
//{
//    if (msec <= 0) return;
//    timer.start();
//    while (timer.elapsed() < msec) {
//        QThread::yieldCurrentThread();
//        //如果没有超时，那就直接退出了
//        if(m_timeoutFlag) return;
//    }
//    //在这里发超时信号（这里就是超时的情况了）

//}

void ExcelSendWorker::integrationWait(int delay,int timeout){
    if (delay <= 0) return;
    if (timeout < delay) return;
    timer.start();
    //他的结束条件是m_stopFlag位
    while (!m_stopFlag && timer.elapsed() < delay) {
        QThread::yieldCurrentThread();
    }
    //如果发送等于超时，表明我要达成精确的发送时间
    //如果手动停止那个也直接退出
    if (delay == timeout || m_stopFlag) return;
    //这种情况表明，发送延时已经满足了，现在进入超时等待环节
    while (timer.elapsed() < timeout) {
        //如果最终没有超过超时时间，那就直接退出了
        if(m_timeoutFlag) return;
        QThread::yieldCurrentThread();
    }
    //在这里发超时信号（这里就是超时的情况了）
    sendTimeOut();
}

// 网络 Excel 连续发送函数
void ExcelSendWorker::startNetworkWork()
{
    m_stopFlag = false;
    m_timeoutFlag = false;
    m_sentCount = 0;
    qDebug() << "网络发送函数已触发";

    if (m_table == nullptr) {
        qWarning() << "m_table is null!";
        emit finished();
        return;
    }

    if (m_repeatLimit == 0) {
        // 无限循环模式
        while (!m_stopFlag) {
            for (int i = 0; i < m_totalRows && !m_stopFlag; ++i) {

                // 获取期望的返回值（第二列）
                QTableWidgetItem *expectedItem = m_table->item(i, 1);
                QString expectedText = expectedItem ? expectedItem->text() : "";
                QByteArray expectedBytes;
                if (m_useHexSend) {
                    expectedBytes = stringToHexBytes(expectedText);
                } else {
                    expectedBytes = expectedText.toUtf8();
                }
                emit commandSent(i, expectedBytes);   // 发射信号，传递行索引和期望字节

                //发送期望命令数据
                QTableWidgetItem *item = m_table->item(i, 0);
                if (!item) continue;
                QString cellText = item->text();
                if (cellText.isEmpty()) continue;

                QByteArray cmdToSend;
                if (m_useHexSend) {
                    cmdToSend = stringToHexBytes(cellText);
                } else {
                    cmdToSend = cellText.toUtf8();   // 普通文本以 UTF-8 发送
                }

                // 发送命令
                emit sendCommand(cmdToSend);
                integrationWait(m_delayMs,m_timeoutMs);
                emit sentCountChanged(++m_sentCount);
                //每次发送完毕都需要重制超时Flag
                m_timeoutFlag = false;
            }
        }
    } else {
        // 有限次数模式
        while (m_sentCount < m_repeatLimit && !m_stopFlag) {
            for (int i = 0; i < m_totalRows && m_sentCount < m_repeatLimit && !m_stopFlag; ++i) {

                // 获取期望的返回值（第二列）
                QTableWidgetItem *expectedItem = m_table->item(i, 1);
                QString expectedText = expectedItem ? expectedItem->text() : "";
                QByteArray expectedBytes;
                if (m_useHexSend) {
                    expectedBytes = stringToHexBytes(expectedText);
                } else {
                    expectedBytes = expectedText.toUtf8();
                }
                emit commandSent(i, expectedBytes);   // 发射信号，传递行索引和期望字节

                //发送期望命令数据
                QTableWidgetItem *item = m_table->item(i, 0);
                if (!item) continue;
                QString cellText = item->text();
                if (cellText.isEmpty()) continue;

                QByteArray cmdToSend;
                if (m_useHexSend) {
                    cmdToSend = stringToHexBytes(cellText);
                } else {
                    cmdToSend = cellText.toUtf8();
                }
                emit sendCommand(cmdToSend);
                integrationWait(m_delayMs,m_timeoutMs);
                emit sentCountChanged(++m_sentCount);
                //每次发送完毕都需要重制超时Flag
                m_timeoutFlag = false;
            }
        }
    }
    emit finished();  // 通知 GUI 发送结束
}

// 停止网络发送（在 GUI 线程调用）
void ExcelSendWorker::stopNetworkWork()
{
    qDebug() << "网络发送停止 Flag 设置完成";
    m_stopFlag = true;
}

// 串口 Excel 连续发送函数
void ExcelSendWorker::serialStartWork()
{
    m_stopFlag = false;
    m_timeoutFlag = false;
    m_sentCount = 0;
    qDebug() << "串口发送函数已触发";

    if (m_table == nullptr) {
        qWarning() << "m_table is null!";
        emit serialFinished();
        return;
    }

    if (m_repeatLimit == 0) {
        // 无限循环模式
        while (!m_stopFlag) {
            for (int i = 0; i < m_totalRows && !m_stopFlag; ++i) {

                // 获取期望的返回值（第二列）
                QTableWidgetItem *expectedItem = m_table->item(i, 1);
                QString expectedText = expectedItem ? expectedItem->text() : "";
                QByteArray expectedBytes;
                if (m_useHexSend) {
                    expectedBytes = stringToHexBytes(expectedText);
                } else {
                    expectedBytes = expectedText.toUtf8();
                }
                emit commandSent(i, expectedBytes);   // 发射信号，传递行索引和期望字节

                //发送期望命令数据
                QTableWidgetItem *item = m_table->item(i, 0);
                if (!item) continue;
                QString cellText = item->text();
                if (cellText.isEmpty()) continue;

                QByteArray cmdToSend;
                if (m_useHexSend) {
                    cmdToSend = stringToHexBytes(cellText);
                } else {
                    cmdToSend = cellText.toUtf8();
                }

                // 发送命令
                emit sendSerialCommand(cmdToSend);
                integrationWait(m_delayMs,m_timeoutMs);
                emit serialSentCountChanged(++m_sentCount);
                //每次发送完毕都需要重制超时Flag
                m_timeoutFlag = false;
            }
        }
    } else {
        // 有限次数模式
        while (m_sentCount < m_repeatLimit && !m_stopFlag) {
            for (int i = 0; i < m_totalRows && m_sentCount < m_repeatLimit && !m_stopFlag; ++i) {

                // 获取期望的返回值（第二列）
                QTableWidgetItem *expectedItem = m_table->item(i, 1);
                QString expectedText = expectedItem ? expectedItem->text() : "";
                QByteArray expectedBytes;
                if (m_useHexSend) {
                    expectedBytes = stringToHexBytes(expectedText);
                } else {
                    expectedBytes = expectedText.toUtf8();
                }
                emit commandSent(i, expectedBytes);   // 发射信号，传递行索引和期望字节

                //发送期望命令数据
                QTableWidgetItem *item = m_table->item(i, 0);
                if (!item) continue;
                QString cellText = item->text();
                if (cellText.isEmpty()) continue;

                QByteArray cmdToSend;
                if (m_useHexSend) {
                    cmdToSend = stringToHexBytes(cellText);
                } else {
                    cmdToSend = cellText.toUtf8();
                }

                // 发送命令
                emit sendSerialCommand(cmdToSend);
                integrationWait(m_delayMs,m_timeoutMs);
                emit serialSentCountChanged(++m_sentCount);
                //每次发送完毕都需要重制超时Flag
                m_timeoutFlag = false;
            }
        }
    }
    emit serialFinished();  // 通知 GUI 发送结束
}

// 停止串口发送（在 GUI 线程调用）
void ExcelSendWorker::serialStopWork()
{
    qDebug() << "串口发送停止 Flag 设置完成";
    m_stopFlag = true;
}

// 停止超时（在 WORK 线程调用）
void ExcelSendWorker::onExternalResponseReceived(const QByteArray &data)
{
    Q_UNUSED(data);
    m_timeoutFlag = true;   // 原子操作，线程安全
}
