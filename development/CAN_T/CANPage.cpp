#include "CANPage.h"

#include "CANPageUI.h"
#include "CANPageSignals.h"
#include "CANErrorHandler.h"
#include "CANWork.h"
#include "CANExcel.h"
#include "../Other_T/ErrorLogExporter.h"
#include "ElaWindow.h"
#include "ElaIcon.h"

#include <QDebug>

CANPage::CANPage(ElaWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_ui = new CANPageUI(this);
    m_ui->createSettingsPage();
    m_ui->createSendPage();
    m_ui->createExcelSendPage();
    m_ui->createLogPage();
    m_ui->createErrorLogPage();

    initNavigation();
    initWindowConfig();

    m_signals = new CANPageSignals(this);
    m_errors = new CANErrorHandler(this);
    m_canFunc = new CANExcel(this, this);
}

CANPage::~CANPage()
{
    if (m_canWork) {
        QMetaObject::invokeMethod(m_canWork, "closeCANPort", Qt::QueuedConnection);
    }
    if (m_canThread) {
        m_canThread->quit();
        if (!m_canThread->wait(3000)) {
            qWarning() << "CANPage: 工作线程未能在 3 秒内退出，强制终止";
            m_canThread->terminate();
            m_canThread->wait();
        }
    }
}

void CANPage::initNavigation()
{
    m_mainWindow->addExpanderNode(QStringLiteral("CAN通讯"), CANMainPageKey, ElaIconType::CarBus);
    m_mainWindow->addPageNode(QStringLiteral("CAN设置"), _CANSettingPage, CANMainPageKey, ElaIconType::Gear);
    m_mainWindow->addPageNode(QStringLiteral("单条发送"), _CANSendPage, CANMainPageKey, ElaIconType::PaperPlane);
    m_mainWindow->addPageNode(QStringLiteral("表格发送"), _CANExcelSendPage, CANMainPageKey, ElaIconType::FileSpreadsheet);
    m_mainWindow->addPageNode(QStringLiteral("发送日志"), _CANLogPage, CANMainPageKey, ElaIconType::FileLines);
    m_mainWindow->addPageNode(QStringLiteral("错误统计"), _CANErrorLogPage, CANMainPageKey, ElaIconType::CircleExclamation);
}

void CANPage::initWindowConfig()
{
    m_mainWindow->setNavigationBarDisplayMode(ElaNavigationType::Auto);
    m_mainWindow->setNavigationBarWidth(300);
    m_mainWindow->setWindowButtonFlags(
        ElaAppBarType::NavigationButtonHint |
        ElaAppBarType::RouteBackButtonHint |
        ElaAppBarType::StayTopButtonHint |
        ElaAppBarType::ThemeChangeButtonHint |
        ElaAppBarType::MinimizeButtonHint |
        ElaAppBarType::MaximizeButtonHint |
        ElaAppBarType::CloseButtonHint);
    m_mainWindow->setIsAllowPageOpenInNewWindow(false);
}

void CANPage::addTimeoutError(const QString &command, const QString &expected)
{
    m_errors->addTimeoutError(command, expected);
}

void CANPage::addContentError(const QString &command,
                              const QString &expected,
                              const QString &actual)
{
    m_errors->addContentError(command, expected, actual);
}

void CANPage::clearErrors()
{
    m_errors->clearErrors();
}

void CANPage::clearSingleSendLog()
{
    if (m_singleSendLog) m_singleSendLog->clear();
    if (m_singleRecvLog) m_singleRecvLog->clear();
}

void CANPage::clearExcelSendLog()
{
    if (m_logSendList) m_logSendList->clear();
    if (m_logRecvList) m_logRecvList->clear();
    if (m_logSentCountCard) m_logSentCountCard->setValue("0");
}

void CANPage::exportErrorsToExcel()
{
    const QString defaultFileName = QStringLiteral("CAN错误统计_%1.xlsx")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    ErrorLogExporter::exportToExcel(m_mainWindow,
                                    QStringLiteral("导出CAN错误统计"),
                                    defaultFileName,
                                    m_errorTable);
}
