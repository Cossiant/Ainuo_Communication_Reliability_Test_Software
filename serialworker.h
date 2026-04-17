#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);

private:
    QSerialPort *m_serialPort;

signals:
    void serialClosed();
    void displayReceivedData(const QByteArray &data);
    void displaySentData(const QByteArray &data);

public slots:
    void writeData(const QByteArray &data);
    void onSerialStart(const QString &portName,
                       qint32 baudRate,
                       QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity,
                       QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl);
    void onSerialStop();
    void handleReadyRead();
};

#endif // SERIALWORKER_H
