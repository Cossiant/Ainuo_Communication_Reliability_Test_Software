//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>

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
    QWidget *_MainCommandCalculationPage = nullptr;
    QWidget *_MainHelpPage  = nullptr;
    QWidget *_MainAboutPage = nullptr;

    // ========== 分组 Key ==========
    QString MainPageKey;
    QString CommandCalculationPageKey;
    QString MainHelpKey;
    QString MainAboutKey;

    // ═════════════ 初始化方法 ═════════
    void initMainPage();
    void initNavigation();
    void initWindowConfig();

    void createHomePage();
    void createCommandCalculationPage();
    void createHelpPage();
    void createAboutPage();

    // ═════════════ 辅助方法 ═════════
    QWidget* createHelpSection(const QString &title, const QStringList &lines) const;
};

#endif //UNTITLED_MAINPAGE_H
