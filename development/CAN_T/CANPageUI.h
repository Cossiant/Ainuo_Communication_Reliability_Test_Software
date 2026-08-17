#pragma once

#include <QObject>

class CANPage;

class CANPageUI : public QObject
{
    Q_OBJECT

public:
    explicit CANPageUI(CANPage *page);

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();

private:
    CANPage *m_page = nullptr;
};
