#pragma once

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include <QQueue>

class SerialPage;
class SerialWork;

class SerialExcel : public QObject {
    Q_OBJECT
public:
    explicit SerialExcel(SerialPage* page, QObject *parent = nullptr);
    ~SerialExcel();

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
    // ★ 捕获模式：将返回值填入表格 B 列
    void fillCaptureResult(const QByteArray &data);
    void fillCaptureTimeout();

    SerialPage* m_page = nullptr;
    SerialWork* m_work = nullptr;

    bool    m_isRunning     = false;
    int     m_currentRow    = 0;
    int     m_repeatLeft    = 0;        // 剩余条数，-1=无限
    int     m_totalSent     = 0;
    bool    m_pendingStop   = false;    // 当前是最后一条

    // ★ 捕获模式标志
    bool    m_isCaptureMode = false;

    // 等待回复状态
    bool       m_waiting     = false;
    bool       m_gotReply    = false;
    QByteArray m_lastRecvData;
    QString    m_lastCmd;
    QByteArray m_expectData;
    bool       m_minDelayOk  = false;   // ★ 由 interCmdDelayFinished 设置
    // ★ 粘包分割队列
    QQueue<QByteArray> m_stickyQueue;
    QByteArray stickyDelimiter() const;

    QTimer* m_timeoutTimer = nullptr;   // 全局超时（仍在主线程）
};
