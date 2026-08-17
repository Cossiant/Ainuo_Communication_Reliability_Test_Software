#pragma once

#include "ICommChannel.h"
#include "CommConfig.h"

#include <QSerialPort>

namespace CommIO {

// 串口通信实现（RS232/RS485/USB 虚拟串口）
class SerialPortChannel : public ICommChannel {
    Q_OBJECT

public:
    explicit SerialPortChannel(QObject* parent = nullptr);
    ~SerialPortChannel() override;

    bool open(const QVariantMap& config) override;
    void close() override;
    void send(const QByteArray& data) override;
    bool isOpen() const override;
    ChannelType type() const override { return ChannelType::SerialPort; }

private:
    // 从 QVariantMap 解析配置
    SerialPortConfig parseConfig(const QVariantMap& config) const;

    QSerialPort* m_serialPort = nullptr;

private slots:
    void onReadyRead();
    void onSerialPortErrorOccurred(QSerialPort::SerialPortError error);
};

} // namespace CommIO
