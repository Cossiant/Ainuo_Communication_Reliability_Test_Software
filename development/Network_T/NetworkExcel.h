// NetworkExcel.h
// ★ 重构：使用公共 RangeComparer / StickySplitter
// ★ 新增：代际标记防止信号串扰

#pragma once

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include <QQueue>

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
    void onTrySendNext();
    void onResponseReceived(QByteArray data);
    void onInterCmdDelayFinished(int generation);       // ★ 修改：接收代际
    void onGlobalTimeout();

private:
    bool loadExcelToTable(const QString &filePath);
    bool generateExcelTemplate(const QString &filePath);
    void setRunning(bool running);
    void finalizeAndNext();
    void fillCaptureResult(const QByteArray &data);
    void fillCaptureTimeout();

    // ★ 区间判断辅助（使用公共 RangeComparer）
    bool tryRangeCompare(const QByteArray &received,
                         const QByteArray &expected,
                         bool hexMode) const;

    NetworkPage* m_page = nullptr;
    NetworkWork* m_work = nullptr;

    bool    m_isRunning     = false;
    int     m_currentRow    = 0;
    int     m_repeatLeft    = 0;
    int     m_totalSent     = 0;
    bool    m_pendingStop   = false;

    bool    m_isCaptureMode = false;

    bool      m_waiting     = false;
    bool      m_gotReply    = false;
    QByteArray m_lastRecvData;
    QString    m_lastCmd;
    QByteArray m_expectData;
    bool      m_minDelayOk  = false;

    QQueue<QByteArray> m_stickyQueue;
    QByteArray stickyDelimiter() const;

    QTimer* m_timeoutTimer = nullptr;

    // ★ 代际标记：每发送一条命令自增，用于校验延迟信号是否属于当前命令
    int        m_cmdGeneration = 0;
};
