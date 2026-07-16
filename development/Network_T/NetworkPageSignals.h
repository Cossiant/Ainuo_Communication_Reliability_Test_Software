// NetworkPageSignals.h
// 职责：线程创建 + 所有信号/槽连接

#pragma once

#include <QObject>

class NetworkPage;

class NetworkPageSignals : public QObject {
    Q_OBJECT
public:
    explicit NetworkPageSignals(NetworkPage* page);

private:
    void setupThreadAndWork();
    void connectAllSignals();

    NetworkPage* m_page;
};
