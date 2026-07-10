// GPIBPage.h
// GPIB 通讯页面：UI + 线程管理 + 信号连接
// 对齐 SerialPage / NetworkPage 架构

#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTableWidget>
#include <QDateTime>
#include <QByteArray>
#include <QHeaderView>
#include <QThread>
#include <QDebug>
#include <QTimer>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaToggleSwitch.h"
#include "../Other_T/LED.h"
#include "../Other_T/StatCard.h"
#include "GPIBExcel.h"
#include "GPIBWork.h"

class GPIBWork;
class QTableWidget;
class QListWidget;
class StatCard;
class ElaWindow;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaCheckBox;
class ElaLineEdit;
class ElaToggleSwitch;

#ifndef UNTITLED_GPIBPAGE_H
#define UNTITLED_GPIBPAGE_H

class GPIBPage : public QObject {
    Q_OBJECT
    friend class GPIBWork;
    friend class GPIBExcel;
public:
    explicit GPIBPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~GPIBPage();
private:
    ElaWindow* m_mainWindow;
    GPIBWork*  m_gpibWork  = nullptr;
    GPIBExcel* m_gpibFunc  = nullptr;

    QThread*   m_gpibThread = nullptr;

    // ═════════════ 页面 ═════════
    QWidget *_GpibSettingPage   = nullptr;
    QWidget *_GpibSendPage      = nullptr;
    QWidget *_GpibExcelSendPage = nullptr;
    QWidget *_GpibLogPage       = nullptr;
    QWidget *_GpibErrorLogPage  = nullptr;

    // ========== 分组 Key ==========
    QString GpibMainPageKey;

    // ═════════════ GPIB 设置控件 ═════════
    ElaLineEdit*   m_boardIndexEdit         = nullptr;
    ElaLineEdit*   m_primaryAddrEdit        = nullptr;
    ElaLineEdit*   m_secondaryAddrEdit      = nullptr;
    ElaLineEdit*   m_timeoutEdit            = nullptr;
    ElaLineEdit*   m_termCharEdit           = nullptr;
    ElaCheckBox*   m_termCharEnabledCheckBox = nullptr;
    ElaCheckBox*   m_sendEndEnabledCheckBox  = nullptr;
    ElaCheckBox*   m_gpibHexSendCheckBox     = nullptr;
    ElaCheckBox*   m_gpibStripCRLFCheckBox   = nullptr;
    ElaComboBox*   m_suffixComboBox         = nullptr;   //发送后缀选择
    ElaPushButton* m_openGpibButton         = nullptr;
    ElaPushButton* m_closeGpibButton        = nullptr;
    QLabel*        m_gpibLED                = nullptr;

    // ★ 连接超时定时器（主线程，5 秒，GPIB 连接可能较慢）
    QTimer*        m_connectTimeoutTimer    = nullptr;
    bool           m_isConnecting           = false;

    // ═════════════ 单条发送控件 ═════════
    ElaLineEdit*   m_singleSendInput      = nullptr;
    ElaPushButton* m_singleSendBtn        = nullptr;
    QListWidget*   m_singleSendLog        = nullptr;
    QListWidget*   m_singleRecvLog        = nullptr;
    ElaPushButton* m_singleSendClearBtn   = nullptr;

    // ═════════════ Excel 表格发送控件 ═════════
    ElaPushButton* m_excelOpenBtn         = nullptr;
    ElaPushButton* m_excelDownloadTplBtn  = nullptr;
    ElaPushButton* m_excelSendBtn         = nullptr;
    ElaPushButton* m_excelStopBtn         = nullptr;
    ElaPushButton* m_excelCaptureBtn      = nullptr;
    ElaLineEdit*   m_excelRepeatCount     = nullptr;
    ElaLineEdit*   m_excelTimeoutMs       = nullptr;
    QTableWidget*  m_excelTableWidget     = nullptr;

    // ═════════════ 发送日志控件 ═════════
    StatCard*      m_logSentCountCard = nullptr;
    StatCard*      m_logRecvCountCard = nullptr;
    StatCard*      m_logStartTimeCard = nullptr;
    QListWidget*   m_logSendList      = nullptr;
    QListWidget*   m_logRecvList      = nullptr;
    ElaPushButton* m_logClearBtn      = nullptr;
    ElaPushButton* m_logPauseBtn      = nullptr;
    QLabel*        m_logLED           = nullptr;
    bool           m_logPaused        = false;

    // ═════════════ 错误统计 ═════════
    int m_errorSeq     = 0;
    int m_timeoutCount = 0;
    int m_contentCount = 0;

    // ═════════════ 错误日志控件 ═════════
    StatCard*        m_errorTotalCard   = nullptr;
    StatCard*        m_errorTimeoutCard = nullptr;
    StatCard*        m_errorContentCard = nullptr;
    QTableWidget*    m_errorTable       = nullptr;
    ElaPushButton*   m_errorClearBtn    = nullptr;
    ElaToggleSwitch* m_errorAutoScroll  = nullptr;

    // ═════════════ 初始化方法 ═════════
    void initGpibPage();
    void initNavigation();
    void initwindowConfig();

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();

    // ═════════════ 错误记录 ═════════
    void addTimeoutError(const QString &command, const QByteArray &expected);
    void addContentError(const QString &command, const QByteArray &expected, const QByteArray &actual);
    void clearErrors();
    void clearSingleSendLog();
    void clearExcelSendLog();
};

#endif // UNTITLED_GPIBPAGE_H
