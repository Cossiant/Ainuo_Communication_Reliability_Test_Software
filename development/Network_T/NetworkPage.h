// NetworkPage.h
// 页面协调器：持有各子模块指针，串联初始化流程
//
// Created by Cossiant on 2026/6/18.
//
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
#include "NetworkExcel.h"
#include "NetworkWork.h"

class NetworkWork;
class NetworkPageUI;
class NetworkPageSignals;
class NetworkErrorHandler;
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

#ifndef UNTITLED_NETWORKPAGE_H
#define UNTITLED_NETWORKPAGE_H

class NetworkPage : public QObject {
    Q_OBJECT
    friend class NetworkWork;
    friend class NetworkExcel;
    friend class NetworkPageUI;
    friend class NetworkPageSignals;
    friend class NetworkErrorHandler;
public:
    explicit NetworkPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~NetworkPage();
private:
    ElaWindow* m_mainWindow;

    // ★ 子模块
    NetworkPageUI*        m_ui        = nullptr;
    NetworkPageSignals*   m_signals   = nullptr;
    NetworkErrorHandler*  m_errors    = nullptr;
    NetworkWork*          m_networkWork  = nullptr;
    NetworkExcel*         m_networkFunc  = nullptr;

    QThread*      m_networkThread = nullptr;
    // ═════════════ 页面 ═════════
    QWidget *_NetworkSettingPage   = nullptr;
    QWidget *_NetworkSendPage      = nullptr;
    QWidget *_NetworkExcelSendPage = nullptr;
    QWidget *_NetworkLogPage       = nullptr;
    QWidget *_NetworkErrorLogPage  = nullptr;

    // ========== 分组 Key ==========
    QString NetworkMainPageKey;

    // ═════════════ 网络设置控件 ═════════
    ElaLineEdit*   m_ipAddressEdit          = nullptr;
    ElaLineEdit*   m_portEdit               = nullptr;
    ElaCheckBox*   m_networkHexSendCheckBox  = nullptr;
    ElaCheckBox*   m_nagleCheckBox           = nullptr;
    ElaCheckBox*   m_networkStripCRLFCheckBox = nullptr;
    ElaPushButton* m_openNetworkButton       = nullptr;
    ElaPushButton* m_closeNetworkButton      = nullptr;
    QLabel*        m_networkLED              = nullptr;
    ElaComboBox*   m_suffixComboBox          = nullptr;

    // 粘包分割
    ElaCheckBox*   m_networkSplitStickyCheckBox    = nullptr;
    ElaComboBox*   m_networkSplitDelimiterComboBox = nullptr;

    // 区间判断控件
    ElaCheckBox*   m_networkAsciiRangeCheckBox = nullptr;
    ElaCheckBox*   m_networkHexRangeCheckBox   = nullptr;
    ElaLineEdit*   m_networkAsciiRangeEdit     = nullptr;
    ElaLineEdit*   m_networkHexRangeEdit       = nullptr;
    // AN3.0 产品系列选择
    ElaComboBox*   m_networkProductComboBox = nullptr;


    // 连接超时定时器（主线程，3 秒）
    QTimer*        m_connectTimeoutTimer     = nullptr;
    bool           m_isConnecting            = false;

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
    void initNetworkPage();
    void initNavigation();
    void initWindowConfig();

    // ═════════════ 错误记录（委托给 NetworkErrorHandler）═════════
    void addTimeoutError(const QString &command, const QByteArray &expected);
    void addContentError(const QString &command, const QByteArray &expected, const QByteArray &actual);
    void clearErrors();
    void clearSingleSendLog();
    void clearExcelSendLog();
};

#endif //UNTITLED_NETWORKPAGE_H
