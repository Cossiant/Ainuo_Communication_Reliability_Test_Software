#include "CommIOModule/CommTypes.h"

#include <QCoreApplication>

namespace CommIO {

namespace {

QString trError(const char* sourceText)
{
    return QCoreApplication::translate("CommIO::ErrorHelper", sourceText);
}

} // namespace

QString channelTypeToString(ChannelType type)
{
    switch (type) {
    case ChannelType::SerialPort: return QStringLiteral("SerialPort");
    case ChannelType::GPIB:       return QStringLiteral("GPIB");
    case ChannelType::CAN:        return QStringLiteral("CAN");
    case ChannelType::TCP:        return QStringLiteral("TCP");
    }
    return QStringLiteral("Unknown");
}


QString ErrorHelper::errorTypeToString(ErrorType type)
{
    switch (type) {
    case ErrorType::NoError:              return trError("未知错误");
    case ErrorType::ConnectionFailed:     return trError("连接失败");
    case ErrorType::DeviceNotFound:       return trError("设备未找到");
    case ErrorType::DeviceBusy:           return trError("设备被占用");
    case ErrorType::PermissionDenied:     return trError("权限不足");
    case ErrorType::AlreadyOpen:          return trError("设备已打开");
    case ErrorType::ConnectionRefused:    return trError("连接被拒绝");
    case ErrorType::HostNotFound:         return trError("主机未找到");
    case ErrorType::RemoteHostClosed:     return trError("远程主机关闭连接");
    case ErrorType::ConfigError:          return trError("配置错误");
    case ErrorType::InvalidParameter:     return trError("参数无效");
    case ErrorType::InvalidAddress:       return trError("地址无效");
    case ErrorType::UnsupportedOperation: return trError("不支持的操作");
    case ErrorType::TimeoutError:         return trError("通信超时");
    case ErrorType::WriteError:           return trError("写入失败");
    case ErrorType::ReadError:            return trError("读取失败");
    case ErrorType::ParityError:          return trError("奇偶校验错误");
    case ErrorType::FramingError:         return trError("帧错误");
    case ErrorType::BreakCondition:       return trError("Break 信号");
    case ErrorType::OverrunError:         return trError("溢出错误");
    case ErrorType::HardwareError:        return trError("硬件错误");
    case ErrorType::ResourceError:        return trError("资源不可用");
    case ErrorType::DriverNotInstalled:   return trError("驱动未安装");
    case ErrorType::DeviceDisconnected:   return trError("设备断开连接");
    case ErrorType::NetworkError:         return trError("网络错误");
    case ErrorType::BusError:             return trError("总线错误");
    case ErrorType::BusOff:               return trError("总线关闭");
    case ErrorType::BufferOverflow:       return trError("缓冲区溢出");
    case ErrorType::CommandFailed:        return trError("命令执行失败");
    case ErrorType::ProtocolError:        return trError("协议错误");
    case ErrorType::InvalidResponse:      return trError("响应格式无效");
    case ErrorType::NotOpenError:         return trError("设备未打开");
    case ErrorType::UnknownError:         return trError("未知错误");
    default:                              return trError("未知错误");
    }
}

QString ErrorHelper::getDetailedMessage(ErrorType type, const QString& context)
{
    QString msg = errorTypeToString(type);
    if (!context.isEmpty()) {
        msg += QStringLiteral(": ") + context;
    }
    return msg;
}

// QSerialPort::SerialPortError → ErrorType
ErrorType ErrorHelper::fromSerialPortError(QSerialPort::SerialPortError error)
{
    switch (error) {
    case QSerialPort::NoError:                    return ErrorType::NoError;
    case QSerialPort::DeviceNotFoundError:        return ErrorType::DeviceNotFound;
    case QSerialPort::PermissionError:            return ErrorType::DeviceBusy;
    case QSerialPort::OpenError:                  return ErrorType::AlreadyOpen;
    case QSerialPort::WriteError:                 return ErrorType::WriteError;
    case QSerialPort::ReadError:                  return ErrorType::ReadError;
    case QSerialPort::ResourceError:              return ErrorType::ResourceError;
    case QSerialPort::UnsupportedOperationError:  return ErrorType::UnsupportedOperation;
    case QSerialPort::TimeoutError:               return ErrorType::TimeoutError;
    case QSerialPort::NotOpenError:               return ErrorType::NotOpenError;
    case QSerialPort::ParityError:                return ErrorType::ParityError;
    case QSerialPort::FramingError:               return ErrorType::FramingError;
    case QSerialPort::BreakConditionError:        return ErrorType::BreakCondition;
    default:                                      return ErrorType::UnknownError;
    }
}

// QAbstractSocket::SocketError → ErrorType
ErrorType ErrorHelper::fromSocketError(QAbstractSocket::SocketError error)
{
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:     return ErrorType::ConnectionRefused;
    case QAbstractSocket::RemoteHostClosedError:      return ErrorType::RemoteHostClosed;
    case QAbstractSocket::HostNotFoundError:          return ErrorType::HostNotFound;
    case QAbstractSocket::SocketAccessError:          return ErrorType::PermissionDenied;
    case QAbstractSocket::SocketResourceError:        return ErrorType::ResourceError;
    case QAbstractSocket::SocketTimeoutError:         return ErrorType::TimeoutError;
    case QAbstractSocket::NetworkError:               return ErrorType::NetworkError;
    default:                                          return ErrorType::UnknownError;
    }
}

// NI-VISA 状态码 → ErrorType
ErrorType ErrorHelper::fromVISAStatus(long viStatus)
{
    // VISA 错误码为负值
    switch (viStatus) {
    case 0:           return ErrorType::NoError;             // VI_SUCCESS
    case -1073807343: return ErrorType::DeviceNotFound;      // VI_ERROR_RSRC_NFOUND
    case -1073807345: return ErrorType::DeviceBusy;          // VI_ERROR_RSRC_LOCKED
    case -1073807195: return ErrorType::ConfigError;         // VI_ERROR_INTF_NUM_NCONFIG
    case -1073807339: return ErrorType::TimeoutError;        // VI_ERROR_TMO
    case -1073807198: return ErrorType::WriteError;          // VI_ERROR_IO
    case -1073807194: return ErrorType::DeviceDisconnected;  // VI_ERROR_CONN_LOST
    case -1073807246: return ErrorType::BusError;            // VI_ERROR_BUS_ERR
    case -1073807244: return ErrorType::InvalidParameter;    // VI_ERROR_INV_SETUP
    case -1073807233: return ErrorType::ParityError;         // VI_ERROR_PARITY_ERR
    case -1073807232: return ErrorType::FramingError;        // VI_ERROR_FRM_ERR
    case -1073807231: return ErrorType::OverrunError;        // VI_ERROR_OVERRUN_ERR
    case -1073807355: return ErrorType::UnsupportedOperation; // VI_ERROR_NSUP_OPER
    case -1073807346: return ErrorType::InvalidParameter;    // VI_ERROR_INV_OBJECT
    default:          return ErrorType::UnknownError;
    }
}

// ZLGCAN 错误码 → ErrorType (错误码可同时包含多个位标志)
ErrorType ErrorHelper::fromCANErrorCode(unsigned int errCode)
{
    if (errCode == 0)
        return ErrorType::ConfigError;

    // 按优先级判断（严重错误优先）
    if (errCode & 0x2000) return ErrorType::DeviceNotFound;   // ERR_LOADKERNELDLL
    if (errCode & 0x1000) return ErrorType::DeviceNotFound;       // ERR_DEVICENOTEXIST
    if (errCode & 0x4000) return ErrorType::CommandFailed;        // ERR_CMDFAILED
    if (errCode & 0x0100) return ErrorType::DeviceBusy;           // ERR_DEVICEOPENED
    if (errCode & 0x0200) return ErrorType::ConnectionFailed;     // ERR_DEVICEOPEN
    if (errCode & 0x0400) return ErrorType::NotOpenError;         // ERR_DEVICENOTOPEN
    if (errCode & 0x0020) return ErrorType::BusOff;               // ERR_CAN_BUSOFF
    if (errCode & 0x0010) return ErrorType::BusError;             // ERR_CAN_BUSERR
    if (errCode & 0x0041) return ErrorType::BufferOverflow;       // ERR_CAN_OVERFLOW | ERR_CAN_BUFFER_OVERFLOW
    if (errCode & 0x0800) return ErrorType::BufferOverflow;       // ERR_BUFFEROVERFLOW
    if (errCode & 0x8000) return ErrorType::ResourceError;        // ERR_BUFFERCREATE
    if (errCode & 0x0002) return ErrorType::HardwareError;        // ERR_CAN_ERRALARM
    if (errCode & 0x0004) return ErrorType::HardwareError;        // ERR_CAN_PASSIVE

    return ErrorType::UnknownError;
}

} // namespace CommIO
