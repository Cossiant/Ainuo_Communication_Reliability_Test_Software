#ifndef READEXCELDATA_H
#define READEXCELDATA_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QTableWidget>
#include <QWidget>

class ExcelReader
{
public:
    // 静态方法：读取 Excel 文件并填充到 QTableWidget
    // 参数：filePath - Excel 文件路径
    //       table - 目标表格控件
    //       parent - 用于显示消息框的父窗口（可选，默认为 nullptr）
    // 返回值：成功返回 true，失败返回 false
    bool loadExcelToTable(const QString &filePath, QTableWidget *table, QWidget *parent = nullptr);
    int totalRows;
};

#endif // READEXCELDATA_H
