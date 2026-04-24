#include "ExcelSendWorker.h"
#include <QThread>

ExcelSendWorker::ExcelSendWorker(QObject *parent)
    : QObject(parent), m_stopFlag(false)
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
void interruptibleWait(int msec)
{
    QTimer timer;
    timer.setTimerType(Qt::PreciseTimer);
    timer.start(msec);
    while (timer.remainingTime() > 0) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

// 网络 Excel 连续发送函数
void ExcelSendWorker::startNetworkWork()
{
    m_stopFlag = false;
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

                emit sendCommand(cmdToSend, m_delayMs);
                interruptibleWait(m_delayMs);
                emit sentCountChanged(++m_sentCount);
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
                emit sendCommand(cmdToSend, m_delayMs);
                interruptibleWait(m_delayMs);
                emit sentCountChanged(++m_sentCount);

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

                emit sendSerialCommand(cmdToSend, m_delayMs);
                interruptibleWait(m_delayMs);
                emit serialSentCountChanged(++m_sentCount);
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

                emit sendSerialCommand(cmdToSend, m_delayMs);
                interruptibleWait(m_delayMs);
                emit serialSentCountChanged(++m_sentCount);
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
