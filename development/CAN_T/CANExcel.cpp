#include "CANExcel.h"

#include "CANPage.h"
#include "CANWork.h"
#include "CANFrameUtils.h"
#include "ElaWindow.h"

#include "xlsxdocument.h"
#include "xlsxformat.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDateTime>

CANExcel::CANExcel(CANPage *page, QObject *parent)
    : QObject(parent), m_page(page), m_work(page->m_canWork)
{
    connect(m_page->m_excelDownloadTplBtn, &ElaPushButton::clicked,
            this, &CANExcel::onDownloadTemplate);
    connect(m_page->m_excelOpenBtn, &ElaPushButton::clicked,
            this, &CANExcel::onOpenExcel);
    connect(m_page->m_excelCaptureBtn, &ElaPushButton::clicked,
            this, &CANExcel::onCapture);
    connect(m_page->m_excelSendBtn, &ElaPushButton::clicked,
            this, &CANExcel::onStartSend);
    connect(m_page->m_excelStopBtn, &ElaPushButton::clicked,
            this, &CANExcel::onStopSend);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setTimerType(Qt::PreciseTimer);
    connect(m_timeoutTimer, &QTimer::timeout, this, &CANExcel::onGlobalTimeout);

    connect(m_work, &CANWork::responseReceived, this, &CANExcel::onResponseReceived);
    connect(m_work, &CANWork::commandWritten, this, &CANExcel::onCommandWritten);
}

CANExcel::~CANExcel()
{
    m_timeoutTimer->stop();
    m_isRunning = false;
}

void CANExcel::setRunning(bool running)
{
    m_isRunning = running;
    m_page->m_excelCaptureBtn->setEnabled(!running);
    m_page->m_excelSendBtn->setEnabled(!running);
    m_page->m_excelStopBtn->setEnabled(running);
    m_page->m_excelOpenBtn->setEnabled(!running);
    m_page->m_excelRepeatCount->setEnabled(!running);
    m_page->m_excelTimeoutMs->setEnabled(!running);
}

void CANExcel::onCapture()
{
    if (m_isRunning || !m_work || !m_work->isOpen()) return;
    if (m_page->m_excelTableWidget->rowCount() == 0) return;

    setRunning(true);
    m_isCaptureMode = true;
    m_currentRow = 0;
    m_repeatLeft = -1;
    m_totalSent = 0;
    m_pendingStop = false;
    m_cmdGeneration = 0;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    QMetaObject::invokeMethod(m_work, "resetTimingCompensation", Qt::QueuedConnection);
    onTrySendNext();
}

void CANExcel::onStartSend()
{
    if (m_isRunning || !m_work || !m_work->isOpen()) return;
    if (m_page->m_excelTableWidget->rowCount() == 0) return;

    int count = m_page->m_excelRepeatCount->text().toInt();
    if (count <= 0) count = -1;

    setRunning(true);
    m_isCaptureMode = false;
    m_currentRow = 0;
    m_repeatLeft = count;
    m_totalSent = 0;
    m_pendingStop = false;
    m_cmdGeneration = 0;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    QMetaObject::invokeMethod(m_work, "resetTimingCompensation", Qt::QueuedConnection);
    onTrySendNext();
}

void CANExcel::onStopSend()
{
    m_timeoutTimer->stop();
    m_waiting = false;
    m_isCaptureMode = false;

    QMetaObject::invokeMethod(m_work, "cancelPendingCommands", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));

    setRunning(false);
    qDebug() << "CANExcel: 发送已停止，总计" << m_totalSent << "条";
}

void CANExcel::onTrySendNext()
{
    if (!m_isRunning || !m_work || !m_work->isOpen()) {
        onStopSend();
        return;
    }

    QTableWidget *table = m_page->m_excelTableWidget;
    const int rowCount = table->rowCount();
    if (rowCount == 0) {
        onStopSend();
        return;
    }

    if (m_isCaptureMode && m_currentRow >= rowCount) {
        onStopSend();
        qDebug() << "CANExcel: 捕获模式完成，共" << m_totalSent << "条";
        return;
    }
    if (m_pendingStop) {
        onStopSend();
        qDebug() << "CANExcel: 发送完成，总计" << m_totalSent << "条";
        return;
    }

    if (m_currentRow >= rowCount) {
        m_currentRow = 0;
    }

    if (m_repeatLeft > 0) {
        --m_repeatLeft;
        if (m_repeatLeft <= 0) {
            m_pendingStop = true;
        }
    }

    QTableWidgetItem *cmdItem = table->item(m_currentRow, 0);
    QTableWidgetItem *expectItem = table->item(m_currentRow, 1);
    QTableWidgetItem *delayItem = table->item(m_currentRow, 2);

    const QString cmdText = cmdItem ? cmdItem->text().trimmed() : QString();
    const QString expectedStr = expectItem ? expectItem->text().trimmed() : QString();
    int delayMs = delayItem ? delayItem->text().toInt() : 100;
    if (delayMs < 0) delayMs = 100;

    int globalTimeout = m_page->m_excelTimeoutMs->text().toInt();
    if (globalTimeout < 0) globalTimeout = 500;
    if (m_isCaptureMode && globalTimeout < 2000) globalTimeout = 2000;

    ++m_currentRow;

    if (cmdText.isEmpty()) {
        onTrySendNext();
        return;
    }

    const int myGen = ++m_cmdGeneration;

    quint32 canId = 0;
    QByteArray canData;
    bool isFd = false;
    bool brs = false;
    QString parseError;
    if (!CANFrameUtils::parseFrameText(cmdText, canId, canData, isFd, brs, &parseError)) {
        ++m_totalSent;
        m_page->m_logSentCountCard->setValue(QString::number(m_totalSent));
        m_page->addContentError(cmdText, expectedStr, parseError);
        finalizeAndNext();
        return;
    }

    QByteArray expectedFrame;
    if (!expectedStr.isEmpty()) {
        quint32 expId = 0;
        QByteArray expData;
        bool expFd = false;
        bool expBrs = false;
        if (CANFrameUtils::parseFrameText(expectedStr, expId, expData, expFd, expBrs)) {
            expectedFrame = CANFrameUtils::encodeFrame(expId, expData, expFd, expBrs);
        } else {
            expectedFrame = expectedStr.toUtf8();   // 解析失败时按原始文本比较
        }
    }

    const QByteArray frame = CANFrameUtils::encodeFrame(canId, canData, isFd, brs);
    m_lastCmd = cmdText;
    m_expectFrame = expectedFrame;

    QMetaObject::invokeMethod(m_work, "sendFrameWithDelay",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, frame),
                              Q_ARG(QByteArray, expectedFrame),
                              Q_ARG(int, delayMs),
                              Q_ARG(bool, m_isCaptureMode),
                              Q_ARG(int, myGen));

    ++m_totalSent;
    m_page->m_logSentCountCard->setValue(QString::number(m_totalSent));

    m_currentTimeoutMs = globalTimeout;
    m_waiting = true;
    m_gotReply = false;
}

void CANExcel::onResponseReceived(QByteArray frame)
{
    if (!m_waiting) return;

    m_gotReply = true;
    m_lastRecvFrame = frame;

    if (m_isCaptureMode) {
        fillCaptureResult(frame);
    }

    if (!m_expectFrame.isEmpty() && frame != m_expectFrame) {
        m_page->addContentError(m_lastCmd,
                                CANFrameUtils::formatFrame(m_expectFrame),
                                CANFrameUtils::formatFrame(frame));
    }

    // 回复到达即预投递下一条，worker 会按住到精确截止时刻
    finalizeAndNext();
}

void CANExcel::onCommandWritten(int generation)
{
    if (!m_waiting) return;
    if (generation != m_cmdGeneration) {
        qDebug() << "CANExcel: [忽略过期写入信号] gen=" << generation
                 << "当前gen=" << m_cmdGeneration;
        return;
    }
    m_timeoutTimer->start(m_currentTimeoutMs);
}

void CANExcel::onGlobalTimeout()
{
    if (!m_waiting) return;
    if (m_isCaptureMode) {
        fillCaptureTimeout();
    }
    if (!m_gotReply && !m_expectFrame.isEmpty()) {
        m_page->addTimeoutError(m_lastCmd, CANFrameUtils::formatFrame(m_expectFrame));
    }
    finalizeAndNext();
}

void CANExcel::finalizeAndNext()
{
    m_timeoutTimer->stop();
    m_waiting = false;
    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));
    onTrySendNext();
}

void CANExcel::fillCaptureResult(const QByteArray &frame)
{
    const int row = m_currentRow - 1;
    QTableWidget *table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem *item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    item->setText(CANFrameUtils::formatFrame(frame));
}

void CANExcel::fillCaptureTimeout()
{
    const int row = m_currentRow - 1;
    QTableWidget *table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem *item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    if (item->text().isEmpty()) {
        item->setText(QStringLiteral("(超时)"));
    }
}

void CANExcel::onOpenExcel()
{
    const QString filePath = QFileDialog::getOpenFileName(
        m_page->m_mainWindow, QStringLiteral("选择 Excel 文件"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QStringLiteral("Excel 文件 (*.xlsx *.xls)"));
    if (filePath.isEmpty()) return;

    if (!loadExcelToTable(filePath)) {
        QMessageBox::critical(m_page->m_mainWindow, QStringLiteral("错误"),
                              QStringLiteral("无法读取 Excel 文件：\n") + filePath);
    }
}

bool CANExcel::loadExcelToTable(const QString &filePath)
{
    QXlsx::Document xlsx(filePath);
    if (!xlsx.load()) return false;

    QStringList sheetNames = xlsx.sheetNames();
    if (sheetNames.isEmpty()) return false;
    xlsx.selectSheet(sheetNames.at(0));

    const int maxRow = xlsx.dimension().rowCount();
    const int maxCol = qMin(xlsx.dimension().columnCount(), 3);
    if (maxRow < 2) {
        QMessageBox::information(m_page->m_mainWindow, QStringLiteral("提示"),
                                 QStringLiteral("Excel 文件为空（至少需要表头 + 一行数据）。"));
        return false;
    }

    QTableWidget *table = m_page->m_excelTableWidget;
    table->clearContents();
    table->setRowCount(0);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("发送的CAN帧(ID 数据HEX)"),
                                      QStringLiteral("正确的返回值"),
                                      QStringLiteral("到下一条命令的时间ms")});

    const int dataRowCount = maxRow - 1;
    table->setRowCount(dataRowCount);
    for (int row = 2; row <= maxRow; ++row) {
        const int tableRow = row - 2;
        for (int col = 1; col <= maxCol; ++col) {
            QVariant cell = xlsx.read(row, col);
            QString text;
            if (cell.isNull()) {
                text = QString();
            } else if (cell.type() == QVariant::Double) {
                if (cell.toDouble() == qint64(cell.toDouble())) {
                    text = QString::number(qint64(cell.toDouble()));
                } else {
                    text = cell.toString();
                }
            } else {
                text = cell.toString().trimmed();
            }
            table->setItem(tableRow, col - 1, new QTableWidgetItem(text));
        }
    }

    const bool hasData = dataRowCount > 0;
    const bool portOpen = m_work && m_work->isOpen();
    m_page->m_excelSendBtn->setEnabled(hasData && portOpen);
    m_page->m_excelCaptureBtn->setEnabled(hasData && portOpen);
    return true;
}

void CANExcel::onDownloadTemplate()
{
    const QString defaultPath =
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            + QStringLiteral("/CAN通讯示例模板.xlsx");
    const QString filePath = QFileDialog::getSaveFileName(
        m_page->m_mainWindow, QStringLiteral("保存示例模板"),
        defaultPath, QStringLiteral("Excel 文件 (*.xlsx)"));
    if (filePath.isEmpty()) return;

    if (!generateExcelTemplate(filePath)) {
        QMessageBox::critical(m_page->m_mainWindow, QStringLiteral("错误"),
                              QStringLiteral("生成模板文件失败！"));
    } else {
        QMessageBox::information(m_page->m_mainWindow, QStringLiteral("成功"),
                                 QStringLiteral("示例模板已保存到：\n") + filePath);
    }
}

bool CANExcel::generateExcelTemplate(const QString &filePath)
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFontSize(11);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);
    headerFormat.setPatternBackgroundColor(QColor(68, 114, 196));
    headerFormat.setFontColor(QColor(Qt::white));

    QXlsx::Format cellFormat;
    cellFormat.setFontSize(11);
    cellFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    cellFormat.setBorderStyle(QXlsx::Format::BorderThin);

    xlsx.write(1, 1, QStringLiteral("发送的CAN帧(ID 数据HEX)"), headerFormat);
    xlsx.write(1, 2, QStringLiteral("正确的返回值"), headerFormat);
    xlsx.write(1, 3, QStringLiteral("到下一条命令的时间ms"), headerFormat);

    struct Sample { QString frame; int delayMs; };
    const QList<Sample> samples = {
        {QStringLiteral("100 11 22 33 44"), 50},
        {QStringLiteral("200 AA BB"), 100},
        {QStringLiteral("7FF 01 02"), 200},
    };
    for (int i = 0; i < samples.size(); ++i) {
        xlsx.write(i + 2, 1, samples[i].frame, cellFormat);
        xlsx.write(i + 2, 2, QString(), cellFormat);
        xlsx.write(i + 2, 3, samples[i].delayMs, cellFormat);
    }

    xlsx.setColumnWidth(1, 35);
    xlsx.setColumnWidth(2, 35);
    xlsx.setColumnWidth(3, 25);
    return xlsx.saveAs(filePath);
}
