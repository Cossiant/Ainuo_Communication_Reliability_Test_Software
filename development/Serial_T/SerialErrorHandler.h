// SerialErrorHandler.h
// 职责：错误记录与统计

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

class SerialPage;

class SerialErrorHandler : public QObject {
    Q_OBJECT
public:
    explicit SerialErrorHandler(SerialPage* page);

    void addTimeoutError(const QString &command, const QByteArray &expected);
    void addContentError(const QString &command,
                         const QByteArray &expected,
                         const QByteArray &actual);
    void clearErrors();

private:
    QString bytesToDisplayText(const QByteArray &data, bool isHexMode);

    SerialPage* m_page;
};
