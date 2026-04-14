#include "serialworker.h"

SerialWorker::SerialWorker()
{
}

//初始化
void SerialWorker::onSerialStart(const QString &portName, qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits,
                                 QSerialPort::FlowControl flowControl)
{
    qDebug() << "开始初始化";
    m_serialPort = new QSerialPort;
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(flowControl);
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "完成初始化！";
    } else {
        qDebug() << "无法打开串口：" << m_serialPort->errorString();
    }

}

//收到停止信号后，需要清理退出
void SerialWorker::onSerialStop()
{
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
            qDebug() << "串口已关闭";
        }
        m_serialPort->deleteLater();  // 安全删除对象
        m_serialPort = nullptr;
    }
    // 发出关闭信号，告知 GUI
    emit serialClosed();
}

//发送函数
void SerialWorker::writeData(const QString &data)
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->write(data.toUtf8());
        // 可选：记录发送日志
        qDebug() << "串口发送:" << data;
    } else {
        qWarning() << "串口未打开，无法发送数据";
    }
}
