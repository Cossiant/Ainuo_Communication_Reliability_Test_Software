#pragma once

#include <QObject>
#include <QTimer>
#include <QByteArray>

class NetworkPage;
class NetworkWork;

class NetworkExcel : public QObject {
    Q_OBJECT
public:
    explicit NetworkExcel(NetworkPage* page, QObject *parent = nullptr);
    ~NetworkExcel();

private slots:
    void onOpenExcel();
    void onDownloadTemplate();
    void onCapture();
    void onStartSend();
    void onStopSend();
    void onTrySendNext();               // 尝试发下一条（统一入口）
    void onResponseReceived(QByteArray data);
    void onInterCmdDelayFinished();     // ★ 工作线程精确延时到期
    void onGlobalTimeout();

private:
    bool loadExcelToTable(const QString &filePath);
    bool generateExcelTemplate(const QString &filePath);
    void setRunning(bool running);
    void finalizeAndNext();             // 结算当前等待 → 发下一条

    NetworkPage* m_page = nullptr;
    NetworkWork* m_work = nullptr;

    bool    m_isRunning    = false;
    int     m_currentRow   = 0;
    int     m_repeatLeft   = 0;         // 剩余条数，-1=无限
    int     m_totalSent    = 0;
    bool    m_pendingStop  = false;     // 当前是最后一条

    // 等待回复状态
    bool      m_waiting     = false;
    bool      m_gotReply    = false;
    QByteArray m_lastRecvData;
    QString    m_lastCmd;
    QByteArray m_expectData;
    bool      m_minDelayOk  = false;    // ★ 由 interCmdDelayFinished 设置

    QTimer* m_timeoutTimer = nullptr;   // 全局超时（仍在主线程）
};
