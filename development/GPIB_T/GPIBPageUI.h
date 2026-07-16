// GPIBPageUI.h
// 职责：创建 5 个页面的所有 Widget 和布局

#pragma once

#include <QObject>

class GPIBPage;

class GPIBPageUI : public QObject {
    Q_OBJECT
public:
    explicit GPIBPageUI(GPIBPage* page);

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();

private:
    GPIBPage* m_page;
};
