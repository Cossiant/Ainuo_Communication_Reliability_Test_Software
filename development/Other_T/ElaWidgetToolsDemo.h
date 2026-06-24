#pragma once

#include "ElaWindow.h"

#include "../Serial_T/SerialPage.h"
#include "../Network_T/NetworkPage.h"
#include "../CAN_T/CANPage.h"
#include "../GPIB_T/GPIBPage.h"
#include "../USER_T/USERPage.h"
#include "MainPage.h"

class ElaWidgetToolsDemo : public ElaWindow
{
    Q_OBJECT

public:
    ElaWidgetToolsDemo(QWidget *parent = nullptr);
    ~ElaWidgetToolsDemo();

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    SerialPage*  m_serialPage  = nullptr;
    NetworkPage* m_networkPage = nullptr;
    CANPage*     m_CANPage     = nullptr;
    GPIBPage*    m_GPIBPage    = nullptr;
    USERPage*    m_USERPage    = nullptr;
    MainPage*    m_mainPage    = nullptr;
};
