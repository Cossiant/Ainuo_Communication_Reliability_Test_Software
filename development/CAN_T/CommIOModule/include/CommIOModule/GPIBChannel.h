#pragma once

#include "ICommChannel.h"
#include "CommConfig.h"

#include <QThread>

namespace CommIO {

class GPIBWorker;

// GPIB 通信实现（NI-VISA，主线程代理对象）
class GPIBChannel : public ICommChannel {
    Q_OBJECT

public:
    explicit GPIBChannel(QObject* parent = nullptr);
    ~GPIBChannel() override;

    bool open(const QVariantMap& config) override;
    void close() override;
    void send(const QByteArray& data) override;     // 只写，不读
    void query(const QByteArray& data) override;     // 写后读取响应
    bool isOpen() const override;
    ChannelType type() const override { return ChannelType::GPIB; }

signals:
    // 内部信号：跨线程传递给 Worker
    void requestOpen(const CommIO::GPIBConfig& config);
    void requestSend(const QByteArray& data);
    void requestQuery(const QByteArray& data);
    void requestClose();

private:
    // 从 QVariantMap 解析配置
    GPIBConfig parseConfig(const QVariantMap& config) const;

    QThread* m_workerThread = nullptr;
    GPIBWorker* m_worker = nullptr;
    bool m_isOpen = false;

private slots:
    void onWorkerOpened(bool success);
    void onWorkerDataReady(const QByteArray& data);
    void onWorkerError(CommIO::ErrorType type, const QString& message);
    void onWorkerPerfEvent(const CommIO::PerfEvent& event);
    void onWorkerClosed();
};

} // namespace CommIO
