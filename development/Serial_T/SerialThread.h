#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QMutex>

class SerialThread : public QObject
{
    Q_OBJECT

public:
    explicit SerialThread(QObject *parent = nullptr);
    ~SerialThread();

public slots:
    void onSerialStart(QString portName,
                       qint32 baudRate,
                       QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity,
                       QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl);
    void onSerialStop();
    void setBufferMode(bool enabled);
    void sendData(const QByteArray &data);

    signals:
        void serialClosed();
    void serialOpened();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onReadyRead();

private:
    QSerialPort *m_serial     = nullptr;
    bool         m_bufferMode = false;
    QByteArray   m_buffer;
    QMutex       m_mutex;
};
