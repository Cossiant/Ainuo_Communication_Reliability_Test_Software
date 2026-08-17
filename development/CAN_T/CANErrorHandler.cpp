#include "CANErrorHandler.h"

#include "CANPage.h"
#include "ElaToggleSwitch.h"

#include <QColor>
#include <QDateTime>
#include <QTableWidget>
#include <QTableWidgetItem>

CANErrorHandler::CANErrorHandler(CANPage *page)
    : QObject(page), m_page(page)
{
}

void CANErrorHandler::addTimeoutError(const QString &command, const QString &expected)
{
    ++m_page->m_errorSeq;
    ++m_page->m_timeoutCount;

    const int total = m_page->m_timeoutCount + m_page->m_contentCount;
    m_page->m_errorTotalCard->setValue(QString::number(total));
    m_page->m_errorTimeoutCard->setValue(QString::number(m_page->m_timeoutCount));

    if (m_page->m_errorTable->rowCount() == 1
        && m_page->m_errorTable->item(0, 1)
        && m_page->m_errorTable->item(0, 1)->text() == QStringLiteral("尚未记录错误")) {
        m_page->m_errorTable->setRowCount(0);
    }

    const int row = m_page->m_errorTable->rowCount();
    m_page->m_errorTable->insertRow(row);

    m_page->m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_page->m_errorSeq)));
    m_page->m_errorTable->setItem(row, 1, new QTableWidgetItem(
        QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));
    m_page->m_errorTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("超时")));
    m_page->m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_page->m_errorTable->setItem(row, 4, new QTableWidgetItem(expected));
    m_page->m_errorTable->setItem(row, 5, new QTableWidgetItem(QStringLiteral("(无返回)")));

    for (int c = 0; c < 6; ++c) {
        if (QTableWidgetItem *it = m_page->m_errorTable->item(row, c)) {
            it->setForeground(QColor("#f39c12"));
        }
    }

    while (m_page->m_errorTable->rowCount() > 1000) {
        m_page->m_errorTable->removeRow(0);
    }
    if (m_page->m_errorAutoScroll && m_page->m_errorAutoScroll->getIsToggled()) {
        m_page->m_errorTable->scrollToBottom();
    }
}

void CANErrorHandler::addContentError(const QString &command,
                                      const QString &expected,
                                      const QString &actual)
{
    ++m_page->m_errorSeq;
    ++m_page->m_contentCount;

    const int total = m_page->m_timeoutCount + m_page->m_contentCount;
    m_page->m_errorTotalCard->setValue(QString::number(total));
    m_page->m_errorContentCard->setValue(QString::number(m_page->m_contentCount));

    if (m_page->m_errorTable->rowCount() == 1
        && m_page->m_errorTable->item(0, 1)
        && m_page->m_errorTable->item(0, 1)->text() == QStringLiteral("尚未记录错误")) {
        m_page->m_errorTable->setRowCount(0);
    }

    const int row = m_page->m_errorTable->rowCount();
    m_page->m_errorTable->insertRow(row);

    m_page->m_errorTable->setItem(row, 0, new QTableWidgetItem(QString::number(m_page->m_errorSeq)));
    m_page->m_errorTable->setItem(row, 1, new QTableWidgetItem(
        QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));
    m_page->m_errorTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("内容错误")));
    m_page->m_errorTable->setItem(row, 3, new QTableWidgetItem(command));
    m_page->m_errorTable->setItem(row, 4, new QTableWidgetItem(expected));
    m_page->m_errorTable->setItem(row, 5, new QTableWidgetItem(actual));

    for (int c = 0; c < 6; ++c) {
        if (QTableWidgetItem *it = m_page->m_errorTable->item(row, c)) {
            it->setForeground(QColor("#e74c3c"));
        }
    }

    while (m_page->m_errorTable->rowCount() > 1000) {
        m_page->m_errorTable->removeRow(0);
    }
    if (m_page->m_errorAutoScroll && m_page->m_errorAutoScroll->getIsToggled()) {
        m_page->m_errorTable->scrollToBottom();
    }
}

void CANErrorHandler::clearErrors()
{
    m_page->m_errorSeq = 0;
    m_page->m_timeoutCount = 0;
    m_page->m_contentCount = 0;
    m_page->m_errorTotalCard->setValue("0");
    m_page->m_errorTimeoutCard->setValue("0");
    m_page->m_errorContentCard->setValue("0");
    m_page->m_errorTable->clearContents();
    m_page->m_errorTable->setRowCount(1);
    m_page->m_errorTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("—")));
    m_page->m_errorTable->setItem(0, 1, new QTableWidgetItem(QStringLiteral("尚未记录错误")));
    m_page->m_errorTable->setSpan(0, 1, 1, 5);
}
