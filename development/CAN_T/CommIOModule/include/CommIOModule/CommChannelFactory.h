#pragma once

#include <QObject>

#include "CommTypes.h"

namespace CommIO {

class ICommChannel;

// 通道工厂：根据类型创建具体通道实例
class CommChannelFactory {
public:
    static ICommChannel* createChannel(ChannelType type, QObject* parent = nullptr);
};

} // namespace CommIO
