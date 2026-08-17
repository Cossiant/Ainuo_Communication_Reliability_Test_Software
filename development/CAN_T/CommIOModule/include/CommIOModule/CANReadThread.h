#pragma once

#include <QThread>
#include <QByteArray>
#include <QAtomicInt>

#include "CommTypes.h"

namespace CommIO {

// CAN 独立接收线程：使用 ZCAN_ReceiveData 合并接收 CAN/CAN FD 帧。
class CANReadThread : public QThread
{
    Q_OBJECT

public:
    explicit CANReadThread(QObject *parent = nullptr);

    void setDeviceInfo(quintptr deviceHandle, int canIndex);
    void requestStop();

signals:
    void dataReady(const QByteArray &data);
    void errorOccurred(CommIO::ErrorType type, const QString &message);
    void perfEvent(const CommIO::PerfEvent &event);

protected:
    void run() override;

private:
    quintptr m_deviceHandle = 0;
    int m_canIndex = 0;
    QAtomicInt m_running;
};

} // namespace CommIO
