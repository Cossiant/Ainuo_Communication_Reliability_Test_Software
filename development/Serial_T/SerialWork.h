// SerialWork.h
// 重写版：解耦 SerialPage，纯串口通信工作类
// 当前不启用多线程，所有操作在主线程执行；
// 未来只需 moveToThread + QThread 即可无缝迁移到多线程。

#ifndef UNTITLED_SERIALWORK_H
#define UNTITLED_SERIALWORK_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>

class SerialWork : public QObject
{
    Q_OBJECT

public:
    explicit SerialWork(QObject *parent = nullptr);
    ~SerialWork();

    // ─── 查询接口（线程安全，只读） ───
    bool isOpen() const;
    int  totalRecvCount() const;

    // ─── 设置接口 ───
    void resetRecvCount();
    void setExpectedResponse(const QByteArray &expected);
    QByteArray expectedResponse() const;
    void setHexDisplayMode(bool hexMode);      // 控制日志显示格式

    // ═══════════════════════════════════════════════════
    //  public slots —— 可被跨线程调用的操作
    // ═══════════════════════════════════════════════════
public slots:
    // 在线程内打开串口
    void openSerialPort(const QString &portName,
                        int baudRate,
                        QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity,
                        QSerialPort::StopBits stopBits,
                        bool buffered = true);

    // 在线程内关闭串口
    void closeSerialPort();

    // 发送原始字节（从主线程调用，内部在线程中执行 write）
    void sendData(const QByteArray &data);

    // 发送字符串（自动处理 HEX / ASCII）
    void sendString(const QString &text, bool hexMode);

    // ═══════════════════════════════════════════════════
    //  signals —— 通知外部（主线程 / UI）
    // ═══════════════════════════════════════════════════
signals:
    // 串口状态
    void serialOpened();                              // 串口已打开
    void serialClosed();                              // 串口已关闭
    void errorOccurred(const QString &errorMessage);  // 错误信息

    // 接收数据（原始字节）
    void dataReceived(const QByteArray &data);

    // Excel 比对专用（兼容旧接口）
    void responseReceived(const QByteArray &data);

    // 日志行（已格式化，可直接添加到 QListWidget）
    void sendLogLine(const QString &line);
    void recvLogLine(const QString &line);

    // 接收计数变化
    void recvCountChanged(int totalCount);

    // ═══════════════════════════════════════════════════
    //  private slots —— 仅在线程内部被触发
    // ═══════════════════════════════════════════════════
private slots:
    // 处理 readyRead 信号（在线程内被触发）
    void onReadyRead();

    // 处理串口错误
    void onSerialError(QSerialPort::SerialPortError error);

    // 缓冲区合并超时
    void onBufferTimeout();

private:
    // ─── 内部辅助 ───
    QString formatByteArray(const QByteArray &data) const;
    void emitData(const QByteArray &data);
    // ─── 成员 ───
    QSerialPort *m_serialPort  = nullptr;
    QTimer      *m_bufferTimer = nullptr;

    QByteArray m_recvBuffer;           // 缓冲区合并用
    bool       m_buffered = true;      // 是否启用缓冲合并

    int        m_totalRecv = 0;        // 总接收计数

    QByteArray m_expectedResponse;     // 期望的返回值（Excel 比对用）
    bool       m_hexDisplay = false;   // 显示格式：HEX / UTF-8
};

#endif // UNTITLED_SERIALWORK_H
