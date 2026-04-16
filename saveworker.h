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
private:
    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;          // 保护多线程写入安全（如果需要）
    bool m_isOpen;
};

#endif // SAVEWORKER_H
