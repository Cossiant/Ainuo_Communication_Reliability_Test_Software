#ifndef RESPONSEVALIDATOR_H
#define RESPONSEVALIDATOR_H

#include <QObject>
#include <QQueue>
#include <QByteArray>
#include <QDebug>

class ResponseValidator : public QObject {
    Q_OBJECT

public:
    explicit ResponseValidator(QObject *parent = nullptr);

public slots:
    // 设置丢包测试模式
    void onSetTestPacketLossMode(bool enabled, bool onlySendDataMode);

    // 接收到新命令的期望返回值
    void onCommandSent(QByteArray expectedResponse);

    // 接收到实际数据，进行比对
    void onDataReceived(QByteArray data);

    // 重置队列和状态
    void onReset();

signals:
    // 比对发现不匹配，通知 GUI 增加错误计数
    void errorDetected(QByteArray expected, QByteArray actual);

private:
    QQueue<QByteArray> m_expectedQueue;
    bool m_testPacketLossMode = false;
    bool m_onlySendDataMode = false;
    QByteArray expected;
};

#endif // RESPONSEVALIDATOR_H
