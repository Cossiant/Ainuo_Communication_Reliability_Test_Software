//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>          // ← 改为继承 QObject
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QSerialPortInfo>
#include <QGroupBox>
#include <QHBoxLayout>          // ← 新增
#include <QListWidget>          // ← 新增
#include <QTableWidget>
#include <QDateTime>
#include <QTableWidget>
#include <QByteArray>
#include <QHeaderView>
#include <QThread>
#include <QDebug>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaToggleSwitch.h"
#include "../Other_T/LED.h"
#include "../Other_T/StatCard.h"
#include "SerialExcel.h"
#include "SerialWork.h"

class SerialWork;
class QTableWidget;
class QListWidget;
class StatCard;
class ElaWindow;            // ← 前向声明，不再 include 头文件
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaCheckBox;
class ElaLineEdit;
class ElaToggleSwitch;

#ifndef UNTITLED_SERIALPAGE_H
#define UNTITLED_SERIALPAGE_H

class SerialPage : public QObject {
    Q_OBJECT
    friend class SerialWork;
    friend class SerialExcel;
public:
    explicit SerialPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~SerialPage();
private:
    ElaWindow* m_mainWindow;
    SerialWork* m_serialWork = nullptr; // ← 内部持有 SerialWork
    SerialExcel* m_serialFunc = nullptr; // ← 内部持有 SerialFunction

    QThread*     m_serialThread  = nullptr;   // ★ 工作线程
    // ═════════════ 页面 ═════════
    QWidget *_SerialSettingPage = nullptr;
    QWidget *_SerialSendPage = nullptr;
    QWidget *_SerialExcelSendPage = nullptr;
    QWidget *_SerialLogPage = nullptr;
    QWidget *_SerialErrorLogPage = nullptr;

    // ========== 分组 Key ==========
    QString SerialMainPageKey;

    // ═════════════ 串口设置控件 ═════════
    ElaComboBox*  m_serialPortComboBox = nullptr;
    ElaComboBox*  m_baudRateComboBox   = nullptr;
    ElaComboBox*  m_dataBitsComboBox   = nullptr;
    ElaComboBox*  m_stopBitsComboBox   = nullptr;
    ElaComboBox*  m_parityComboBox     = nullptr;
    ElaCheckBox*  m_serialBufferCheckBox = nullptr;
    ElaCheckBox*  m_serialHexSendCheckBox  = nullptr;
    ElaPushButton* m_openSerialButton  = nullptr;
    ElaPushButton* m_closeSerialButton = nullptr;
    QLabel*        m_serialLED         = nullptr;

    // ═════════════ 单条发送控件 ═════════
    ElaLineEdit*   m_singleSendInput   = nullptr;
    ElaPushButton* m_singleSendBtn     = nullptr;
    QListWidget*   m_singleSendLog     = nullptr;
    QListWidget*   m_singleRecvLog     = nullptr;
    ElaPushButton* m_singleSendClearBtn = nullptr;

    // ═════════════ Excel 表格发送控件 ═════════
    ElaPushButton* m_excelOpenBtn        = nullptr;
    ElaPushButton* m_excelDownloadTplBtn = nullptr;
    ElaPushButton* m_excelSendBtn        = nullptr;
    ElaPushButton* m_excelStopBtn        = nullptr;
    ElaPushButton* m_excelCaptureBtn = nullptr;   // 读取返回值（预扫）
    ElaLineEdit*   m_excelRepeatCount = nullptr;  // ← 新增：发送次数
    ElaLineEdit*   m_excelTimeoutMs   = nullptr;
    QTableWidget*  m_excelTableWidget    = nullptr;

    // ═════════════ 发送日志控件 ═════════
    StatCard*     m_logSentCountCard = nullptr;   // 总计发送
    StatCard*     m_logRecvCountCard = nullptr;   // 总计接收
    StatCard*     m_logStartTimeCard = nullptr;   // 开始时间
    QListWidget*  m_logSendList      = nullptr;   // 发送日志（左）
    QListWidget*  m_logRecvList      = nullptr;   // 接收日志（右）
    ElaPushButton* m_logClearBtn     = nullptr;   // 清空日志

    // ═════════════ 错误统计 ═════════
    int m_errorSeq        = 0;
    int m_timeoutCount    = 0;
    int m_contentCount    = 0;

    // ═════════════ 错误日志控件 ═════════
    StatCard*        m_errorTotalCard   = nullptr;   // ← 改
    StatCard*        m_errorTimeoutCard = nullptr;   // ← 改
    StatCard*        m_errorContentCard = nullptr;   // ← 改
    QTableWidget*    m_errorTable        = nullptr;
    ElaPushButton*   m_errorClearBtn     = nullptr;
    ElaToggleSwitch* m_errorAutoScroll   = nullptr;

    // ═════════════ 初始化方法 ═════════
    void initSerialPage();
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


#endif //UNTITLED_SERIALPAGE_H
