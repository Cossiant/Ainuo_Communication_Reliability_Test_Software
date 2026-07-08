//
// Created by Cossiant on 2026/6/2.
//

#include "LED.h"

LED::LED()
{

}

void LED::setLED(QLabel *label, int color, int size)
{
    label->setText(""); // 清空文本

    QString bgColor;
    switch (color) {
        case 0: bgColor = "rgb(190,190,190)"; break;//灰色
        case 1: bgColor = "rgb(255,0,0)"; break;//红色
        case 2: bgColor = "rgb(0,255,0)"; break;//绿色
        case 3: bgColor = "rgb(255,255,0)"; break;//黄色
        default: bgColor = "rgb(255,255,255)"; break;//使用白色
    }

    QString style = QString(
        "min-width: %1px;"
        "min-height: %1px;"
        "max-width: %1px;"
        "max-height: %1px;"
        "border-radius: %2px;"
        "border: 1px solid black;"
        "background-color: %3;"
    ).arg(size).arg(size/2).arg(bgColor);

    label->setStyleSheet(style);
}
