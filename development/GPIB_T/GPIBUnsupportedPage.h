// GPIBUnsupportedPage.h
// 无 GPIB 版本：保留 GPIB 入口，但仅显示不支持提示。
// 本文件不依赖任何 NI-VISA / NI-488.2 驱动头文件或库。

#pragma once

#include <QObject>
#include <QWidget>

class ElaWindow;

class GPIBUnsupportedPage : public QObject {
    Q_OBJECT
public:
    explicit GPIBUnsupportedPage(ElaWindow *mainWindow, QObject *parent = nullptr);
    ~GPIBUnsupportedPage() override;

private:
    ElaWindow *m_mainWindow = nullptr;
};
