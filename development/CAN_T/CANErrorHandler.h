#pragma once

#include <QObject>
#include <QString>

class CANPage;

class CANErrorHandler : public QObject
{
    Q_OBJECT

public:
    explicit CANErrorHandler(CANPage *page);

    void addTimeoutError(const QString &command, const QString &expected);
    void addContentError(const QString &command,
                         const QString &expected,
                         const QString &actual);
    void clearErrors();

private:
    CANPage *m_page = nullptr;
};
