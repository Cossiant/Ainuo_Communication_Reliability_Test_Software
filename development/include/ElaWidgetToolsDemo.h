#pragma once

#include "ElaWindow.h"
#include "ElaToggleSwitch.h"
#include "ElaAcrylicUrlCard.h"

#include <QFileDialog>
#include <QCloseEvent>

#include "readexceldata.h"
#include "connectnetwork.h"
#include "excelsendworker.h"
#include "serialworker.h"
#include "saveworker.h"
#include "led.h"
#include "responsevalidator.h"
#include "StatCard.h"
#include "../Serial_T/SerialPage.h"
#include "../Network_T/NetworkPage.h"
#include "../CAN_T/CANPage.h"
#include "../GPIB_T/GPIBPage.h"
#include "../USER_T/USERPage.h"
#include "../Other_T/MainPage.h"

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
    SerialPage* m_serialPage = nullptr;
    NetworkPage* m_networkPage = nullptr;
    CANPage* m_CANPage = nullptr;
    GPIBPage* m_GPIBPage = nullptr;
    USERPage* m_USERPage = nullptr;
    MainPage* m_mainPage = nullptr;
    // ═════════════ 页面 ═════════
    QWidget* _dataPage          = nullptr;
    QWidget* _settingsPage      = nullptr;
    QWidget* _aboutPage         = nullptr;
    QWidget* _debugPage         = nullptr;
    QWidget* _singleSendPage    = nullptr;
    QWidget* _errorLogPage      = nullptr;

    // ═════════════ 子线程 ═════════
    QThread* m_networkThread    = nullptr;
    QThread* m_excelSendThread  = nullptr;
    QThread* m_serialThread     = nullptr;
    QThread* m_savedataThread   = nullptr;
    QThread* m_validatorThread  = nullptr;

    // ═════════════ 核心 Worker ═════════
    QTableWidget* m_excelTableWidget          = nullptr;
    QListWidget*  m_receiveListWidget         = nullptr;
    QListWidget*  m_sendListWidget            = nullptr;
    ExcelSendWorker*    m_excelSendWorker     = nullptr;
    NetworkClient*      m_networkClient       = nullptr;
    SerialWorker*       m_serialWorker        = nullptr;
    saveworker*         m_savedataWorker      = nullptr;
    ResponseValidator*  m_responseValidator   = nullptr;
    ExcelReader*        m_excelReader         = nullptr;

    // ═════════════ 数据变量 ═════════
    QQueue<QByteArray> m_expectedResponseQueue;
    int m_errorCount        = 0;
    int m_errorTimeOut      = 0;
    int m_maxDisplayItems   = 300;
    bool m_sendAndReadRecordBool = false;

    int m_currentRow         = -1;
    QByteArray m_currentExpected;

    enum class ConnectionType { None, Network, Serial };
    ConnectionType m_currentConnectionType = ConnectionType::None;

    QFileDialog m_fileDialog;
    bool m_debugMode        = false;
    bool m_shutdownComplete = false;

    // ═════════════ LED ═════════
    QLabel* m_NetWorkLED = nullptr;
    QLabel* m_SerialLED  = nullptr;

    // ═════════════ 标签 → ElaText ═════════
    ElaText* m_excelReadLabel           = nullptr;
    ElaText* m_receiveLabel             = nullptr;
    ElaText* m_sendLabel                = nullptr;
    ElaText* m_serverIpLabel            = nullptr;
    ElaText* m_portLabel                = nullptr;
    ElaText* m_delayLabel               = nullptr;
    ElaText* m_timeoutLabel             = nullptr;
    ElaText* m_sendLimitLabel           = nullptr;
    ElaText* m_sentCountLabel           = nullptr;
    ElaText* m_sentCountDisplayLabel    = nullptr;
    ElaText* m_serialPortLabel          = nullptr;
    ElaText* m_baudRateLabel            = nullptr;
    ElaText* m_dataBitsLabel            = nullptr;
    ElaText* m_stopBitsLabel            = nullptr;
    ElaText* m_parityLabel              = nullptr;
    ElaText* m_errorCountLabel          = nullptr;
    ElaText* m_errorCountDisplayLabel   = nullptr;
    ElaText* m_errorTimeOutLabel        = nullptr;
    ElaText* m_errorTimeOutDisplayLabel = nullptr;
    StatCard* m_errorCard{nullptr};
    StatCard* m_timeoutCard{nullptr};
    StatCard* m_totalSendCard{nullptr};
    ElaText* m_NetWorkLEDLabel          = nullptr;
    ElaText* m_SerialLEDLabel           = nullptr;
    ElaText* m_timePrecisionLabel       = nullptr;
    ElaText* m_dataPageErrorCountLabel      = nullptr;
    ElaText* m_dataPageErrorTimeOutLabel    = nullptr;
    // ═════════════ 输入框 → ElaLineEdit ═════════
    ElaLineEdit* m_serverIpLineEdit  = nullptr;
    ElaLineEdit* m_portLineEdit      = nullptr;
    ElaLineEdit* m_delayLineEdit     = nullptr;
    ElaLineEdit* m_timeoutLineEdit   = nullptr;
    ElaLineEdit* m_sendLimitLineEdit = nullptr;

    // ═════════════ 单条发送输入框 ═════════
    ElaLineEdit* m_singleSendInput = nullptr;

    // ═════════════ 下拉框 → ElaComboBox ═════════
    ElaComboBox* m_serialPortComboBox    = nullptr;
    ElaComboBox* m_baudRateComboBox      = nullptr;
    ElaComboBox* m_dataBitsComboBox      = nullptr;
    ElaComboBox* m_stopBitsComboBox      = nullptr;
    ElaComboBox* m_parityComboBox        = nullptr;
    ElaComboBox* m_timePrecisionComboBox = nullptr;

    // ═════════════ 勾选框 → ElaCheckBox ═════════
    ElaCheckBox* m_sendWithAN3CheckBox       = nullptr;
    ElaCheckBox* m_tcpNoDelayCheckBox        = nullptr;
    ElaCheckBox* m_testPacketLossCheckBox    = nullptr;
    ElaCheckBox* m_onlySendDataModeCheckBox  = nullptr;
    ElaCheckBox* m_serialBufferCheckBox      = nullptr;

    // ═════════════ 按钮 → ElaPushButton ═════════
    ElaPushButton* m_openExcelButton                  = nullptr;
    ElaPushButton* m_sendExcelButton                  = nullptr;
    ElaPushButton* m_stopSendExcelButton              = nullptr;
    ElaPushButton* m_sendSerialButton                 = nullptr;
    ElaPushButton* m_stopSendSerialButton             = nullptr;
    ElaPushButton* m_disconnectButton                 = nullptr;
    ElaPushButton* m_connectButton                    = nullptr;
    ElaPushButton* m_openSerialButton                 = nullptr;
    ElaPushButton* m_closeSerialButton                = nullptr;
    ElaPushButton* m_GUIClearButton                   = nullptr;
    ElaPushButton* m_sendNetWorkAndReadRecordButton   = nullptr;
    ElaPushButton* m_sendSerialAndReadRecordButton    = nullptr;

    // ═════════════ 单条发送按钮 ═════════
    ElaPushButton* m_singleSendNetBtn    = nullptr;
    ElaPushButton* m_singleSendSerialBtn = nullptr;
    ElaPushButton* m_singleSendClearBtn  = nullptr;

    // ═════════════ 单条发送日志 ═════════
    QListWidget* m_singleSendLog = nullptr;
    QListWidget* m_singleRecvLog = nullptr;

    // ═════════════ 错误日志页面 ═════════
    QListWidget*  m_errorLogList    = nullptr;
    ElaPushButton* m_exportErrorBtn = nullptr;
    ElaPushButton* m_clearErrorBtn  = nullptr;

    // ═════════════ 初始化方法 ═════════
    void initWindow();
    void initPages();
    void initNavigation();
    void initWindowConfig();
    void createDataPage();
    void createSettingsPage();
    void createSingleSendPage();
    void createErrorLogPage();
    void createDebugPage();
    void applyDebugMode(bool enabled);

    // ═════════════ 辅助方法 ═════════
    static QString toHexDisplay(const QByteArray &data);
    QString currentTimeString() const;
    QByteArray hexStringToBytes(const QString& hexText) const;
    QString bytesToDisplayText(const QByteArray& data) const;
    void setButtonsForNetworkMode();
    void setButtonsForSerialMode();
    void updateAllErrorDisplayLabels();

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
    void onErrorDetected(QByteArray expected, QByteArray actual);
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

    void onExportErrorLog();
    void onClearErrorLog();
};
