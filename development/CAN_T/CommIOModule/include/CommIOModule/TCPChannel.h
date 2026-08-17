#pragma once

#include "ICommChannel.h"
#include "CommConfig.h"

#include <QTcpSocket>
#include <QTimer>

namespace CommIO {

// TCP Client 通信实现
class TCPChannel : public ICommChannel {
    Q_OBJECT

public:
    explicit TCPChannel(QObject* parent = nullptr);
    ~TCPChannel() override;

    bool open(const QVariantMap& config) override;
    void close() override;
    void send(const QByteArray& data) override;
    bool isOpen() const override;
    ChannelType type() const override { return ChannelType::TCP; }

private:
    // 从 QVariantMap 解析配置
    TCPConfig parseConfig(const QVariantMap& config) const;

    QTcpSocket* m_socket = nullptr;
    QTimer* m_connectTimer = nullptr;
    bool m_isConnecting = false;    // 防止超时和错误信号重复触发

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketErrorOccurred(QAbstractSocket::SocketError error);
    void onConnectTimeout();
};

} // namespace CommIO
