#ifndef SAVEWORKER_H
#define SAVEWORKER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QTableWidget>
#include <QTimer>

class saveworker:public QObject
{
    Q_OBJECT
public:
    explicit saveworker(QObject *parent = nullptr);
    ~saveworker();
public slots:
    // 初始化：创建以当前时间命名的文件，并打开准备写入
    void initWriteFile(const QString &baseDir = QString());

    // 写入一行文本（自动添加时间戳前缀）
    void writeFile(const QString &text);

    // 关闭文件并清理资源（由外部调用或线程结束时调用）
    void closeWriteFile();

    //写入到table的第二列当中（不需要加时间戳，只需要将数据写到table当中即可）
    void writeDataToTable(QTableWidget *excelTable);
    // 新增：设置待写入表格的数据（在调用 writeDataToTable 之前使用）
    void setPendingTableData(const QString &data);
    // 新增：重置写入位置（例如在开始新一轮发送前调用）
    void resetTableWritePosition();

private:
    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;          // 保护多线程写入安全（如果需要）
    bool m_isOpen;

    int m_tableWriteRow = 0;      // 当前应写入的行号（从0开始）
    QString m_pendingTableData;   // 暂存将要写入table的数据
};

#endif // SAVEWORKER_H
