// GPIBExcel.h
// GPIB Excel 批量发送 / 捕获，对齐 SerialExcel / NetworkExcel
// ★ 重构：使用公共 RangeComparer

#pragma once

#include <QObject>
#include <QTimer>
#include <QByteArray>

class GPIBPage;
class GPIBWork;

class GPIBExcel : public QObject {
    Q_OBJECT
public:
    explicit GPIBExcel(GPIBPage* page, QObject *parent = nullptr);
    ~GPIBExcel();

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
    void fillCaptureResult(const QByteArray &data);
    void fillCaptureTimeout();

    // ★ 区间判断辅助（使用公共 RangeComparer）
    bool tryRangeCompare(const QByteArray &received,
                         const QByteArray &expected,
                         bool hexMode) const;

    GPIBPage* m_page = nullptr;
    GPIBWork* m_work = nullptr;

    bool    m_isRunning     = false;
    int     m_currentRow    = 0;
    int     m_repeatLeft    = 0;
    int     m_totalSent     = 0;
    bool    m_pendingStop   = false;

    bool    m_isCaptureMode = false;

    bool       m_waiting     = false;
    bool       m_gotReply    = false;
    QByteArray m_lastRecvData;
    QString    m_lastCmd;
    QByteArray m_expectData;
    bool       m_minDelayOk  = false;

    QTimer* m_timeoutTimer = nullptr;
};
