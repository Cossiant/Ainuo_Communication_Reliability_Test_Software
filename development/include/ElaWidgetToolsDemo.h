#pragma once

#include "ElaWindow.h"

// ========== 旧项目头文件（不变） ==========
#include <QHostAddress>
#include <QTableWidget>
#include <QListWidget>
#include <QQueue>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QGroupBox>
#include <QScrollArea>
#include <QTime>
#include <chrono>

#include "readexceldata.h"
#include "connectnetwork.h"
#include "excelsendworker.h"
#include "serialworker.h"
#include "saveworker.h"
#include "led.h"
#include "responsevalidator.h"

// ========== ElaWidgetTools 组件前向声明 ==========
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaCheckBox;
class ElaLineEdit;

class ElaWidgetToolsDemo : public ElaWindow
{
    Q_OBJECT

public:
    ElaWidgetToolsDemo(QWidget *parent = nullptr);
    ~ElaWidgetToolsDemo();

private:
    // ═════════════ 页面 ═════════
    QWidget* _dataPage;       // 数据收发页面
    QWidget* _settingsPage;   // 通讯设置页面
    QWidget* _aboutPage;

    // ═════════════ 子线程（不变） ═════════
    QThread *m_networkThread;
    QThread *m_excelSendThread;
    QThread *m_serialThread;
    QThread *m_savedataThread;
    QThread *m_validatorThread;

    // ═════════════ 核心 Worker / 控件（不变） ═════════
    QTableWidget *m_excelTableWidget;
    QListWidget *m_receiveListWidget;
    QListWidget *m_sendListWidget;
    ExcelReader *m_excelReader;
    ExcelSendWorker *m_excelSendWorker;
    NetworkClient *m_networkClient;
    SerialWorker *m_serialWorker;
    saveworker *m_savedataWorker;
    ResponseValidator *m_responseValidator;

    // ═════════════ 数据变量（不变） ═════════
    QQueue<QByteArray> m_expectedResponseQueue;
    int m_errorCount = 0;
    int m_errorTimeOut = 0;
    int m_maxDisplayItems = 300;
    bool m_sendAndReadRecordBool = false;

    enum class ConnectionType { None, Network, Serial };
    ConnectionType m_currentConnectionType = ConnectionType::None;

    QFileDialog m_fileDialog;

    // ═════════════ LED（保持原生 QLabel） ═════════
    QLabel *m_NetWorkLED;
    QLabel *m_SerialLED;

    // ═════════════ 标签 → ElaText ═════════
    ElaText *m_excelReadLabel;
    ElaText *m_receiveLabel;
    ElaText *m_sendLabel;
    ElaText *m_serverIpLabel;
    ElaText *m_portLabel;
    ElaText *m_delayLabel;
    ElaText *m_timeoutLabel;
    ElaText *m_sendLimitLabel;
    ElaText *m_sentCountLabel;
    ElaText *m_sentCountDisplayLabel;
    ElaText *m_serialPortLabel;
    ElaText *m_baudRateLabel;
    ElaText *m_dataBitsLabel;
    ElaText *m_stopBitsLabel;
    ElaText *m_parityLabel;
    ElaText *m_errorCountLabel;
    ElaText *m_errorCountDisplayLabel;
    ElaText *m_errorTimeOutLabel;
    ElaText *m_errorTimeOutDisplayLabel;
    ElaText *m_NetWorkLEDLabel;
    ElaText *m_SerialLEDLabel;
    ElaText *m_timePrecisionLabel;

    // ═════════════ 输入框 → ElaLineEdit ═════════
    ElaLineEdit *m_serverIpLineEdit;
    ElaLineEdit *m_portLineEdit;
    ElaLineEdit *m_delayLineEdit;
    ElaLineEdit *m_timeoutLineEdit;
    ElaLineEdit *m_sendLimitLineEdit;

    // ═════════════ 下拉框 → ElaComboBox ═════════
    ElaComboBox *m_serialPortComboBox;
    ElaComboBox *m_baudRateComboBox;
    ElaComboBox *m_dataBitsComboBox;
    ElaComboBox *m_stopBitsComboBox;
    ElaComboBox *m_parityComboBox;
    ElaComboBox *m_timePrecisionComboBox;

    // ═════════════ 勾选框 → ElaCheckBox ═════════
    ElaCheckBox *m_sendWithAN3CheckBox;
    ElaCheckBox *m_tcpNoDelayCheckBox;
    ElaCheckBox *m_testPacketLossCheckBox;
    ElaCheckBox *m_onlySendDataModeCheckBox;
    ElaCheckBox *m_serialBufferCheckBox;

    // ═════════════ 按钮 → ElaPushButton ═════════
    ElaPushButton *m_openExcelButton;
    ElaPushButton *m_sendExcelButton;
    ElaPushButton *m_stopSendExcelButton;
    ElaPushButton *m_sendSerialButton;
    ElaPushButton *m_stopSendSerialButton;
    ElaPushButton *m_disconnectButton;
    ElaPushButton *m_connectButton;
    ElaPushButton *m_openSerialButton;
    ElaPushButton *m_closeSerialButton;
    ElaPushButton *m_GUIClearButton;
    ElaPushButton *m_sendNetWorkAndReadRecordButton;
    ElaPushButton *m_sendSerialAndReadRecordButton;

    // ═════════════ 初始化方法 ═════════
    void initWindow();
    void initPages();
    void initNavigation();
    void initWindowConfig();
    void createDataPage();
    void createSettingsPage();

    // ═════════════ 辅助方法 ═════════
    static QString toHexDisplay(const QByteArray &data);
    QString currentTimeString() const;
    void setButtonsForNetworkMode();
    void setButtonsForSerialMode();

    // ═════════════ 信号（和原来一模一样） ═════════
signals:
    void requestNetworkConnect(int port, QHostAddress serverIP);
    void requestSendNetworkData(QByteArray msg, int delayMS = 0);
    void requestStartSend();
    void requestStopSend();

    void requestSerialOpen(const QString &portName, qint32 baudRate,
                           QSerialPort::DataBits dataBits, QSerialPort::Parity parity,
                           QSerialPort::StopBits stopBits, QSerialPort::FlowControl flowControl);
    void requestSerialClose();

    void requestInitSaveFile(const QString &baseDir);
    void requestWriteSaveFile(const QString &text);
    void requestCloseSaveFile();
    void requestResetTableWrite();

    void requestEnqueueExpected(QByteArray expectedResponse);
    void requestValidateData(QByteArray data);
    void requestResetValidator();
    void requestSetValidatorMode(bool testPacketLoss, bool onlySend);

    // ═════════════ 槽函数（和原来一模一样） ═════════
private slots:
    void onOpenExcelClicked();
    void onConnectServer();
    void onDisconnectServer();
    void successConnectServer();

    void onStartSendExcel(bool data);
    void onStopSendExcel();
    void onSendFinished();

    void onStartSendSerial(bool data);
    void onStopSendSerial();
    void onSerialSendFinished();

    void onCommandSent(int row, QByteArray expectedResponse);
    void onErrorDetected();
    void onErrorTimeOut();
    void updateValidatorMode();

    void onOpenSerial();
    void onCloseSerial();
    void onSerialClosed();

    void onDisplayReceivedData(QByteArray msg);
    void onDisplaySentData(QByteArray msg);
    void onUpdateSentCount(int count);

    void clearGUI();
    void resetTableForReadRecord();
    void sendNetWorkAndReadRecordSlot();
    void sendSerialAndReadRecordSlot();
};
