#pragma once

#include <QObject>
#include <QThread>
#include <QSerialPort>

class SerialPage;
class SerialThread;
class ElaWindow;

class SerialWork : public QObject
{
    Q_OBJECT

public:
    explicit SerialWork(SerialPage *serialPage, QObject *parent = nullptr);
    ~SerialWork();

    bool isSerialOpen() const;
    void sendData(const QByteArray &data);

private slots:
    void onOpenSerial();
    void onCloseSerial();
    void onSerialClosed();
    void onWorkerError(const QString &errorMessage);

private:
    void cleanupThread();
    void updateUIForOpened(bool opened);

    SerialPage  *m_serialPage = nullptr;
    ElaWindow   *m_mainWindow = nullptr;

    QThread      *m_thread   = nullptr;
    SerialThread *m_worker   = nullptr;    // ← 类型改为 SerialThread

    bool          m_isOpening = false;
};
