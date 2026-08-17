#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QThread>
#include <QDateTime>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaToggleSwitch.h"
#include "../Other_T/LED.h"
#include "../Other_T/StatCard.h"

class CANWork;
class CANExcel;
class CANPageUI;
class CANPageSignals;
class CANErrorHandler;
class ElaWindow;
class ElaText;

class CANPage : public QObject
{
    Q_OBJECT

    friend class CANWork;
    friend class CANExcel;
    friend class CANPageUI;
    friend class CANPageSignals;
    friend class CANErrorHandler;

public:
    explicit CANPage(ElaWindow *mainWindow, QObject *parent = nullptr);
    ~CANPage();

private:
    void initNavigation();
    void initWindowConfig();

    void addTimeoutError(const QString &command, const QString &expected);
    void addContentError(const QString &command,
                         const QString &expected,
                         const QString &actual);
    void clearErrors();
    void clearSingleSendLog();
    void clearExcelSendLog();
    void exportErrorsToExcel();

    ElaWindow *m_mainWindow = nullptr;

    CANPageUI *m_ui = nullptr;
    CANPageSignals *m_signals = nullptr;
    CANErrorHandler *m_errors = nullptr;
    CANWork *m_canWork = nullptr;
    CANExcel *m_canFunc = nullptr;
    QThread *m_canThread = nullptr;

    QWidget *_CANSettingPage = nullptr;
    QWidget *_CANSendPage = nullptr;
    QWidget *_CANExcelSendPage = nullptr;
    QWidget *_CANLogPage = nullptr;
    QWidget *_CANErrorLogPage = nullptr;
    QString CANMainPageKey;

    // 设置页
    ElaLineEdit *m_deviceTypeEdit = nullptr;
    ElaLineEdit *m_deviceIndexEdit = nullptr;
    ElaLineEdit *m_canIndexEdit = nullptr;
    ElaComboBox *m_canModeComboBox = nullptr;
    ElaComboBox *m_baudRateComboBox = nullptr;
    ElaComboBox *m_dbitBaudComboBox = nullptr;
    ElaCheckBox *m_terminalResCheckBox = nullptr;
    ElaPushButton *m_openButton = nullptr;
    ElaPushButton *m_closeButton = nullptr;
    QLabel *m_canLED = nullptr;

    // 单条发送页
    ElaLineEdit *m_singleSendInput = nullptr;
    ElaPushButton *m_singleSendBtn = nullptr;
    ElaPushButton *m_singleSendClearBtn = nullptr;
    ElaPushButton *m_singleLogPauseBtn = nullptr;
    QLabel *m_singleLogLED = nullptr;
    bool m_singleLogPaused = false;
    QListWidget *m_singleSendLog = nullptr;
    QListWidget *m_singleRecvLog = nullptr;

    // Excel 发送页
    ElaPushButton *m_excelOpenBtn = nullptr;
    ElaPushButton *m_excelDownloadTplBtn = nullptr;
    ElaPushButton *m_excelSendBtn = nullptr;
    ElaPushButton *m_excelStopBtn = nullptr;
    ElaPushButton *m_excelCaptureBtn = nullptr;
    ElaLineEdit *m_excelRepeatCount = nullptr;
    ElaLineEdit *m_excelTimeoutMs = nullptr;
    QTableWidget *m_excelTableWidget = nullptr;

    // 发送日志页
    StatCard *m_logSentCountCard = nullptr;
    StatCard *m_logRecvCountCard = nullptr;
    StatCard *m_logStartTimeCard = nullptr;
    QListWidget *m_logSendList = nullptr;
    QListWidget *m_logRecvList = nullptr;
    ElaPushButton *m_logClearBtn = nullptr;
    ElaPushButton *m_logPauseBtn = nullptr;
    QLabel *m_logLED = nullptr;
    bool m_logPaused = false;

    // 错误统计
    int m_errorSeq = 0;
    int m_timeoutCount = 0;
    int m_contentCount = 0;
    StatCard *m_errorTotalCard = nullptr;
    StatCard *m_errorTimeoutCard = nullptr;
    StatCard *m_errorContentCard = nullptr;
    QTableWidget *m_errorTable = nullptr;
    ElaPushButton *m_errorClearBtn = nullptr;
    ElaPushButton *m_errorExportBtn = nullptr;
    ElaToggleSwitch *m_errorAutoScroll = nullptr;
};
