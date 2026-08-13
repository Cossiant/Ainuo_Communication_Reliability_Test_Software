#include "ErrorLogExporter.h"

#include "xlsxdocument.h"
#include "xlsxformat.h"

#include <QColor>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace ErrorLogExporter {

bool exportToExcel(QWidget *parent,
                   const QString &dialogTitle,
                   const QString &defaultFileName,
                   QTableWidget *table)
{
    if (!table) {
        return false;
    }

    const int rowCount = table->rowCount();
    const QString placeholder = QStringLiteral("尚未记录错误");

    // 空表格会保留一行占位提示，识别它并视为“没有数据”。
    const bool onlyPlaceholder = (rowCount == 1)
            && table->item(0, 1)
            && table->item(0, 1)->text() == placeholder;

    if (rowCount == 0 || onlyPlaceholder) {
        QMessageBox::information(parent,
                                 QStringLiteral("导出错误统计"),
                                 QStringLiteral("当前没有可导出的错误数据。"));
        return false;
    }

    QString defaultDir =
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        dialogTitle,
        QDir(defaultDir).filePath(defaultFileName),
        QStringLiteral("Excel 文件 (*.xlsx)"));

    if (filePath.isEmpty()) {
        return false;
    }

    if (!filePath.endsWith(QStringLiteral(".xlsx"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".xlsx");
    }

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
    cellFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    cellFormat.setBorderStyle(QXlsx::Format::BorderThin);

    const int columnCount = table->columnCount();
    for (int col = 0; col < columnCount; ++col) {
        const QString header = table->horizontalHeaderItem(col)
                ? table->horizontalHeaderItem(col)->text()
                : QStringLiteral("列%1").arg(col + 1);
        xlsx.write(1, col + 1, header, headerFormat);

        const double width = table->columnWidth(col) / 7.0;
        xlsx.setColumnWidth(col + 1, qMax(width, 10.0));
    }

    int outRow = 1;
    for (int row = 0; row < rowCount; ++row) {
        if (onlyPlaceholder && row == 0) {
            continue;
        }
        ++outRow;
        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem *item = table->item(row, col);
            xlsx.write(outRow, col + 1, item ? item->text() : QString(), cellFormat);
        }
    }

    if (!xlsx.saveAs(filePath)) {
        QMessageBox::critical(parent,
                              QStringLiteral("导出错误统计"),
                              QStringLiteral("保存 Excel 文件失败：\n%1").arg(filePath));
        return false;
    }

    QMessageBox::information(parent,
                             QStringLiteral("导出错误统计"),
                             QStringLiteral("错误数据已导出到：\n%1").arg(filePath));
    return true;
}

} // namespace ErrorLogExporter
