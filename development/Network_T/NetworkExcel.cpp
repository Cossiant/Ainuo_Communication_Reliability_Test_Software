#include "NetworkExcel.h"
#include "NetworkPage.h"
#include "NetworkWork.h"
#include "ElaWindow.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>
#include <QTableWidgetItem>
#include <QDateTime>

NetworkExcel::NetworkExcel(NetworkPage* page, QObject *parent)
    : QObject(parent), m_page(page), m_work(page->m_networkWork)
{
    connect(m_page->m_excelDownloadTplBtn, &ElaPushButton::clicked, this, &NetworkExcel::onDownloadTemplate);
    connect(m_page->m_excelOpenBtn,        &ElaPushButton::clicked, this, &NetworkExcel::onOpenExcel);
    connect(m_page->m_excelCaptureBtn,     &ElaPushButton::clicked, this, &NetworkExcel::onCapture);
    connect(m_page->m_excelSendBtn,        &ElaPushButton::clicked, this, &NetworkExcel::onStartSend);
    connect(m_page->m_excelStopBtn,        &ElaPushButton::clicked, this, &NetworkExcel::onStopSend);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setTimerType(Qt::PreciseTimer);
    connect(m_timeoutTimer, &QTimer::timeout, this, &NetworkExcel::onGlobalTimeout);

    connect(m_work, &NetworkWork::responseReceived, this, &NetworkExcel::onResponseReceived);
    connect(m_work, &NetworkWork::interCmdDelayFinished, this, &NetworkExcel::onInterCmdDelayFinished);
}

NetworkExcel::~NetworkExcel()
{
    m_timeoutTimer->stop();
    m_isRunning = false;
}

void NetworkExcel::setRunning(bool running)
{
    m_isRunning = running;
    m_page->m_excelCaptureBtn->setEnabled(!running);
    m_page->m_excelSendBtn->setEnabled(!running);
    m_page->m_excelStopBtn->setEnabled(running);
    m_page->m_excelOpenBtn->setEnabled(!running);
    m_page->m_excelRepeatCount->setEnabled(!running);
    m_page->m_excelTimeoutMs->setEnabled(!running);
}

// ═══════════════════════════════════════════════ ★ 读取返回值（捕获模式） ═══
void NetworkExcel::onCapture()
{
    if (m_isRunning || !m_work || !m_work->isOpen()) return;

    QTableWidget* table = m_page->m_excelTableWidget;
    int rowCount = table->rowCount();
    if (rowCount == 0) return;

    setRunning(true);

    m_isCaptureMode = true;       // ★ 进入捕获模式
    m_currentRow    = 0;
    m_repeatLeft    = -1;         // 不限次数，由行数控制结束
    m_totalSent     = 0;
    m_pendingStop   = false;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    onTrySendNext();
}

// ═══════════════════════════════════════════════ 开始发送 ═══
void NetworkExcel::onStartSend()
{
    if (m_isRunning || !m_work || !m_work->isOpen()) return;
    if (m_page->m_excelTableWidget->rowCount() == 0) return;

    int count = m_page->m_excelRepeatCount->text().toInt();
    if (count <= 0) count = -1;

    setRunning(true);

    m_isCaptureMode = false;      // ★ 普通发送模式
    m_currentRow    = 0;
    m_repeatLeft    = count;
    m_totalSent     = 0;
    m_pendingStop   = false;

    m_page->clearExcelSendLog();
    m_page->m_logStartTimeCard->setValue(QDateTime::currentDateTime().toString("HH:mm:ss"));

    onTrySendNext();
}

// ═══════════════════════════════════════════════ 停止 ═══
void NetworkExcel::onStopSend()
{
    m_timeoutTimer->stop();
    m_waiting = false;
    m_isCaptureMode = false;       // ★ 退出捕获模式

    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));

    setRunning(false);
    qDebug() << "NetworkExcel: 发送已停止，总计" << m_totalSent << "条";
}

// ═══════════════════════════════════════════════ 统一入口 ═══
void NetworkExcel::onTrySendNext()
{
    if (!m_isRunning || !m_work || !m_work->isOpen()) { onStopSend(); return; }

    QTableWidget* table = m_page->m_excelTableWidget;
    int rowCount = table->rowCount();
    if (rowCount == 0) { onStopSend(); return; }

    // ★ 捕获模式：所有行都发送完毕 → 停止
    if (m_isCaptureMode && m_currentRow >= rowCount) {
        onStopSend();
        qDebug() << "NetworkExcel: 捕获模式完成，共" << m_totalSent << "条";
        return;
    }

    if (m_pendingStop) {
        onStopSend();
        qDebug() << "NetworkExcel: 发送完成，总计" << m_totalSent << "条";
        return;
    }

    if (m_currentRow >= rowCount) m_currentRow = 0;

    if (m_repeatLeft > 0) {
        m_repeatLeft--;
        if (m_repeatLeft <= 0) m_pendingStop = true;
    }

    QTableWidgetItem* cmdItem    = table->item(m_currentRow, 0);
    QTableWidgetItem* expectItem = table->item(m_currentRow, 1);
    QTableWidgetItem* delayItem  = table->item(m_currentRow, 2);

    QString cmdText     = cmdItem    ? cmdItem->text().trimmed()    : "";
    QString expectedStr = expectItem ? expectItem->text().trimmed() : "";
    int delayMs         = delayItem  ? delayItem->text().toInt()    : 100;
    if (delayMs <= 0) delayMs = 100;

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

    bool hexMode = m_page->m_networkHexSendCheckBox->isChecked();
    m_expectData = expectedStr.isEmpty() ? QByteArray()
                   : hexMode ? QByteArray::fromHex(expectedStr.toLatin1())
                             : expectedStr.toUtf8();
    m_lastCmd = cmdText;

    QMetaObject::invokeMethod(m_work, "sendStringWithDelay",
                              Qt::QueuedConnection,
                              Q_ARG(QString, cmdText),
                              Q_ARG(bool, hexMode),
                              Q_ARG(QByteArray, m_expectData),
                              Q_ARG(int, delayMs));

    m_totalSent++;
    m_page->m_logSentCountCard->setValue(QString::number(m_totalSent));

    m_waiting    = true;
    m_gotReply   = false;
    m_minDelayOk = false;

    m_timeoutTimer->start(globalTimeout);
}

// ═══════════════════════════════════════════════ 收到回复 ═══
void NetworkExcel::onResponseReceived(QByteArray data)
{
    if (!m_waiting) return;

    m_gotReply     = true;
    m_lastRecvData = data;

    // ★ 捕获模式：将返回值填入表格 B 列
    if (m_isCaptureMode) {
        fillCaptureResult(data);
    }

    if (!m_expectData.isEmpty() && data != m_expectData) {
        m_page->addContentError(m_lastCmd, m_expectData, data);
    }

    if (m_minDelayOk) finalizeAndNext();
}

// ═══════════════════════════════════════════════ ★ 工作线程精确延时到期 ═══
void NetworkExcel::onInterCmdDelayFinished()
{
    if (!m_waiting) return;

    m_minDelayOk = true;
    if (m_gotReply) finalizeAndNext();
}

// ═══════════════════════════════════════════════ 全局超时 ═══
void NetworkExcel::onGlobalTimeout()
{
    if (!m_waiting) return;

    // ★ 捕获模式：超时也填入 "(超时)"
    if (m_isCaptureMode) {
        fillCaptureTimeout();
    }

    if (!m_gotReply && !m_expectData.isEmpty()) {
        m_page->addTimeoutError(m_lastCmd, m_expectData);
    }
    finalizeAndNext();
}

// ═══════════════════════════════════════════════ 结算 → 下一条 ═══
void NetworkExcel::finalizeAndNext()
{
    m_timeoutTimer->stop();
    m_waiting = false;

    QMetaObject::invokeMethod(m_work, "setExpectedResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, QByteArray()));

    onTrySendNext();
}

// ═══════════════════════════════════════════════ ★ 捕获模式：填入返回值 ═══
void NetworkExcel::fillCaptureResult(const QByteArray &data)
{
    // m_currentRow 已在上一条发送时递增，当前回复对应 row = m_currentRow - 1
    int row = m_currentRow - 1;
    QTableWidget* table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    bool hexMode = m_page->m_networkHexSendCheckBox->isChecked();
    QString displayText;
    if (hexMode) {
        displayText = data.toHex(' ').toUpper();
    } else {
        displayText = QString::fromUtf8(data);
        if (displayText.isEmpty())
            displayText = data.toHex(' ').toUpper();
    }

    QTableWidgetItem* item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    item->setText(displayText);

    qDebug() << "NetworkExcel: 捕获模式 — 第" << (row + 1) << "行返回值已填入:" << displayText;
}

// ═══════════════════════════════════════════════ ★ 捕获模式：超时标记 ═══
void NetworkExcel::fillCaptureTimeout()
{
    int row = m_currentRow - 1;
    QTableWidget* table = m_page->m_excelTableWidget;
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem* item = table->item(row, 1);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, 1, item);
    }
    // 只有当单元格为空时才填入超时标记
    if (item->text().isEmpty()) {
        item->setText("(超时)");
    }

    qDebug() << "NetworkExcel: 捕获模式 — 第" << (row + 1) << "行超时";
}

// ═══════════════════════════════════════════════ 打开 Excel 并读取 ═══
void NetworkExcel::onOpenExcel()
{
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

bool NetworkExcel::loadExcelToTable(const QString &filePath)
{
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

    QTableWidget* table = m_page->m_excelTableWidget;
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

    bool hasData  = (dataRowCount > 0);
    bool portOpen = m_page->m_networkWork && m_page->m_networkWork->isOpen();
    m_page->m_excelSendBtn->setEnabled(hasData && portOpen);
    m_page->m_excelCaptureBtn->setEnabled(hasData && portOpen);

    qDebug() << "NetworkExcel: 加载了" << dataRowCount << "行数据";
    return true;
}

void NetworkExcel::onDownloadTemplate()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                          + "/网口通讯示例模板.xlsx";

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

bool NetworkExcel::generateExcelTemplate(const QString &filePath)
{
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

    xlsx.write(1, 1, "发送的命令",          headerFormat);
    xlsx.write(1, 2, "正确的返回值",        headerFormat);
    xlsx.write(1, 3, "到下一条命令的时间ms", headerFormat);

    struct Sample { QString command; int delayMs; };
    QList<Sample> samples = {
        {"*IDN?",            50},
        {"SYST:ERR?",       100},
        {"MEAS:VOLT:DC?",   200},
        {"CONF:VOLT:DC 10", 100},
        {"READ?",           300},
        {"SYST:LOC",         50},
        {"*RST",            500},
    };

    for (int i = 0; i < samples.size(); ++i) {
        int row = i + 2;
        xlsx.write(row, 1, samples[i].command, cmdFormat);
        xlsx.write(row, 2, "",                 returnFormat);
        xlsx.write(row, 3, samples[i].delayMs, delayFormat);
    }

    xlsx.setColumnWidth(1, 35);
    xlsx.setColumnWidth(2, 35);
    xlsx.setColumnWidth(3, 25);

    return xlsx.saveAs(filePath);
}
