#pragma once

#include "ICommChannel.h"
#include "CommConfig.h"

#include <QThread>

namespace CommIO {

class CANWorker;
class CANReadThread;

// CAN 通信实现（周立功 ZLGCAN x64，主线程代理对象）
// 写操作通过 Worker 线程，读操作通过独立 ReadThread
class CANChannel : public ICommChannel {
    Q_OBJECT

public:
    explicit CANChannel(QObject* parent = nullptr);
    ~CANChannel() override;

    bool open(const QVariantMap& config) override;
    void close() override;
    void send(const QByteArray& data) override;
    bool isOpen() const override;
    ChannelType type() const override { return ChannelType::CAN; }

signals:
    // 内部信号：跨线程传递给 Worker
    void requestOpen(const CommIO::CANConfig& config);
    void requestSend(const QByteArray& data);
    void requestClose();

private:
    // 从 QVariantMap 解析配置
    CANConfig parseConfig(const QVariantMap& config) const;

    // 停止并释放读取线程
    void stopReadThread();

    QThread* m_workerThread = nullptr;
    CANWorker* m_worker = nullptr;
    CANReadThread* m_readThread = nullptr;
    bool m_isOpen = false;

private slots:
    void onWorkerOpened(bool success,
                        quintptr deviceHandle,
                        int canIndex,
                        int receiveWaitTime);
    void onReadThreadDataReady(const QByteArray& data);
    void onWorkerError(CommIO::ErrorType type, const QString& message);
    void onReadThreadError(CommIO::ErrorType type, const QString& message);
    void onWorkerPerfEvent(const CommIO::PerfEvent& event);
    void onReadThreadPerfEvent(const CommIO::PerfEvent& event);
    void onWorkerClosed();
};

} // namespace CommIO
