//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCloseEvent>

class ElaWindow;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaCheckBox;
class ElaLineEdit;
class ElaToggleSwitch;

#ifndef UNTITLED_NETWORKPAGE_H
#define UNTITLED_NETWORKPAGE_H

class NetworkPage : public QObject {
    Q_OBJECT
public:
    explicit NetworkPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~NetworkPage();

private:
    ElaWindow* m_mainWindow;

    // ═════════════ 页面 ═════════
    QWidget *_NetworkSettingPage   = nullptr;
    QWidget *_NetworkSendPage      = nullptr;
    QWidget *_NetworkExcelSendPage = nullptr;
    QWidget *_NetworkLogPage       = nullptr;
    QWidget *_NetworkErrorLogPage  = nullptr;

    // ========== 分组 Key ==========
    QString NetworkMainPageKey;

    // ═════════════ 初始化方法 ═════════
    void initNetworkPage();
    void initNavigation();
    void initwindowConfig();

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();
};

#endif //UNTITLED_NETWORKPAGE_H
