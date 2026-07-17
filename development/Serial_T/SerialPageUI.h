// SerialPageUI.h
// 职责：创建 5 个页面的所有 Widget 和布局

#pragma once

#include <QObject>

class SerialPage;

class SerialPageUI : public QObject {
    Q_OBJECT
public:
    explicit SerialPageUI(SerialPage* page);

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();

private:
    SerialPage* m_page;
};
