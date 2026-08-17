#include "CommIOModule/PerfClock.h"

#include <QElapsedTimer>
#include <QDateTime>

namespace CommIO {

namespace {

struct PerfClockState {
    QElapsedTimer timer;
    qint64 epochStartUs = 0;

    PerfClockState()
    {
        timer.start();
        epochStartUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    }
};

PerfClockState& state()
{
    static PerfClockState s;
    return s;
}

} // namespace

void PerfClock::initialize()
{
    Q_UNUSED(state());
}

qint64 PerfClock::monotonicUs()
{
    return state().timer.nsecsElapsed() / 1000;
}

qint64 PerfClock::epochUs()
{
    return state().epochStartUs + monotonicUs();
}



} // namespace CommIO
