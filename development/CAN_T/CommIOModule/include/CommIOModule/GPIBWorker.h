#pragma once

#include <QObject>
#include "CommTypes.h"
#include "CommConfig.h"

// NI-VISA 头文件 (前向声明类型以避免全局包含)
typedef unsigned long ViSession;
typedef long ViStatus;

namespace CommIO {

// GPIB Worker：在独立线程中执行阻塞 VISA 操作
class GPIBWorker : public QObject {
    Q_OBJECT

public:
    explicit GPIBWorker(QObject* parent = nullptr);
    ~GPIBWorker() override;

public slots:
    void doOpen(const CommIO::GPIBConfig& config);
    void doSend(const QByteArray& data);   // 只写，不读
    void doQuery(const QByteArray& data);  // 写后读取响应
    void doClose();

signals:
    void opened(bool success);
    void dataReady(const QByteArray& data);
    void errorOccurred(CommIO::ErrorType type, const QString& message);
    void perfEvent(const CommIO::PerfEvent& event);
    void closed();

private:
    // 执行 viRead，阻塞直到数据到达或超时
    void readResponse();

    // VISA 错误处理辅助
    bool checkVISAStatus(ViStatus status, const QString& operation);

    ViSession m_resourceManager = 0;
    ViSession m_instrument = 0;
    bool m_isOpen = false;
    int m_timeout = 3000;
};

} // namespace CommIO
