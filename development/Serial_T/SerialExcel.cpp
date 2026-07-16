#include "SerialExcel.h"
#include "SerialPage.h"
#include "SerialWork.h"
#include "ElaWindow.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>
#include <QTableWidgetItem>
#include <QDateTime>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
//  ★ 静态辅助函数：ASCII 区间比较
// ═══════════════════════════════════════════════════════════════
static bool rangeCompareAscii(const QByteArray &received,
                               const QByteArray &expected,
                               double tolerance)
{
    QByteArray recvClean = received;
    QByteArray expectClean = expected;
    recvClean.replace("\r", "").replace("\n", "").replace(" ", "");
    expectClean.replace("\r", "").replace("\n", "").replace(" ", "");

    if (recvClean.isEmpty() || expectClean.isEmpty())
        return false;

    bool ok1 = false, ok2 = false;
    double recvVal   = recvClean.toDouble(&ok1);
    double expectVal = expectClean.toDouble(&ok2);

    if (!ok1 || !ok2)
        return false;

    return qAbs(recvVal - expectVal) <= tolerance;
}

SerialExcel::SerialExcel(SerialPage *page, QObject *parent)
    : QObject(parent), m_page(page), m_work(page->m_serialWork) {
    connect(m_page->m_excelDownloadTplBtn, &ElaPushButton::clicked, this, &SerialExcel::onDownloadTemplate);
    connect(m_page->m_excelOpenBtn, &ElaPushButton::clicked, this, &SerialExcel::onOpenExcel);
    connect(m_page->m_excelCaptureBtn, &ElaPushButton::clicked, this, &SerialExcel::onCapture);
    connect(m_page->m_excelSendBtn, &ElaPushButton::clicked, this, &SerialExcel::onStartSend);
    connect(m_page->m_excelStopBtn, &ElaPushButton::clicked, this, &SerialExcel::onStopSend);

    // 全局超时定时器（仍在主线程，超时不需要高精度）
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setTimerType(Qt::PreciseTimer); // 顺手也精确化
    connect(m_timeoutTimer, &QTimer::timeout, this, &SerialExcel::onGlobalTimeout);

    // ★ responseReceived 来自工作线程，自动 QueuedConnection
    connect(m_work, &SerialWork::responseReceived, this, &SerialExcel::onResponseReceived);

    // ★ 命令间隔延时到期（来自工作线程，自动 QueuedConnection）
    connect(m_work, &SerialWork::interCmdDelayFinished, this, &SerialExcel::onInterCmdDelayFinished);
}

SerialExcel::~SerialExcel() {
    m_timeoutTimer->stop();
    m_isRunning = false;
}

void SerialExcel::setRunning(bool running) {
    m_isRunning = running;
    m_page->m_excelCaptureBtn->setEnabled(!running);
    m_page->m_excelSendBtn->setEnabled(!running);
    m_page->m_excelStopBtn->setEnabled(running);
    m_page->m_excelOpenBtn->setEnabled(!running);
    m_page->m_excelRepeatCount->setEnabled(!running);
    m_page->m_excelTimeoutMs->setEnabled(!running);
}

// ═══════════════════════════════════════════════ ★ 读取返回值（捕获模式） ═══
void SerialExcel::onCapture() {
    if (m_isRunning || !m_work || !m_work->isOpen()) return;

    QTableWidget *table = m_page->m_excelTableWidget;
    int rowCount = table->rowCount();
    if (rowCount == 0) return;

    setRunning(true);

    m_isCaptureMode = true; // ★ 进入捕获模式
    m_currentRow = 0;
    m_repeatLeft = -1; // 不限次数，由行数控制结束
    m_totalSent = 0;
    m_pendingStop = false;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    onTrySendNext();
}

// ═══════════════════════════════════════════════ 开始发送 ═══
void SerialExcel::onStartSend() {
    if (m_isRunning || !m_work || !m_work->isOpen()) return;
    if (m_page->m_excelTableWidget->rowCount() == 0) return;

    int count = m_page->m_excelRepeatCount->text().toInt();
    if (count <= 0) count = -1;

    setRunning(true);

    m_isCaptureMode = false; // ★ 普通发送模式
    m_currentRow = 0;
    m_repeatLeft = count;
    m_totalSent = 0;
    m_pendingStop = false;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    onTrySendNext();
}

void SerialExcel::onStopSend() {
    m_timeoutTimer->stop();
    m_waiting = false;
    m_isCaptureMode = false; // ★ 退出捕获模式
    m_stickyQueue.clear(); // ★ 清空粘包队列

    // 清除期望值（跨线程）
    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));

    setRunning(false);
    qDebug() << "SerialExcel: 发送已停止，总计" << m_totalSent << "条";
}


// ═══════════════════════════════════════════════ 统一入口 ═══
void SerialExcel::onTrySendNext() {
    if (!m_isRunning || !m_work || !m_work->isOpen()) {
        onStopSend();
        return;
    }

    QTableWidget *table = m_page->m_excelTableWidget;
    int rowCount = table->rowCount();
    if (rowCount == 0) {
        onStopSend();
        return;
    }

    // ★ 捕获模式：所有行都发送完毕 → 停止
    if (m_isCaptureMode && m_currentRow >= rowCount) {
        onStopSend();
        qDebug() << "SerialExcel: 捕获模式完成，共" << m_totalSent << "条";
        return;
    }

    if (m_pendingStop) {
        onStopSend();
        qDebug() << "SerialExcel: 发送完成，总计" << m_totalSent << "条";
        return;
    }

    if (m_currentRow >= rowCount) m_currentRow = 0;

    if (m_repeatLeft > 0) {
        m_repeatLeft--;
        if (m_repeatLeft <= 0) m_pendingStop = true;
    }

    // 读表格
    QTableWidgetItem *cmdItem = table->item(m_currentRow, 0);
    QTableWidgetItem *expectItem = table->item(m_currentRow, 1);
    QTableWidgetItem *delayItem = table->item(m_currentRow, 2);

    QString cmdText = cmdItem ? cmdItem->text().trimmed() : "";
    QString expectedStr = expectItem ? expectItem->text().trimmed() : "";
    int delayMs = delayItem ? delayItem->text().toInt() : 100;
    if (delayMs < 0) delayMs = 100;

    int globalTimeout = m_page->m_excelTimeoutMs->text().toInt();
    if (globalTimeout <= 0) globalTimeout = 500;

    // ★ 捕获模式：适当延长超时
    if (m_isCaptureMode && globalTimeout < 2000) {
        globalTimeout = 2000;
    }

    m_currentRow++;
    if (cmdText.isEmpty()) {
        onTrySendNext();
        return;
    }

    // ★ 粘包队列消费：队列非空时，不发真实命令，直接消费队列数据
    if (m_page->m_serialSplitStickyCheckBox
        && m_page->m_serialSplitStickyCheckBox->isChecked()
        && !m_stickyQueue.isEmpty()) {
        QByteArray queuedData = m_stickyQueue.dequeue();
        m_totalSent++;
        m_page->m_logSentCountCard->setValue(QString::number(m_totalSent));

        m_waiting = true;
        m_gotReply = true; // 已有数据，无需等硬件回复
        m_minDelayOk = false;
        m_lastRecvData = queuedData;
        m_timeoutTimer->start(globalTimeout);

        if (m_isCaptureMode) {
            fillCaptureResult(queuedData);
        }

        QByteArray cmpData = queuedData;
        bool hexMode = m_page->m_serialHexSendCheckBox->isChecked();
        if (!hexMode && m_page->m_serialStripCRLFCheckBox->isChecked()) {
            cmpData.replace("\r", "");
            cmpData.replace("\n", "");
        }
        // ★ 区间判断
        if (!m_expectData.isEmpty()) {
            bool match = false;
            bool asciiRangeEnabled = m_page->m_serialAsciiRangeCheckBox
                                     && m_page->m_serialAsciiRangeCheckBox->isChecked();
            if (asciiRangeEnabled && !hexMode) {
                double tolerance = m_page->m_serialAsciiRangeEdit
                                   ? m_page->m_serialAsciiRangeEdit->text().toDouble()
                                   : 0.5;
                match = rangeCompareAscii(cmpData, m_expectData, tolerance);
            } else {
                match = (cmpData == m_expectData);
            }
            if (!match) {
                m_page->addContentError(m_lastCmd, m_expectData, cmpData);
            }
        }
        // 仍需遵守命令间隔延时
        if (delayMs > 0) {
            QTimer::singleShot(delayMs, this, [this]() {
                if (m_waiting) {
                    m_minDelayOk = true;
                    finalizeAndNext();
                }
            });
        } else {
            m_minDelayOk = true;
            finalizeAndNext();
        }
        return;
    }
    // 解析期望值
    bool hexMode = m_page->m_serialHexSendCheckBox->isChecked();
    if (expectedStr.isEmpty()) {
        m_expectData = QByteArray();
    } else {
        // ★ 将 Excel 中显示的转义字符 \r \n 还原为真实字节
        QString unescaped = expectedStr;
        unescaped.replace(QLatin1String("\\r"), QLatin1String("\r"));
        unescaped.replace(QLatin1String("\\n"), QLatin1String("\n"));
        m_expectData = hexMode
                           ? QByteArray::fromHex(unescaped.toLatin1())
                           : unescaped.toUtf8();
    }
    m_lastCmd = cmdText;

    // ★ 去除 \r\n：非 HEX 模式下，勾选后去除期望值中的 \r\n
    if (!hexMode && m_page->m_serialStripCRLFCheckBox->isChecked()) {
        m_expectData.replace("\r", "");
        m_expectData.replace("\n", "");
    }

    // ★ 对齐 GPIB：传递 forceRead 参数
    QMetaObject::invokeMethod(m_work, "sendStringWithDelay",
                              Qt::QueuedConnection,
                              Q_ARG(QString, cmdText),
                              Q_ARG(bool, hexMode),
                              Q_ARG(QByteArray, m_expectData),
                              Q_ARG(int, delayMs),
                              Q_ARG(bool, m_isCaptureMode)); // ★ forceRead

    m_totalSent++;
    m_page->m_logSentCountCard->setValue(QString::number(m_totalSent));

    // 进入等待
    m_waiting = true;
    m_gotReply = false;
    m_minDelayOk = false;

    // ★ 对齐 GPIB：始终启动超时定时器
    m_timeoutTimer->start(globalTimeout);
}

// ═══════════════════════════════════════════════ 收到回复 ═══
void SerialExcel::onResponseReceived(QByteArray data)
{
    if (!m_waiting) return;

    // ★ 粘包分割：按分隔符拆分，第一段用于当前比对，剩余入队
    bool splitMode = m_page->m_serialSplitStickyCheckBox
                     && m_page->m_serialSplitStickyCheckBox->isChecked();
    if (splitMode && !data.isEmpty()) {
        QByteArray delim = stickyDelimiter();
        QList<QByteArray> parts;
        int start = 0;
        int dlen  = delim.size();
        while (start < data.size()) {
            int idx = data.indexOf(delim, start);
            if (idx == -1) {
                QByteArray remaining = data.mid(start);
                if (!remaining.isEmpty())
                    parts.append(remaining);
                break;
            }
            QByteArray segment = data.mid(start, idx - start + dlen);
            if (!segment.isEmpty())
                parts.append(segment);
            start = idx + dlen;
        }
        if (!parts.isEmpty()) {
            data = parts.takeFirst();
            for (const QByteArray &p : parts)
                m_stickyQueue.enqueue(p);
        }
    }

    m_gotReply     = true;
    m_lastRecvData = data;

    // ★ 捕获模式：将返回值填入表格 B 列
    if (m_isCaptureMode) {
        fillCaptureResult(data);
    }

    // ★ 去除 \r\n：非 HEX 模式下，勾选后去除接收数据中的 \r\n 再比对
    QByteArray cmpData = data;
    bool hexMode = m_page->m_serialHexSendCheckBox->isChecked();
    if (!hexMode && m_page->m_serialStripCRLFCheckBox->isChecked()) {
        cmpData.replace("\r", "");
        cmpData.replace("\n", "");
    }

    // 比对
    // ★ 区间判断
    if (!m_expectData.isEmpty()) {
        bool match = false;
        bool asciiRangeEnabled = m_page->m_serialAsciiRangeCheckBox
                                 && m_page->m_serialAsciiRangeCheckBox->isChecked();
        if (asciiRangeEnabled && !hexMode) {
            double tolerance = m_page->m_serialAsciiRangeEdit
                               ? m_page->m_serialAsciiRangeEdit->text().toDouble()
                               : 0.5;
            match = rangeCompareAscii(cmpData, m_expectData, tolerance);
        } else {
            match = (cmpData == m_expectData);
        }
        if (!match) {
            m_page->addContentError(m_lastCmd, m_expectData, cmpData);
        }
    }

    // 延时已过 → 立刻下一条
    if (m_minDelayOk) finalizeAndNext();
}

// ═══════════════════════════════════════════════ ★ 工作线程精确延时到期 ═══
void SerialExcel::onInterCmdDelayFinished() {
    if (!m_waiting) return;

    m_minDelayOk = true;
    // ★ 设置命令时 m_gotReply 已在 onTrySendNext 中预设为 true，
    //    延时到期后立即进入下一条
    if (m_gotReply) finalizeAndNext();
}

// ═══════════════════════════════════════════════ 全局超时 ═══
void SerialExcel::onGlobalTimeout() {
    if (!m_waiting) return;

    // ★ 捕获模式：超时也填入 "(超时)"
    if (m_isCaptureMode) {
        fillCaptureTimeout();
    }

    // 超时前还没收到回复但有期望值 → 记录超时错误
    if (!m_gotReply && !m_expectData.isEmpty()) {
        m_page->addTimeoutError(m_lastCmd, m_expectData);
    }
    finalizeAndNext();
}

// ═══════════════════════════════════════════════ 结算 → 下一条 ═══
void SerialExcel::finalizeAndNext() {
    m_timeoutTimer->stop();
    m_waiting = false;

    // 清除期望值（跨线程）
    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));

    onTrySendNext();
}

// ═══════════════════════════════════════════════ ★ 捕获模式：填入返回值 ═══
void SerialExcel::fillCaptureResult(const QByteArray &data) {
    // m_currentRow 已在上一条发送时递增，当前回复对应 row = m_currentRow - 1
    int row = m_currentRow - 1;
    QTableWidget *table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    bool hexMode = m_page->m_serialHexSendCheckBox->isChecked();
    QString displayText;
    if (hexMode) {
        displayText = data.toHex(' ').toUpper();
    } else {
        displayText = QString::fromUtf8(data);
        if (displayText.isEmpty())
            displayText = data.toHex(' ').toUpper();
    }

    QTableWidgetItem *item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    // ★ 将控制字符转义为可见字符串
    displayText.replace(QLatin1Char('\r'), QLatin1String("\\r"));
    displayText.replace(QLatin1Char('\n'), QLatin1String("\\n"));

    item->setText(displayText);

    qDebug() << "SerialExcel: 捕获模式 — 第" << (row + 1) << "行返回值已填入:" << displayText;
}

// ═══════════════════════════════════════════════ ★ 捕获模式：超时标记 ═══
void SerialExcel::fillCaptureTimeout() {
    int row = m_currentRow - 1;
    QTableWidget *table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem *item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    // 只有当单元格为空时才填入超时标记
    if (item->text().isEmpty()) {
        item->setText("(超时)");
    }

    qDebug() << "SerialExcel: 捕获模式 — 第" << (row + 1) << "行超时";
}

// ═══════════════════════════════════════════════ 打开 Excel 并读取 ═══
void SerialExcel::onOpenExcel() {
    QString filePath = QFileDialog::getOpenFileName(
        m_page->m_mainWindow,
        "选择 Excel 文件",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Excel 文件 (*.xlsx *.xls)"
    );

    if (filePath.isEmpty())
        return;

    if (!loadExcelToTable(filePath)) {
        QMessageBox::critical(m_page->m_mainWindow, "错误",
                              "无法读取 Excel 文件:\n" + filePath);
    }
}

// ═══════════════════════════════════════════════ 加载到表格 ═══
bool SerialExcel::loadExcelToTable(const QString &filePath) {
    QXlsx::Document xlsx(filePath);
    if (!xlsx.load())
        return false;

    QStringList sheetNames = xlsx.sheetNames();
    if (sheetNames.isEmpty())
        return false;

    xlsx.selectSheet(sheetNames.at(0));

    int maxRow = xlsx.dimension().rowCount();
    int maxCol = qMin(xlsx.dimension().columnCount(), 3);

    if (maxRow < 2) {
        QMessageBox::information(m_page->m_mainWindow, "提示",
                                 "Excel 文件为空（至少需要表头 + 一行数据）。");
        return false;
    }

    QTableWidget *table = m_page->m_excelTableWidget;
    table->clearContents();
    table->setRowCount(0);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({
        "发送的命令", "正确的返回值", "到下一条命令的时间ms"
    });

    int dataRowCount = maxRow - 1;
    table->setRowCount(dataRowCount);

    for (int row = 2; row <= maxRow; ++row) {
        int tableRow = row - 2;
        for (int col = 1; col <= maxCol; ++col) {
            QVariant cell = xlsx.read(row, col);
            QString text;
            if (cell.isNull()) {
                text = "";
            } else if (cell.type() == QVariant::Double) {
                if (cell.toDouble() == qint64(cell.toDouble()))
                    text = QString::number(qint64(cell.toDouble()));
                else
                    text = cell.toString();
            } else {
                text = cell.toString().trimmed();
            }
            table->setItem(tableRow, col - 1, new QTableWidgetItem(text));
        }
    }

    bool hasData = (dataRowCount > 0);
    bool portOpen = m_page->m_serialWork && m_page->m_serialWork->isOpen();
    m_page->m_excelSendBtn->setEnabled(hasData && portOpen);
    m_page->m_excelCaptureBtn->setEnabled(hasData && portOpen);

    qDebug() << "SerialExcel: 加载了" << dataRowCount << "行数据";
    return true;
}

// ═══════════════════════════════════════════════ 下载模板 ═══
void SerialExcel::onDownloadTemplate() {
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                          + "/串口通讯示例模板.xlsx";

    QString filePath = QFileDialog::getSaveFileName(
        m_page->m_mainWindow, "保存示例模板", defaultPath, "Excel 文件 (*.xlsx)");

    if (filePath.isEmpty())
        return;

    if (!generateExcelTemplate(filePath)) {
        QMessageBox::critical(m_page->m_mainWindow, "错误", "生成模板文件失败！");
    } else {
        QMessageBox::information(m_page->m_mainWindow, "成功",
                                 "示例模板已保存到:\n" + filePath);
    }
}

// ═══════════════════════════════════════════════ 生成模板 ═══
bool SerialExcel::generateExcelTemplate(const QString &filePath) {
    QXlsx::Document xlsx;

    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFontSize(11);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);
    headerFormat.setPatternBackgroundColor(QColor(68, 114, 196));
    headerFormat.setFontColor(QColor(255, 255, 255));

    QXlsx::Format cmdFormat;
    cmdFormat.setFontSize(11);
    cmdFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    cmdFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    cmdFormat.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format returnFormat;
    returnFormat.setFontSize(11);
    returnFormat.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    returnFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    returnFormat.setBorderStyle(QXlsx::Format::BorderThin);
    returnFormat.setPatternBackgroundColor(QColor(255, 255, 200));

    QXlsx::Format delayFormat;
    delayFormat.setFontSize(11);
    delayFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    delayFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    delayFormat.setBorderStyle(QXlsx::Format::BorderThin);

    xlsx.write(1, 1, "发送的命令", headerFormat);
    xlsx.write(1, 2, "正确的返回值", headerFormat);
    xlsx.write(1, 3, "到下一条命令的时间ms", headerFormat);

    struct Sample {
        QString command;
        int delayMs;
    };
    QList<Sample> samples = {
        {"*IDN?", 50},
        {"SYST:ERR?", 100},
        {"MEAS:VOLT:DC?", 200},
        {"CONF:VOLT:DC 10", 100},
        {"READ?", 300},
        {"SYST:LOC", 50},
        {"*RST", 500},
    };

    for (int i = 0; i < samples.size(); ++i) {
        int row = i + 2;
        xlsx.write(row, 1, samples[i].command, cmdFormat);
        xlsx.write(row, 2, "", returnFormat);
        xlsx.write(row, 3, samples[i].delayMs, delayFormat);
    }

    xlsx.setColumnWidth(1, 35);
    xlsx.setColumnWidth(2, 35);
    xlsx.setColumnWidth(3, 25);

    return xlsx.saveAs(filePath);
}

// ═══════════════════════════════════════════════ ★ 获取粘包分隔符 ═══
QByteArray SerialExcel::stickyDelimiter() const
{
    if (!m_page->m_serialSplitDelimiterComboBox)
        return QByteArray("\n");
    QString text = m_page->m_serialSplitDelimiterComboBox->currentText();
    if (text == "\\r")      return QByteArray("\r");
    if (text == "\\r\\n")   return QByteArray("\r\n");
    return QByteArray("\n");
}
