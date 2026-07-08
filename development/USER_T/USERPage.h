//
// Created by Cossiant on 2026/6/18.
//
#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

class ElaWindow;
class ElaText;

#ifndef UNTITLED_USERPAGE_H
#define UNTITLED_USERPAGE_H

class USERPage : public QObject {
    Q_OBJECT
public:
    explicit USERPage(ElaWindow* mainWindow, QObject *parent = nullptr);
    ~USERPage();

private:
    ElaWindow* m_mainWindow;

    QWidget *_USERGuidePage    = nullptr;
    QWidget *_USERVersionPage  = nullptr;

    QString USERMainPageKey;

    void initUSERPage();
    void initNavigation();
    void initwindowConfig();

    void createGuidePage();
    void createVersionPage();
};

#endif //UNTITLED_USERPAGE_H
