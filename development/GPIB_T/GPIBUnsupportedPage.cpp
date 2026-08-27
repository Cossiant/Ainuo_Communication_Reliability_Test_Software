// GPIBUnsupportedPage.cpp
// 无 GPIB 版本：仅显示不支持提示，不加载任何 NI-VISA / GPIB 驱动相关代码。

#include "GPIBUnsupportedPage.h"

#include <QVBoxLayout>

#include "ElaWindow.h"
#include "ElaIcon.h"
#include "ElaText.h"

GPIBUnsupportedPage::GPIBUnsupportedPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    QWidget *page = new QWidget();
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(40, 40, 40, 40);
    lay->setSpacing(20);

    ElaText *title = new ElaText("GPIB 通讯");
    title->setTextPixelSize(28);
    title->setTextStyle(ElaTextType::Title);
    lay->addWidget(title);

    ElaText *tip = new ElaText(
        QString::fromUtf8("此版本不支持 GPIB，如需要支持 GPIB 的版本请联系开发人员。"));
    tip->setTextPixelSize(16);
    tip->setWordWrap(true);
    lay->addWidget(tip);

    lay->addStretch();

    m_mainWindow->addPageNode("GPIB通讯", page, ElaIconType::Microchip);
}

GPIBUnsupportedPage::~GPIBUnsupportedPage() = default;
