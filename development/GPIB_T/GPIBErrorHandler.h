// GPIBErrorHandler.h
// 职责：错误记录与统计

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

class GPIBPage;

class GPIBErrorHandler : public QObject {
    Q_OBJECT
public:
    explicit GPIBErrorHandler(GPIBPage* page);

    void addTimeoutError(const QString &command, const QByteArray &expected);
    void addContentError(const QString &command,
                         const QByteArray &expected,
                         const QByteArray &actual);
    void clearErrors();

private:
    QString bytesToDisplayText(const QByteArray &data, bool isHexMode);

    GPIBPage* m_page;
};
