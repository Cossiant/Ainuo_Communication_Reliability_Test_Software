#pragma once

#include <QString>
#include <QSerialPort>

namespace CommIO {

// 串口配置（RS232/RS485/USB 虚拟串口）
struct SerialPortConfig {
    // 常规配置
    QString portName;                                         // "COM1"
    int baudRate = 9600;

    // 详细配置
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
    bool dataTerminalReady = false;                           // DTR 信号
    bool requestToSend = false;                               // RTS 信号
    int readTimeout = 1000;                                   // 读取超时 (ms)
    qint64 readBufferSize = 0;                                // 0 = 无限制
};



// GPIB 配置（NI-VISA）
struct GPIBConfig {
    // 常规配置
    int boardIndex = 0;                                       // GPIB 板卡号
    int primaryAddress = 0;                                   // 主地址 (0-30)

    // 详细配置
    int secondaryAddress = 0;                                 // 副地址 (0=不使用)
    int timeout = 3000;                                       // 超时 (ms)

    // EOS 模式 (VI_ATTR_TERMCHAR 相关)
    bool termCharEnabled = true;                              // 是否启用结束字符检测
    char termChar = '\n';                                     // 结束字符 (默认 LF)
    bool sendEndEnabled = true;                               // 发送时是否附加 EOI
};

// CAN 配置（周立功 ZLGCAN x64）
struct CANConfig {
    // 常规配置
    int canIndex = 0;                                         // CAN 通道索引

    // 详细配置
    int deviceType = 41;                                      // ZCAN_USBCANFD_200U = 41 (CANFD200)
    int deviceIndex = 0;                                      // 设备索引

    int canType = 1;                                          // 0=CAN, 1=CANFD
    int abitBaudRate = 500000;                                // 仲裁域波特率 (bps)
    int dbitBaudRate = 2000000;                               // 数据域波特率 (bps, CAN FD)
    int canfdStandard = 0;                                    // 0=ISO, 1=非ISO
    int terminalResistance = 0;                               // 是否使能内置终端电阻

    // 滤波/模式参数
    unsigned long accCode = 0x00000000;                       // 验收码
    unsigned long accMask = 0xFFFFFFFF;                       // 屏蔽码
    unsigned char filter = 0;                                 // 滤波方式
    unsigned char mode = 0;                                   // 0=正常, 1=只听

    int receiveWaitTime = 100;                                // 接收等待时间 (ms)
};

// TCP 配置（TCP Client）
struct TCPConfig {
    // 常规配置
    QString host;                                             // "192.168.1.100"
    quint16 port = 0;                                         // 端口号

    // 详细配置
    int connectTimeout = 3000;                                // 连接超时 (ms)
    int readTimeout = 1000;                                   // 读取超时 (ms)
    bool keepAlive = false;                                   // TCP KeepAlive
    bool lowDelay = false;                                    // TCP_NODELAY
    qint64 readBufferSize = 0;                                // 0 = 无限制
};

} // namespace CommIO

// 跨线程信号传递需要注册自定义类型
Q_DECLARE_METATYPE(CommIO::GPIBConfig)
Q_DECLARE_METATYPE(CommIO::CANConfig)
