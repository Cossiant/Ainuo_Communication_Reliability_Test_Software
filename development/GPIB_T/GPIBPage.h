//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

class ElaWindow;
class ElaText;

#ifndef UNTITLED_GPIBPAGE_H
#define UNTITLED_GPIBPAGE_H

class GPIBPage : public QObject {
    Q_OBJECT
public:
    explicit GPIBPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~GPIBPage();

private:
    ElaWindow* m_mainWindow;

    QWidget *_GPIBSettingPage   = nullptr;
    QWidget *_GPIBSendPage      = nullptr;
    QWidget *_GPIBExcelSendPage = nullptr;
    QWidget *_GPIBLogPage       = nullptr;
    QWidget *_GPIBErrorLogPage  = nullptr;

    QString GPIBMainPageKey;

    void initGPIBPage();
    void initNavigation();
    void initwindowConfig();

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();
};

#endif //UNTITLED_GPIBPAGE_H
