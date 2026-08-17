#include "CommIOModule/CANChannel.h"
#include "CommIOModule/CANWorker.h"
#include "CommIOModule/CANReadThread.h"
#include "CommIOModule/ZLGCanLoader.h"
namespace CommIO {

CANChannel::CANChannel(QObject* parent)
    : ICommChannel(parent)
{
    // Qt5 跨线程 QueuedConnection 必须在运行时注册类型
    qRegisterMetaType<CommIO::CANConfig>();
    qRegisterMetaType<CommIO::ErrorType>();
    qRegisterMetaType<CommIO::PerfEvent>();
    // 运行时加载 64 位 zlgcan.dll
    ZLGCanDriver::load();

    m_workerThread = new QThread(this);
    m_workerThread->start(QThread::TimeCriticalPriority);
    m_worker = new CANWorker();

    m_worker->moveToThread(m_workerThread);

    // 主线程 → Worker 线程
    connect(this, &CANChannel::requestOpen,
            m_worker, &CANWorker::doOpen);
    connect(this, &CANChannel::requestSend,
            m_worker, &CANWorker::doSend);
    connect(this, &CANChannel::requestClose,
            m_worker, &CANWorker::doClose);

    // Worker 线程 → 主线程
    connect(m_worker, &CANWorker::opened,
            this, &CANChannel::onWorkerOpened);
    connect(m_worker, &CANWorker::errorOccurred,
            this, &CANChannel::onWorkerError);
    connect(m_worker, &CANWorker::perfEvent,
            this, &CANChannel::onWorkerPerfEvent);
    connect(m_worker, &CANWorker::closed,
            this, &CANChannel::onWorkerClosed);

    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

CANChannel::~CANChannel()
{
    // 停止读取线程
    stopReadThread();
    // 同步关闭设备（不 emit 信号，对象正在销毁）
    if (m_isOpen) {
        QMetaObject::invokeMethod(m_worker, "doClose", Qt::BlockingQueuedConnection);
    }
    m_workerThread->quit();
    m_workerThread->wait(3000);
}

bool CANChannel::open(const QVariantMap& config)
{
    if (m_isOpen) {
        emit errorOccurred(ErrorType::AlreadyOpen,
                           tr("CAN 已打开"));
        return false;
    }

    if (!ZLGCanDriver::isLoaded()) {
        emit errorOccurred(ErrorType::DriverNotInstalled,
                           tr("未找到可用的 64 位 zlgcan.dll"));
        return false;
    }

    CANConfig cfg = parseConfig(config);

    emit requestOpen(cfg);
    return true;
}

void CANChannel::close()
{
    if (!m_isOpen) {
        return;
    }
    // 先停止读取线程，确保 VCI_Receive 不再访问设备
    stopReadThread();
    // 同步调用 Worker::doClose()，阻塞直到 VCI_CloseDevice 完成
    QMetaObject::invokeMethod(m_worker, "doClose", Qt::BlockingQueuedConnection);
    m_isOpen = false;
    emit connectionStateChanged(false);
}

void CANChannel::stopReadThread()
{
    if (m_readThread) {
        m_readThread->requestStop();
        m_readThread->wait(2000);
        delete m_readThread;
        m_readThread = nullptr;
    }
}

void CANChannel::send(const QByteArray& data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("CAN 未打开"));
        return;
    }
    emit requestSend(data);
}

bool CANChannel::isOpen() const
{
    return m_isOpen;
}

CANConfig CANChannel::parseConfig(const QVariantMap& config) const
{
    CANConfig cfg;

    cfg.canIndex = config.value("canIndex", 0).toInt();
    cfg.deviceType = config.value("deviceType", 41).toInt();
    cfg.deviceIndex = config.value("deviceIndex", 0).toInt();
    cfg.canType = config.value("canType", 1).toInt();
    cfg.abitBaudRate = config.value("abitBaudRate", 500000).toInt();
    cfg.dbitBaudRate = config.value("dbitBaudRate", 2000000).toInt();
    cfg.canfdStandard = config.value("canfdStandard", 0).toInt();
    cfg.terminalResistance = config.value("terminalResistance", 0).toInt();
    cfg.accCode = config.value("accCode", 0x00000000).toUInt();
    cfg.accMask = config.value("accMask", 0xFFFFFFFF).toUInt();
    cfg.filter = static_cast<unsigned char>(config.value("filter", 0).toUInt());
    cfg.mode = static_cast<unsigned char>(config.value("mode", 0).toUInt());
    cfg.receiveWaitTime = config.value("receiveWaitTime", 100).toInt();

    return cfg;
}

void CANChannel::onWorkerOpened(bool success,
                                 quintptr deviceHandle,
                                 int canIndex,
                                 int receiveWaitTime)
{
    Q_UNUSED(receiveWaitTime)
    m_isOpen = success;

    if (success) {
        // 设备打开成功，启动独立读取线程
        m_readThread = new CANReadThread();
        m_readThread->setDeviceInfo(deviceHandle, canIndex);

        connect(m_readThread, &CANReadThread::dataReady,
                this, &CANChannel::onReadThreadDataReady);
        connect(m_readThread, &CANReadThread::errorOccurred,
                this, &CANChannel::onReadThreadError);
        connect(m_readThread, &CANReadThread::perfEvent,
                this, &CANChannel::onReadThreadPerfEvent);

        m_readThread->start();
    }

    emit connectionStateChanged(success);
}

void CANChannel::onReadThreadDataReady(const QByteArray& data)
{
    emit dataReceived(data);
}

void CANChannel::onWorkerError(CommIO::ErrorType type, const QString& message)
{
    if (type == ErrorType::BusOff || type == ErrorType::DeviceDisconnected || type == ErrorType::CommandFailed) {
        m_isOpen = false;
        // 发送错误时也停止读取线程
        if (m_readThread) {
            m_readThread->requestStop();
        }
        emit connectionStateChanged(false);
    }
    emit errorOccurred(type, message);
}

void CANChannel::onReadThreadError(CommIO::ErrorType type, const QString& message)
{
    emit errorOccurred(type, message);
}

void CANChannel::onWorkerPerfEvent(const CommIO::PerfEvent& event)
{
    emit perfEvent(event);
}

void CANChannel::onReadThreadPerfEvent(const CommIO::PerfEvent& event)
{
    emit perfEvent(event);
}

void CANChannel::onWorkerClosed()
{
    // 防止与同步 close() 重复触发
    if (!m_isOpen) {
        return;
    }
    m_isOpen = false;
    emit connectionStateChanged(false);
}

} // namespace CommIO
