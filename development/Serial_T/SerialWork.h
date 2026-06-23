//
// Created by Cossiant on 2026/6/22.
//

#ifndef UNTITLED_SERIALWORK_H
#define UNTITLED_SERIALWORK_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>

class SerialPage;
class ElaWindow;

class SerialWork : public QObject {
    Q_OBJECT

public:
    explicit SerialWork(SerialPage* serialPage, QObject *parent = nullptr);
    ~SerialWork();

    void sendString(const QString &text);
    void sendBytes(const QByteArray &data, const QString &displayText = QString());
    bool isOpen() const;
    void resetRecvCount();

    void setExpectedResponse(const QByteArray &expected);
    QByteArray expectedResponse() const { return m_expectedResponse; }

    signals:
        void responseReceived(QByteArray data);

private slots:
    void onOpenSerial();
    void onCloseSerial();
    void onReadyRead();
    void onBufferTimeout();

private:
    void updateUIForOpened(bool opened);

    void logSend(const QString &displayText);
    void logRecv(const QByteArray &data);

    SerialPage*  m_serialPage = nullptr;
    ElaWindow*   m_mainWindow = nullptr;
    QSerialPort* m_serialPort = nullptr;

    QByteArray m_recvBuffer;
    QTimer*    m_bufferTimer    = nullptr;
    int        m_totalRecv      = 0;

    QByteArray m_expectedResponse;
};

#endif //UNTITLED_SERIALWORK_H
