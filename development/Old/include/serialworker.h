#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QTimer>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);

private:
    QSerialPort *m_serialPort;
    QByteArray m_buffer;       // 接收缓冲区
    QTimer *m_flushTimer;      // 延迟合并定时器
    bool m_useBufferMode;      // 是否启用缓冲合并

    signals:
        void serialClosed();
    void displayReceivedData(const QByteArray &data);
    void displaySentData(const QByteArray &data);

private slots:
    void onFlushBuffer();      // 定时器超时处理

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
    void setBufferMode(bool enabled);
};

#endif // SERIALWORKER_H
