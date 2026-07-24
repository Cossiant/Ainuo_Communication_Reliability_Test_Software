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
class ElaPushButton;
class ElaCheckBox;
class ElaLineEdit;
class QTextEdit;
class ElaComboBox;

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

    // ═════════════ 命令转换页面控件 ═════
    QTextEdit*      m_asciiEdit        = nullptr;  // ASCII 输入区（可编辑）
    QTextEdit*      m_hexEdit          = nullptr;  // HEX 输入区（可编辑）
    ElaPushButton*  m_asciiToHexBtn    = nullptr;  // ASCII → HEX
    ElaPushButton*  m_hexToAsciiBtn    = nullptr;  // HEX → ASCII
    ElaPushButton*  m_copyHexBtn       = nullptr;  // 复制HEX
    ElaPushButton*  m_clearBtn         = nullptr;  // 清空
    ElaCheckBox*    m_upperCaseCheck   = nullptr;  // 大写HEX
    ElaCheckBox*    m_spaceSepCheck    = nullptr;  // 空格分隔

    // ═════════════ AN3.0 字段解析控件 ═════
    ElaComboBox*    m_an30ProductCombo   = nullptr;  // 产品系列
    ElaComboBox*    m_an30CmdCombo       = nullptr;  // 命令码
    QTextEdit*      m_an30Input          = nullptr;  // HEX 字节区
    QTextEdit*      m_an30Output         = nullptr;  // 解析结果区
    ElaPushButton*  m_an30ParseBtn       = nullptr;  // 解析按钮
    ElaPushButton*  m_an30CopyBtn        = nullptr;  // 复制结果

    void parseAn30Fields();
    void onAn30ProductChanged(int index);
    void populateAn30CmdCombo();


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

    // ═════════════ 转换逻辑 ═════════
    void convertAsciiToHex();
    void convertHexToAscii();
};

#endif //UNTITLED_MAINPAGE_H
