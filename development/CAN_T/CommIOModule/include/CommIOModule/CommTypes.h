#pragma once

#include <QString>
#include <QSerialPort>
#include <QAbstractSocket>
#include <QtGlobal>

namespace CommIO {

// 通信通道类型
enum class ChannelType {
    SerialPort,  // RS232/RS485/USB 虚拟串口
    GPIB,
    CAN,
    TCP
};

enum class PerfStage {
    TxIoBegin,
    RxIoFirst
};

struct PerfEvent {
    ChannelType channel = ChannelType::SerialPort;
    PerfStage   stage   = PerfStage::TxIoBegin;
    qint64      epochUs = 0;
    QByteArray  data;   // TxIoBegin: 发送内容; RxIoFirst: 接收内容
};

// 错误类型枚举
// 参考 QSerialPort::SerialPortError、QAbstractSocket::SocketError、
// NI-VISA 状态码、ZLGCAN 错误码 综合设计
enum class ErrorType {
    NoError = 0,

    // === 连接相关错误 (1xx) ===
    ConnectionFailed = 100,     // 通用连接失败
    DeviceNotFound = 101,       // QSerialPort::DeviceNotFoundError / VI_ERROR_RSRC_NFOUND / ERR_DEVICENOTEXIST
    DeviceBusy = 102,           // QSerialPort::PermissionError / VI_ERROR_RSRC_LOCKED / ERR_DEVICEOPENED
    PermissionDenied = 103,     // QAbstractSocket::SocketAccessError
    AlreadyOpen = 104,          // QSerialPort::OpenError
    ConnectionRefused = 105,    // QAbstractSocket::ConnectionRefusedError
    HostNotFound = 106,         // QAbstractSocket::HostNotFoundError
    RemoteHostClosed = 107,     // QAbstractSocket::RemoteHostClosedError

    // === 配置相关错误 (2xx) ===
    ConfigError = 200,          // 通用配置错误
    InvalidParameter = 201,     // VI_ERROR_INV_SETUP
    InvalidAddress = 202,       // GPIB 主地址超出 0-30 / TCP IP 格式错误
    UnsupportedOperation = 203, // QSerialPort::UnsupportedOperationError

    // === 通信相关错误 (3xx) ===
    TimeoutError = 300,         // QSerialPort::TimeoutError / VI_ERROR_TMO / QAbstractSocket::SocketTimeoutError
    WriteError = 301,           // QSerialPort::WriteError / VI_ERROR_IO
    ReadError = 302,            // QSerialPort::ReadError
    ParityError = 303,          // QSerialPort::ParityError / VI_ERROR_PARITY_ERR
    FramingError = 304,         // QSerialPort::FramingError / VI_ERROR_FRM_ERR
    BreakCondition = 305,       // QSerialPort::BreakConditionError
    OverrunError = 306,         // VI_ERROR_OVERRUN_ERR

    // === 硬件/驱动相关错误 (4xx) ===
    HardwareError = 400,        // 通用硬件错误
    ResourceError = 401,        // QSerialPort::ResourceError / QAbstractSocket::SocketResourceError
    DriverNotInstalled = 402,   // ERR_LOADKERNELDLL (0x2000)
    DeviceDisconnected = 403,   // VI_ERROR_CONN_LOST
    NetworkError = 404,         // QAbstractSocket::NetworkError
    BusError = 405,             // VI_ERROR_BUS_ERR / ERR_CAN_BUSERR
    BusOff = 406,               // ERR_CAN_BUSOFF (0x0020)
    BufferOverflow = 407,       // ERR_CAN_OVERFLOW / ERR_BUFFEROVERFLOW
    CommandFailed = 408,        // ERR_CMDFAILED (0x4000)

    // === 协议相关错误 (5xx, 预留扩展) ===
    ProtocolError = 500,
    InvalidResponse = 501,

    // === 系统相关错误 (9xx) ===
    NotOpenError = 900,         // QSerialPort::NotOpenError / ERR_DEVICENOTOPEN
    UnknownError = 999
};

// 错误信息辅助类
class ErrorHelper {
public:
    // 错误类型转为可读字符串
    static QString errorTypeToString(ErrorType type);

    // 获取详细错误信息（含上下文）
    static QString getDetailedMessage(ErrorType type, const QString& context = QString());

    // 各库错误码 → ErrorType 转换
    static ErrorType fromSerialPortError(QSerialPort::SerialPortError error);
    static ErrorType fromSocketError(QAbstractSocket::SocketError error);
    static ErrorType fromVISAStatus(long viStatus);
    static ErrorType fromCANErrorCode(unsigned int errCode);
};

QString channelTypeToString(ChannelType type);

} // namespace CommIO

// 跨线程信号传递需要注册自定义类型
Q_DECLARE_METATYPE(CommIO::ErrorType)
Q_DECLARE_METATYPE(CommIO::PerfEvent)
