#pragma once

#include <QObject>
#include <QTimer>
#include <QByteArray>

class CANPage;
class CANWork;

class CANExcel : public QObject
{
    Q_OBJECT

public:
    explicit CANExcel(CANPage *page, QObject *parent = nullptr);
    ~CANExcel();

private slots:
    void onOpenExcel();
    void onDownloadTemplate();
    void onCapture();
    void onStartSend();
    void onStopSend();
    void onTrySendNext();
    void onResponseReceived(QByteArray frame);
    void onCommandWritten(int generation);
    void onGlobalTimeout();

private:
    bool loadExcelToTable(const QString &filePath);
    bool generateExcelTemplate(const QString &filePath);
    void setRunning(bool running);
    void finalizeAndNext();
    void fillCaptureResult(const QByteArray &frame);
    void fillCaptureTimeout();

    CANPage *m_page = nullptr;
    CANWork *m_work = nullptr;

    bool m_isRunning = false;
    int  m_currentRow = 0;
    int  m_repeatLeft = 0;
    int  m_totalSent = 0;
    bool m_pendingStop = false;
    bool m_isCaptureMode = false;

    bool m_waiting = false;
    bool m_gotReply = false;
    QByteArray m_lastRecvFrame;
    QString m_lastCmd;
    QByteArray m_expectFrame;
    int m_currentTimeoutMs = 500;

    QTimer *m_timeoutTimer = nullptr;
    int m_cmdGeneration = 0;
};
