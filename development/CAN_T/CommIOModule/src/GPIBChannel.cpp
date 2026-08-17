#include "CommIOModule/GPIBChannel.h"
#include "CommIOModule/GPIBWorker.h"

namespace CommIO {

GPIBChannel::GPIBChannel(QObject* parent)
    : ICommChannel(parent)
{
    // Qt5 跨线程 QueuedConnection 必须在运行时注册类型
    qRegisterMetaType<CommIO::GPIBConfig>();
    qRegisterMetaType<CommIO::ErrorType>();
    qRegisterMetaType<CommIO::PerfEvent>();

    m_workerThread = new QThread(this);
    m_worker = new GPIBWorker(); // 不设 parent，由 moveToThread 管理
    m_workerThread->start(QThread::TimeCriticalPriority);
    m_worker->moveToThread(m_workerThread);

    // 主线程 → Worker 线程的操作信号
    connect(this, &GPIBChannel::requestOpen,
            m_worker, &GPIBWorker::doOpen);
    connect(this, &GPIBChannel::requestSend,
            m_worker, &GPIBWorker::doSend);
    connect(this, &GPIBChannel::requestQuery,
            m_worker, &GPIBWorker::doQuery);
    connect(this, &GPIBChannel::requestClose,
            m_worker, &GPIBWorker::doClose);

    // Worker 线程 → 主线程的结果信号
    connect(m_worker, &GPIBWorker::opened,
            this, &GPIBChannel::onWorkerOpened);
    connect(m_worker, &GPIBWorker::dataReady,
            this, &GPIBChannel::onWorkerDataReady);
    connect(m_worker, &GPIBWorker::errorOccurred,
            this, &GPIBChannel::onWorkerError);
    connect(m_worker, &GPIBWorker::perfEvent,
            this, &GPIBChannel::onWorkerPerfEvent);
    connect(m_worker, &GPIBWorker::closed,
            this, &GPIBChannel::onWorkerClosed);

    // 线程结束时清理 Worker
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

GPIBChannel::~GPIBChannel()
{
    // 同步关闭 VISA 会话（不 emit 信号，对象正在销毁）
    if (m_isOpen) {
        QMetaObject::invokeMethod(m_worker, "doClose", Qt::BlockingQueuedConnection);
    }
    m_workerThread->quit();
    m_workerThread->wait(3000);
}

bool GPIBChannel::open(const QVariantMap& config)
{
    if (m_isOpen) {
        emit errorOccurred(ErrorType::AlreadyOpen,
                           tr("GPIB 已打开"));
        return false;
    }

    GPIBConfig cfg = parseConfig(config);

    // 地址范围校验
    if (cfg.primaryAddress < 0 || cfg.primaryAddress > 30) {
        emit errorOccurred(ErrorType::InvalidAddress,
                           tr("GPIB 主地址超出范围 (0-30): %1").arg(cfg.primaryAddress));
        return false;
    }

    emit requestOpen(cfg);
    return true; // 异步操作，实际结果通过 onWorkerOpened 回调
}

void GPIBChannel::close()
{
    if (!m_isOpen) {
        return;
    }
    // 同步调用 Worker::doClose()，阻塞直到 VISA 资源释放完毕
    QMetaObject::invokeMethod(m_worker, "doClose", Qt::BlockingQueuedConnection);
    m_isOpen = false;
    emit connectionStateChanged(false);
}

void GPIBChannel::send(const QByteArray& data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("GPIB 未打开"));
        return;
    }
    emit requestSend(data);
}

void GPIBChannel::query(const QByteArray& data)
{
    if (!m_isOpen) {
        emit errorOccurred(ErrorType::NotOpenError,
                           tr("GPIB 未打开"));
        return;
    }
    emit requestQuery(data);
}

bool GPIBChannel::isOpen() const
{
    return m_isOpen;
}

GPIBConfig GPIBChannel::parseConfig(const QVariantMap& config) const
{
    GPIBConfig cfg;

    cfg.boardIndex = config.value("boardIndex", 0).toInt();
    cfg.primaryAddress = config.value("primaryAddress", 0).toInt();
    cfg.secondaryAddress = config.value("secondaryAddress", 0).toInt();
    cfg.timeout = config.value("timeout", 3000).toInt();
    cfg.termCharEnabled = config.value("termCharEnabled", true).toBool();

    // termChar 支持 int 或 string 传入
    QVariant termCharVar = config.value("termChar", QVariant('\n'));
    if (termCharVar.type() == QVariant::Int) {
        cfg.termChar = static_cast<char>(termCharVar.toInt());
    } else {
        QString termCharStr = termCharVar.toString();
        cfg.termChar = termCharStr.isEmpty() ? '\n' : termCharStr.at(0).toLatin1();
    }

    cfg.sendEndEnabled = config.value("sendEndEnabled", true).toBool();

    return cfg;
}

void GPIBChannel::onWorkerOpened(bool success)
{
    m_isOpen = success;
    emit connectionStateChanged(success);
}

void GPIBChannel::onWorkerDataReady(const QByteArray& data)
{
    emit dataReceived(data);
}

void GPIBChannel::onWorkerError(CommIO::ErrorType type, const QString& message)
{
    // 连接丢失 → 自动标记为关闭
    if (type == ErrorType::DeviceDisconnected) {
        m_isOpen = false;
        emit connectionStateChanged(false);
    }
    emit errorOccurred(type, message);
}

void GPIBChannel::onWorkerPerfEvent(const CommIO::PerfEvent& event)
{
    emit perfEvent(event);
}

void GPIBChannel::onWorkerClosed()
{
    // 防止与同步 close() 重复触发
    if (!m_isOpen) {
        return;
    }
    m_isOpen = false;
    emit connectionStateChanged(false);
}

} // namespace CommIO
