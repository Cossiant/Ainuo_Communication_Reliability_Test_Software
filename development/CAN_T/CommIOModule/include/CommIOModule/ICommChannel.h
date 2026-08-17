#pragma once

#include <QObject>
#include <QByteArray>
#include <QVariantMap>

#include "CommTypes.h"

namespace CommIO {

// 统一通信通道抽象基类
class ICommChannel : public QObject {
    Q_OBJECT

public:
    explicit ICommChannel(QObject* parent = nullptr)
        : QObject(parent) {}
    virtual ~ICommChannel() = default;

    // 核心操作
    virtual bool open(const QVariantMap& config) = 0;

    // 状态查询
    virtual bool isOpen() const = 0;
    virtual ChannelType type() const = 0;

public slots:
    virtual void close() = 0;
    virtual void send(const QByteArray& data) = 0;

    // 查询操作（发送后等待读取响应）
    // 默认实现等同 send，GPIB 等需要手动触发读取的接口应 override
    virtual void query(const QByteArray& data) { send(data); }

signals:
    void dataReceived(const QByteArray& data);
    void errorOccurred(CommIO::ErrorType type, const QString& message);
    void connectionStateChanged(bool connected);
    void perfEvent(const CommIO::PerfEvent& event);
};

} // namespace CommIO
