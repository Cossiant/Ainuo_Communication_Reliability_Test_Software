#pragma once

#include <QString>

class QWidget;
class QTableWidget;

namespace ErrorLogExporter {

// 将错误统计表格导出为独立的 .xlsx 文件。
// 返回 true 表示已成功生成文件；用户取消或导出失败时返回 false。
bool exportToExcel(QWidget *parent,
                   const QString &dialogTitle,
                   const QString &defaultFileName,
                   QTableWidget *table);

} // namespace ErrorLogExporter
