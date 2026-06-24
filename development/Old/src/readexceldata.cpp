#include "../include/readexceldata.h"
#include "../../QXlsx/QXlsx/header/xlsxdocument.h"

#include <QTableWidgetItem>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

bool ExcelReader::loadExcelToTable(const QString &filePath,
                                    QTableWidget *table,
                                    QWidget *parent)
{
    qDebug() << "现在进入加载函数！";
    // ===== ① 参数检查 =====
    if (!table) {
        if (parent)
            QMessageBox::critical(parent, "错误", "目标表格控件为空");
        return false;
    }

    // ===== ② 检查文件是否存在 =====
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        if (parent)
            QMessageBox::critical(parent, "错误",
                QString("文件不存在:\n%1").arg(filePath));
        return false;
    }

    // ===== ③ 打开 xlsx 文件（构造函数自动加载） =====
    QXlsx::Document xlsx(filePath);

    // ===== ④ 检查加载是否成功 =====
    //   QXlsx 的 Document 没有 load() 方法！
    //   通过检查 dimension() 和试读 A1 单元格来判断
    QXlsx::CellRange range = xlsx.dimension();
    int rowCount = range.rowCount();

    if (rowCount <= 0) {
        if (parent)
            QMessageBox::critical(parent, "错误",
                QString("无法读取 Excel 文件:\n%1\n请确保文件是 .xlsx 格式且未被加密").arg(filePath));
        return false;
    }

    int colCount = range.columnCount();
    qDebug() << "检测到 Excel 总行数:" << rowCount << "总列数:" << colCount;
    totalRows = rowCount;

    // ===== ⑤ 设置表格 =====
    table->clear();
    table->setRowCount(rowCount);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"需发送的命令", "正确的返回值"});

    int nonEmptyCount = 0;

    // ===== ⑥ 逐行读取 =====
    for (int row = 1; row <= rowCount; ++row)
    {
        // 读取 A 列
        QVariant valA;
        if (colCount >= 1) {
            valA = xlsx.read(row, 1);
        }
        QString textA;
        if (valA.isValid() && !valA.isNull()) {
            textA = valA.toString().trimmed();
        }

        // 读取 B 列
        QVariant valB;
        if (colCount >= 2) {
            valB = xlsx.read(row, 2);
        }
        QString textB;
        if (valB.isValid() && !valB.isNull()) {
            textB = valB.toString().trimmed();
        }

        if (!textA.isEmpty()) nonEmptyCount++;

        // 写入表格（确保 cell 非空）
        table->setItem(row - 1, 0, new QTableWidgetItem(textA));
        table->setItem(row - 1, 1, new QTableWidgetItem(textB));

        if (row <= 5) {
            qDebug() << "第" << row << "行 A列:" << textA << " B列:" << textB;
        }
    }

    qDebug() << "共读取" << rowCount << "行，非空单元格数量:" << nonEmptyCount;

    return true;
}
