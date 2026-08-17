#pragma once

#include <QtGlobal>

namespace CommIO {

class PerfClock {
public:
    static void initialize();
    static qint64 monotonicUs();
    static qint64 epochUs();
};

} // namespace CommIO
