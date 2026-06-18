//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

class ElaWindow;
class ElaText;

#ifndef UNTITLED_CANPAGE_H
#define UNTITLED_CANPAGE_H

class CANPage : public QObject {
    Q_OBJECT
public:
    explicit CANPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~CANPage();

private:
    ElaWindow* m_mainWindow;

    QWidget *_CANSettingPage   = nullptr;
    QWidget *_CANSendPage      = nullptr;
    QWidget *_CANExcelSendPage = nullptr;
    QWidget *_CANLogPage       = nullptr;
    QWidget *_CANErrorLogPage  = nullptr;

    QString CANMainPageKey;

    void initCANPage();
    void initNavigation();
    void initwindowConfig();

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();
};

#endif //UNTITLED_CANPAGE_H
