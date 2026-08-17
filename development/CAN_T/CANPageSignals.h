#pragma once

#include <QObject>

class CANPage;

class CANPageSignals : public QObject
{
    Q_OBJECT

public:
    explicit CANPageSignals(CANPage *page);

signals:
    void openCANRequested(int deviceType, int deviceIndex, int canIndex,
                          int canType, int abitBaud, int dbitBaud,
                          int canfdStandard, int terminalResistance,
                          quint32 accCode, quint32 accMask,
                          int filter, int mode);

private:
    void setupThreadAndWork();
    void connectAllSignals();

    CANPage *m_page = nullptr;
};
