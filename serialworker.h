#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QThread>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    SerialWorker();
private:
    QSerialPort *m_serialPort;      //串口变量

signals:
    void serialClosed();
    void displayReceivedData(QString msg);       //接收到的数据显示信号量
    void displaySentData(QString msg);           //发送过去的数据显示信号量
public slots:
    void writeData(const QString &data);
    void onSerialStart(const QString &portName,
                       qint32 baudRate,
                       QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity,
                       QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl);     //串口启动槽
    void onSerialStop();
    void handleReadyRead();                      // 串口数据到达时被调用
};

#endif // SERIALWORKER_H
