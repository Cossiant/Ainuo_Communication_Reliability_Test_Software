#include "saveworker.h"

saveworker::saveworker(QObject *parent):QObject(parent), m_isOpen(false)
{
    // 注意：不要在构造函数中打开文件，因为线程尚未启动
}
saveworker::~saveworker(){
    if (m_isOpen) {
        m_file.close();
    }
}

void saveworker::initWriteFile(const QString &baseDir){
    QMutexLocker locker(&m_mutex); // 确保线程安全

    if (m_isOpen) {
        qDebug()<<"输出文件已经打开，请先关闭。";
        return;
    }

    // 生成时间戳文件名，精确到秒，例如 "savedata_20260416_153045.txt"
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString fileName = QString("savedata_%1.txt").arg(timestamp);

    QString dirPath = baseDir;
    if (dirPath.isEmpty()) {
        // 默认保存在应用程序当前目录下的 savedata 文件夹
        dirPath = QDir::currentPath() + "/savedata";
    }

    // 确保目录存在
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug()<<"无法创建输出文件目录："<<dirPath;
            return;
        }
    }

    QString fullPath = dir.filePath(fileName);
    m_file.setFileName(fullPath);

    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug()<<"无法打开输出文件：" << fullPath << "错误：" << m_file.errorString();
        return;
    }

    m_stream.setDevice(&m_file);
    m_stream.setCodec("UTF-8");
    m_stream.setGenerateByteOrderMark(true);  // 写入 BOM 以便记事本识别 UTF-8

    m_isOpen = true;

    //通知GUI文件已经创建好了
    //    emit logFileCreated(fullPath);

    // 可选：写入文件头信息
//    QString header = QString("===== 导出数据开始 [%1] =====\n")
//            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
//    m_stream << header;
    m_stream.flush();
}

void saveworker::writeFile(const QString &text){
    QMutexLocker locker(&m_mutex);

    if (!m_isOpen) {
        qWarning() << "writeFile: 文件未打开，无法写入。";
        return;
    }
    m_stream << text << "\n";
    m_stream.flush();  // 立即刷新到磁盘，确保数据不丢失
}

void saveworker::closeWriteFile(){
    QMutexLocker locker(&m_mutex);

    if (m_isOpen) {
        // 写入结束标记
        QString footer = QString("===== 日志结束 [%1] =====\n\n")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        m_stream << footer;
        m_stream.flush();
        m_file.close();
        m_isOpen = false;
        qDebug() << "LogWriter: 日志文件已关闭。";
    }
}

