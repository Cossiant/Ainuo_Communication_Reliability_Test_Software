//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>          // ← 改为继承 QObject
#include <QWidget>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCloseEvent>
class ElaWindow;            // ← 前向声明，不再 include 头文件
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaCheckBox;
class ElaLineEdit;
class ElaToggleSwitch;
#ifndef UNTITLED_SERIALPAGE_H
#define UNTITLED_SERIALPAGE_H

class SerialPage : public QObject {
    Q_OBJECT
public:
    explicit SerialPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~SerialPage();
private:
    ElaWindow* m_mainWindow;
    // ═════════════ 页面 ═════════
    QWidget *_SerialSettingPage = nullptr;
    QWidget *_SerialSendPage = nullptr;
    QWidget *_SerialExcelSendPage = nullptr;
    QWidget *_SerialLogPage = nullptr;
    QWidget *_SerialErrorLogPage = nullptr;

    // ========== 分组 Key ==========
    QString SerialMainPageKey;

    // ═════════════ 初始化方法 ═════════
    void initSerialPage();
    void initNavigation();
    void initwindowConfig();

    void createSettingsPage();
    void createSendPage();
    void createExcelSendPage();
    void createLogPage();
    void createErrorLogPage();
};


#endif //UNTITLED_SERIALPAGE_H
