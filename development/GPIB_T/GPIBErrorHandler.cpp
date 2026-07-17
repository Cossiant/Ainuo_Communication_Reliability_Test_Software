// GPIBErrorHandler.cpp

#include "GPIBErrorHandler.h"
#include "GPIBPage.h"
#include "ElaToggleSwitch.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QColor>
#include <QDebug>

GPIBErrorHandler::GPIBErrorHandler(GPIBPage* page)
    : QObject(page), m_page(page)
{
}

QString GPIBErrorHandler::bytesToDisplayText(const QByteArray &data, bool isHexMode)
{
    if (data.isEmpty())
        return QString::fromUtf8("—");
    if (isHexMode) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty()) {
            text.replace(QLatin1Char('\r'), QLatin1String("\\r"));
            text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
            return text;
        } else {
            return data.toHex(' ').toUpper();
        }
    }
}

void GPIBErrorHandler::addTimeoutError(const QString &command, const QByteArray &expected)
{
    ++m_page->m_errorSeq;
    ++m_page->m_timeoutCount;

    int total = m_page->m_timeoutCount + m_page->m_contentCount;
    m_page->m_errorTotalCard->setValue(QString::number(total));
    m_page->m_errorTimeoutCard->setValue(QString::number(m_page->m_timeoutCount));

    if (m_page->m_errorTable->rowCount() == 1
        && m_page->m_errorTable->item(0, 1)
        && m_page->m_errorTable->item(0, 1)->text() == "尚未记录错误")
    {
        m_page->m_errorTable->setRowCount(0);
    }

    int row = m_page->m_errorTable->rowCount();
    m_page->m_errorTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    bool hexMode = m_page->m_gpibHexSendCheckBox && m_page->m_gpibHexSendCheckBox->isChecked();

    m_page->m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_page->m_errorSeq)));
    m_page->m_errorTable->setItem(row, 1, new QTableWidgetItem(timeStr));
    m_page->m_errorTable->setItem(row, 2, new QTableWidgetItem("超时"));
    m_page->m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_page->m_errorTable->setItem(row, 4, new QTableWidgetItem(bytesToDisplayText(expected, hexMode)));
    m_page->m_errorTable->setItem(row, 5, new QTableWidgetItem("(无返回)"));

    for (int c = 0; c < 6; ++c) {
        QTableWidgetItem* it = m_page->m_errorTable->item(row, c);
        if (it) it->setForeground(QColor("#f39c12"));
    }

    while (m_page->m_errorTable->rowCount() > 1000)
        m_page->m_errorTable->removeRow(0);

    if (m_page->m_errorAutoScroll && m_page->m_errorAutoScroll->getIsToggled())
        m_page->m_errorTable->scrollToBottom();
}

void GPIBErrorHandler::addContentError(const QString &command,
                                        const QByteArray &expected,
                                        const QByteArray &actual)
{
    ++m_page->m_errorSeq;
    ++m_page->m_contentCount;

    int total = m_page->m_timeoutCount + m_page->m_contentCount;
    m_page->m_errorTotalCard->setValue(QString::number(total));
    m_page->m_errorContentCard->setValue(QString::number(m_page->m_contentCount));

    if (m_page->m_errorTable->rowCount() == 1
        && m_page->m_errorTable->item(0, 1)
        && m_page->m_errorTable->item(0, 1)->text() == "尚未记录错误")
    {
        m_page->m_errorTable->setRowCount(0);
    }

    int row = m_page->m_errorTable->rowCount();
    m_page->m_errorTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    bool hexMode = m_page->m_gpibHexSendCheckBox && m_page->m_gpibHexSendCheckBox->isChecked();

    m_page->m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_page->m_errorSeq)));
    m_page->m_errorTable->setItem(row, 1, new QTableWidgetItem(timeStr));
    m_page->m_errorTable->setItem(row, 2, new QTableWidgetItem("内容错误"));
    m_page->m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_page->m_errorTable->setItem(row, 4, new QTableWidgetItem(bytesToDisplayText(expected, hexMode)));
    m_page->m_errorTable->setItem(row, 5, new QTableWidgetItem(bytesToDisplayText(actual,   hexMode)));

    for (int c = 0; c < 6; ++c) {
        QTableWidgetItem* it = m_page->m_errorTable->item(row, c);
        if (it) it->setForeground(QColor("#e74c3c"));
    }

    while (m_page->m_errorTable->rowCount() > 1000)
        m_page->m_errorTable->removeRow(0);

    if (m_page->m_errorAutoScroll && m_page->m_errorAutoScroll->getIsToggled())
        m_page->m_errorTable->scrollToBottom();
}

void GPIBErrorHandler::clearErrors()
{
    m_page->m_errorSeq     = 0;
    m_page->m_timeoutCount = 0;
    m_page->m_contentCount = 0;
    m_page->m_errorTotalCard->setValue("0");
    m_page->m_errorTimeoutCard->setValue("0");
    m_page->m_errorContentCard->setValue("0");
    m_page->m_errorTable->clearContents();
    m_page->m_errorTable->setRowCount(1);
    m_page->m_errorTable->setItem(0, 0, new QTableWidgetItem("—"));
    m_page->m_errorTable->setItem(0, 1, new QTableWidgetItem("尚未记录错误"));
    m_page->m_errorTable->setSpan(0, 1, 1, 5);
}
