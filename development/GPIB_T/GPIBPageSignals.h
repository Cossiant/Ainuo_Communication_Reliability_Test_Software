// GPIBPageSignals.h
// 职责：线程创建 + 所有信号/槽连接

#pragma once

#include <QObject>

class GPIBPage;

class GPIBPageSignals : public QObject {
    Q_OBJECT
public:
    explicit GPIBPageSignals(GPIBPage* page);

private:
    void setupThreadAndWork();
    void connectAllSignals();

    GPIBPage* m_page;
};
