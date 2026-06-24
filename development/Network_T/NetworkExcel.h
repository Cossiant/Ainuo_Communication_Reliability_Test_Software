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
    void onTrySendNext();
    void onResponseReceived(QByteArray data);
    void onInterCmdDelayFinished();
    void onGlobalTimeout();

private:
    bool loadExcelToTable(const QString &filePath);
    bool generateExcelTemplate(const QString &filePath);
    void setRunning(bool running);
    void finalizeAndNext();
    // ★ 捕获模式：将返回值填入表格 B 列
    void fillCaptureResult(const QByteArray &data);
    void fillCaptureTimeout();

    NetworkPage* m_page = nullptr;
    NetworkWork* m_work = nullptr;

    bool    m_isRunning     = false;
    int     m_currentRow    = 0;
    int     m_repeatLeft    = 0;
    int     m_totalSent     = 0;
    bool    m_pendingStop   = false;

    // ★ 捕获模式标志
    bool    m_isCaptureMode = false;

    // 等待回复状态
    bool      m_waiting     = false;
    bool      m_gotReply    = false;
    QByteArray m_lastRecvData;
    QString    m_lastCmd;
    QByteArray m_expectData;
    bool      m_minDelayOk  = false;

    QTimer* m_timeoutTimer = nullptr;
};
