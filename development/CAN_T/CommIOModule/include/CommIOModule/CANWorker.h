#pragma once

#include <QObject>

#include "CommTypes.h"
#include "CommConfig.h"

namespace CommIO {

// CAN Worker：在独立线程中执行 ZLGCAN 打开/关闭和发送操作。
class CANWorker : public QObject
{
    Q_OBJECT

public:
    explicit CANWorker(QObject *parent = nullptr);
    ~CANWorker() override;

public slots:
    void doOpen(const CommIO::CANConfig &config);
    void doSend(const QByteArray &data);
    void doClose();

signals:
    void opened(bool success,
                quintptr deviceHandle,
                int canIndex,
                int receiveWaitTime);
    void errorOccurred(CommIO::ErrorType type, const QString &message);
    void perfEvent(const CommIO::PerfEvent &event);
    void closed();

private:
    quintptr m_deviceHandle = 0;
    quintptr m_channelHandle = 0;
    int m_canIndex = 0;
    bool m_isOpen = false;
};

} // namespace CommIO
