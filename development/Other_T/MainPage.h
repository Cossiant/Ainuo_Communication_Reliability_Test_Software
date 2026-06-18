//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

class ElaWindow;
class ElaText;

#ifndef UNTITLED_MAINPAGE_H
#define UNTITLED_MAINPAGE_H

class MainPage : public QObject {
    Q_OBJECT
public:
    explicit MainPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~MainPage();

private:
    ElaWindow* m_mainWindow;

    // ═════════════ 页面 ═════════
    QWidget *_MainHomePage  = nullptr;
    QWidget *_MainHelpPage  = nullptr;
    QWidget *_MainAboutPage = nullptr;

    // ========== 分组 Key ==========
    QString MainPageKey;
    QString MainHelpKey;
    QString MainAboutKey;

    // ═════════════ 初始化方法 ═════════
    void initMainPage();
    void initNavigation();
    void initWindowConfig();

    void createHomePage();
    void createHelpPage();
    void createAboutPage();
};

#endif //UNTITLED_MAINPAGE_H
