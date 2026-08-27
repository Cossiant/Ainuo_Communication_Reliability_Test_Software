#include "CommIOModule/CommChannelFactory.h"
#include "CommIOModule/ICommChannel.h"
#include "CommIOModule/SerialPortChannel.h"
#include "CommIOModule/TCPChannel.h"

#include "CommIOModule/CANChannel.h"

namespace CommIO {

ICommChannel* CommChannelFactory::createChannel(ChannelType type, QObject* parent)
{
    switch (type) {
    case ChannelType::SerialPort:
        return new SerialPortChannel(parent);
    case ChannelType::TCP:
        return new TCPChannel(parent);


    case ChannelType::CAN:
        return new CANChannel(parent);
    default:
        return nullptr;
    }
}

} // namespace CommIO
