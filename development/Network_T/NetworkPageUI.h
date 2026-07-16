// NetworkPageUI.h
// 职责：创建 5 个页面的所有 Widget 和布局

#pragma once

#include <QObject>

class NetworkPage;

class NetworkPageUI : public QObject {
    Q_OBJECT
public:
    explicit NetworkPageUI(NetworkPage* page);

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();

private:
    NetworkPage* m_page;
};
