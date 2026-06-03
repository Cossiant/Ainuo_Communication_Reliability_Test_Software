#pragma once

#include "ElaWindow.h"
#include "ElaToggleSwitch.h"

#include <QFileDialog>
#include <QCloseEvent>

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
class ElaToggleSwitch;

class ElaWidgetToolsDemo : public ElaWindow
{
    Q_OBJECT

public:
    ElaWidgetToolsDemo(QWidget *parent = nullptr);
    ~ElaWidgetToolsDemo();

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    // ═════════════ 页面 ═════════
    QWidget* _dataPage;
    QWidget* _settingsPage;
    QWidget* _aboutPage;
    QWidget* _debugPage;
    QWidget* _singleSendPage;

    // ═════════════ 子线程 ═════════
    QThread *m_networkThread;
    QThread *m_excelSendThread;
    QThread *m_serialThread;
    QThread *m_savedataThread;
    QThread *m_validatorThread;

    // ═════════════ 核心 Worker ═════════
    QTableWidget *m_excelTableWidget;
    QListWidget *m_receiveListWidget;
    QListWidget *m_sendListWidget;
    ExcelSendWorker *m_excelSendWorker = nullptr;
    NetworkClient *m_networkClient = nullptr;
    SerialWorker *m_serialWorker = nullptr;
    saveworker *m_savedataWorker = nullptr;
    ResponseValidator *m_responseValidator = nullptr;
    ExcelReader *m_excelReader = nullptr;

    // ═════════════ 数据变量 ═════════
    QQueue<QByteArray> m_expectedResponseQueue;
    int m_errorCount = 0;
    int m_errorTimeOut = 0;
    int m_maxDisplayItems = 300;
    bool m_sendAndReadRecordBool = false;

    enum class ConnectionType { None, Network, Serial };
    ConnectionType m_currentConnectionType = ConnectionType::None;

    QFileDialog m_fileDialog;

    bool m_debugMode = false;

    // ═════════════ LED ═════════
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

    // ═════════════ 单条发送 → ElaLineEdit ═════════
    ElaLineEdit *m_singleSendInput = nullptr;

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

    // ═════════════ 单条发送按钮 ═════════
    ElaPushButton *m_singleSendNetBtn = nullptr;
    ElaPushButton *m_singleSendSerialBtn = nullptr;
    ElaPushButton *m_singleSendClearBtn = nullptr;

    // ═════════════ 单条发送日志 ═════════
    QListWidget *m_singleSendLog = nullptr;
    QListWidget *m_singleRecvLog = nullptr;

    // ═════════════ 初始化方法 ═════════
    void initWindow();
    void initPages();
    void initNavigation();
    void initWindowConfig();
    void createDataPage();
    void createSettingsPage();
    void createSingleSendPage();
    void createDebugPage();
    void applyDebugMode(bool enabled);

    // ═════════════ 辅助方法 ═════════
    static QString toHexDisplay(const QByteArray &data);
    QString currentTimeString() const;
    void setButtonsForNetworkMode();
    void setButtonsForSerialMode();

    // ═════════════ 信号 ═════════
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

    // ═════════════ 槽函数 ═════════
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

    void onSingleSendNetwork();
    void onSingleSendSerial();
};
