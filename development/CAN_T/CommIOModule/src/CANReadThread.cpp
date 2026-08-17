#include "CommIOModule/CANReadThread.h"
#include "CommIOModule/PerfClock.h"
#include "CommIOModule/ZLGCanLoader.h"

#include <QCoreApplication>
#include <QVector>

namespace CommIO {

namespace {

PerfEvent makePerfEvent(PerfStage stage, const QByteArray &data)
{
    PerfEvent event;
    event.channel = ChannelType::CAN;
    event.stage = stage;
    event.epochUs = PerfClock::epochUs();
    event.data = data;
    return event;
}

QByteArray encodeFrame(const ZCANCANFDData &data)
{
    const bool isFd = (data.flag.unionVal.frameType == 1);
    const quint32 canIdRaw = data.frame.can_id;
    const int len = qBound(0, static_cast<int>(data.frame.len), 64);

    // 应用统一帧格式：1 字节标志 + 4 字节大端 ID + 数据
    quint8 flags = 0;
    if (isFd) flags |= 0x01;
    if (data.frame.flags & CANFD_BRS) flags |= 0x02;
    if (canIdRaw & CAN_EFF_FLAG) flags |= 0x04;

    QByteArray out;
    out.reserve(5 + len);
    out.append(static_cast<char>(flags));
    out.append(static_cast<char>((canIdRaw >> 24) & 0xFF));
    out.append(static_cast<char>((canIdRaw >> 16) & 0xFF));
    out.append(static_cast<char>((canIdRaw >> 8) & 0xFF));
    out.append(static_cast<char>(canIdRaw & 0xFF));
    out.append(reinterpret_cast<const char *>(data.frame.data), len);
    return out;
}

} // namespace

CANReadThread::CANReadThread(QObject *parent)
    : QThread(parent)
{
    m_running.storeRelaxed(0);
}

void CANReadThread::setDeviceInfo(quintptr deviceHandle, int canIndex)
{
    m_deviceHandle = deviceHandle;
    m_canIndex = canIndex;
}

void CANReadThread::requestStop()
{
    m_running.storeRelaxed(0);
}

void CANReadThread::run()
{
    m_running.storeRelaxed(1);

    QVector<ZCANDataObj> recvBuf(50);

    while (m_running.loadRelaxed()) {
        const int waitMs = 1;
        const UINT recvCount = ZLGCanDriver::receiveData(
            reinterpret_cast<DEVICE_HANDLE>(m_deviceHandle),
            recvBuf.data(), static_cast<UINT>(recvBuf.size()), waitMs);

        if (recvCount == 0) {
            continue;
        }

        for (UINT i = 0; i < recvCount; ++i) {
            const ZCANDataObj &obj = recvBuf[static_cast<int>(i)];
            if (obj.dataType != ZCAN_DT_ZCAN_CAN_CANFD_DATA) {
                continue;
            }
            if (static_cast<int>(obj.chnl) != m_canIndex) {
                continue;
            }

            const QByteArray frame = encodeFrame(obj.data.zcanCANFDData);
            emit perfEvent(makePerfEvent(PerfStage::RxIoFirst, frame));
            emit dataReady(frame);
        }
    }
}

} // namespace CommIO
