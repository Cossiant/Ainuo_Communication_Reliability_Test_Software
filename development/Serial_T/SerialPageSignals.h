// SerialPageSignals.h
// 职责：线程创建 + 所有信号/槽连接

#pragma once

#include <QObject>

class SerialPage;

class SerialPageSignals : public QObject {
    Q_OBJECT
public:
    explicit SerialPageSignals(SerialPage* page);

private:
    void setupThreadAndWork();
    void connectAllSignals();

    SerialPage* m_page;
};
