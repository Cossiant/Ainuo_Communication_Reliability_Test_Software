#ifndef LED_H
#define LED_H

#include <QObject>
#include <QLabel>
#include <QString>

class LED
{
public:
    LED();

    static void setLED(QLabel *label,int color,int size);
};

#endif // LED_H
