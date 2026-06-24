#ifndef READEXCELDATA_H
#define READEXCELDATA_H

#include <QString>
#include <QTableWidget>
#include <QWidget>

class ExcelReader
{
public:
    // 读取 Excel 文件到 QTableWidget（用 QXlsx，无需安装 Excel/WPS）
    bool loadExcelToTable(const QString &filePath,
                          QTableWidget *table,
                          QWidget *parent = nullptr);

    int totalRows = 0;   // 读取的总行数（公开，供发送线程使用）
};

#endif // READEXCELDATA_H
