#include "readexceldata.h"
#include <QAxObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QWidget>
#include <QDebug>

bool readExcelData::loadExcelToTable(const QString &filePath, QTableWidget *table, QWidget *parent)
{
    if (!table) {
        if (parent)
            QMessageBox::critical(parent,("错误"), ("目标表格控件为空"));
        return false;
    }

    QAxObject excel("Excel.Application");
    if (excel.isNull()) {
        if (parent)
            QMessageBox::critical(parent,("错误"),("无法启动 Excel 应用程序，请确保已安装 Excel 或 WPS。"));
        return false;
    }

    excel.setProperty("Visible", false);
    excel.setProperty("DisplayAlerts", false);

    QAxObject *workbooks = excel.querySubObject("Workbooks");
    QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", filePath);
    if (!workbook) {
        if (parent)
            QMessageBox::critical(parent,("错误"), QString("无法打开 Excel 文件:\n%1").arg(filePath));
        excel.dynamicCall("Quit()");
        return false;
    }

    QAxObject *sheets = workbook->querySubObject("Worksheets");
    QAxObject *sheet = sheets->querySubObject("Item(int)", 1);
    if (!sheet) {
        if (parent)
            QMessageBox::critical(parent, ("错误"),("无法获取第一个工作表"));
        workbook->dynamicCall("Close()");
        excel.dynamicCall("Quit()");
        return false;
    }

    // 方法1：获取已使用区域的总行数
    QAxObject *usedRange = sheet->querySubObject("UsedRange");
    if (!usedRange) {
        if (parent)
            QMessageBox::critical(parent, ("错误"), ("无法获取已使用区域"));
        delete sheet;
        delete sheets;
        workbook->dynamicCall("Close()");
        excel.dynamicCall("Quit()");
        return false;
    }

    QAxObject *rows = usedRange->querySubObject("Rows");
    int totalRows = rows->property("Count").toInt();
    delete rows;
    delete usedRange;

    qDebug() << "检测到 Excel 总行数（UsedRange）:" << totalRows;
    ExceltotalRows = totalRows;

    if (totalRows == 0) {
        if (parent)
            QMessageBox::warning(parent, ("提示"), ("Excel 文件为空"));
        table->clear();
        table->setRowCount(0);
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels(QStringList() << ("需要发送的命令") << ("正确的返回值"));
        delete sheet;
        delete sheets;
        workbook->dynamicCall("Close()");
        excel.dynamicCall("Quit()");
        return false;
    }

    // 设置表格
    table->clear();
    table->setRowCount(totalRows);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << ("需要发送的命令") << ("正确的返回值"));

    int nonEmptyCount = 0;
    // 读取第一列（A列）数据
    for (int row = 1; row <= totalRows; ++row) {
        // 获取单元格对象
        QAxObject *cell = sheet->querySubObject("Cells(int,int)", row, 1);
        if (!cell) {
            qWarning() << "无法获取单元格" << row << ",1";
            delete cell;
            continue;
        }

        // 尝试多种方式获取单元格文本
        QString value;
        // 方式1: Value 属性
        QVariant var = cell->property("Value");
        if (!var.isNull()) {
            value = var.toString();
        }
        // 如果 value 为空，尝试 Text 属性（显示文本）
        if (value.isEmpty()) {
            QVariant textVar = cell->property("Text");
            if (!textVar.isNull()) {
                value = textVar.toString();
            }
        }
        // 如果还是空，尝试 Value2
        if (value.isEmpty()) {
            QVariant val2 = cell->property("Value2");
            if (!val2.isNull()) {
                value = val2.toString();
            }
        }

        // 调试输出前5行的内容
        if (row <= 5) {
            qDebug() << "第" << row << "行，读取到的值:" << value;
        }

        if (!value.isEmpty()) {
            nonEmptyCount++;
        }

        // 表格第1列（命令）填入行号
        QTableWidgetItem *cmdItem = new QTableWidgetItem(value);
        table->setItem(row - 1, 0, cmdItem);

        // 表格第2列（命令）填入读取到的值
        QTableWidgetItem *backItem = new QTableWidgetItem(value);
        table->setItem(row - 1, 1, backItem);

        delete cell;
    }

    qDebug() << "共读取" << totalRows << "行，非空单元格数量:" << nonEmptyCount;

    // 清理 COM 对象
    delete sheet;
    delete sheets;
    workbook->dynamicCall("Close()");
    delete workbook;
    delete workbooks;
    excel.dynamicCall("Quit()");

    if (nonEmptyCount == 0 && totalRows > 0) {
        if (parent) {
            QMessageBox::warning(parent,("警告"),("Excel 第一列所有单元格都为空或无法读取。\n请检查文件内容或格式。"));
        }
        return false;
    }

    return true;
}
