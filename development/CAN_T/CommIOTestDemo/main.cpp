#include <QCoreApplication>
#include <QTextStream>
#include <QThread>

#include "CommIOModule/CommTypes.h"
#include "CommIOModule/CommConfig.h"
#include "CommIOModule/ICommChannel.h"
#include "CommIOModule/CommChannelFactory.h"

using namespace CommIO;

static QTextStream qout(stdout);
static bool g_isCANMode = false;  // CAN 模式标志（影响收发数据格式）

// === Stdin 读取线程 ===
// Windows 控制台的 stdin 读取是阻塞的，必须放在独立线程中
// 否则会阻塞 Qt 事件循环，导致信号槽无法执行
class StdinReader : public QThread {
    Q_OBJECT
public:
    explicit StdinReader(QObject* parent = nullptr) : QThread(parent) {}

signals:
    void lineReady(const QString& line);

protected:
    void run() override {
        QTextStream qin(stdin);
        while (!isInterruptionRequested()) {
            QString line = qin.readLine();
            if (line.isNull()) break; // EOF
            emit lineReady(line.trimmed());
        }
    }
};

// === CAN 帧格式化工具 ===

// 将 CAN ID + 数据 HEX 字符串 → QByteArray (前4字节ID大端 + 数据)
static QByteArray buildCANFrame(quint32 canId, const QByteArray& data)
{
    QByteArray frame;
    frame.append(static_cast<char>((canId >> 24) & 0xFF));
    frame.append(static_cast<char>((canId >> 16) & 0xFF));
    frame.append(static_cast<char>((canId >> 8)  & 0xFF));
    frame.append(static_cast<char>(canId & 0xFF));
    frame.append(data.left(8)); // CAN 最多 8 字节
    return frame;
}

// 将接收到的 QByteArray → 可读的 CAN 帧显示
static QString formatCANFrame(const QByteArray& raw)
{
    if (raw.size() < 4) {
        return raw.toHex(' ').toUpper();
    }

    quint32 canId = (static_cast<unsigned char>(raw[0]) << 24) |
                    (static_cast<unsigned char>(raw[1]) << 16) |
                    (static_cast<unsigned char>(raw[2]) << 8)  |
                    (static_cast<unsigned char>(raw[3]));

    QByteArray data = raw.mid(4);

    return QStringLiteral("ID=0x%1  DLC=%2  Data=[%3]")
        .arg(canId, 3, 16, QChar('0')).toUpper()
        .arg(data.size())
        .arg(QString(data.toHex(' ').toUpper()));
}

// 通用信号连接：接收数据、错误、状态变化
static void connectSignals(ICommChannel* channel)
{
    QObject::connect(channel, &ICommChannel::dataReceived,
                     [](const QByteArray& data) {
        if (g_isCANMode) {
            // CAN 模式：格式化为 ID + DLC + Data
            qout << QStringLiteral("\n[接收] ") << formatCANFrame(data);
        } else {
            qout << QStringLiteral("\n[接收] (%1 字节): ").arg(data.size());
            bool isPrintable = true;
            for (char c : data) {
                if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') {
                    isPrintable = false;
                    break;
                }
            }
            if (isPrintable) {
                qout << QString::fromUtf8(data).trimmed();
            } else {
                qout << data.toHex(' ').toUpper();
            }
        }
        qout << "\n> " << Qt::flush;
    });

    QObject::connect(channel, &ICommChannel::errorOccurred,
                     [](ErrorType type, const QString& msg) {
        qout << QStringLiteral("\n[错误] %1 - %2")
                    .arg(ErrorHelper::errorTypeToString(type), msg)
             << "\n> " << Qt::flush;
    });

    QObject::connect(channel, &ICommChannel::connectionStateChanged,
                     [](bool connected) {
        qout << QStringLiteral("\n[状态] %1")
                    .arg(connected ? QStringLiteral("已连接") : QStringLiteral("已断开"))
             << "\n> " << Qt::flush;
    });
}

// === 通用交互模式 (串口/TCP/GPIB) ===
static void startInteractive(ICommChannel* channel, StdinReader* reader)
{
    qout << QStringLiteral("\n--- 进入交互模式 ---") << Qt::endl;
    qout << QStringLiteral("输入要发送的数据 (文本模式, 自动追加 \\n)") << Qt::endl;
    qout << QStringLiteral("输入 'hex:XX XX' 发送 HEX 数据") << Qt::endl;
    qout << QStringLiteral("输入 'quit' 退出") << Qt::endl;
    qout << "> " << Qt::flush;

    QObject::connect(reader, &StdinReader::lineReady,
                     channel, [channel](const QString& line) {
        if (line.isEmpty()) return;

        if (line.toLower() == "quit") {
            qout << QStringLiteral("正在关闭...") << Qt::endl;
            channel->close();
            QCoreApplication::quit();
            return;
        }

        QByteArray dataToSend;
        if (line.startsWith("hex:", Qt::CaseInsensitive)) {
            QString hexStr = line.mid(4).trimmed();
            dataToSend = QByteArray::fromHex(hexStr.toLatin1());
        } else {
            dataToSend = (line + "\n").toUtf8();
        }

        qout << QStringLiteral("[发送] (%1 字节): %2")
                    .arg(dataToSend.size())
                    .arg(QString::fromUtf8(dataToSend).trimmed())
             << Qt::endl;
        qout << "> " << Qt::flush;
        channel->send(dataToSend);
    });
}

// === CAN 专用交互模式 ===
static void startCANInteractive(ICommChannel* channel, StdinReader* reader)
{
    g_isCANMode = true;

    qout << QStringLiteral("\n--- 进入 CAN 交互模式 ---") << Qt::endl;
    qout << QStringLiteral("发送格式: <CAN_ID> <HEX数据>") << Qt::endl;
    qout << QStringLiteral("  示例: 100 11 22 33 44     → ID=0x100, Data=11 22 33 44") << Qt::endl;
    qout << QStringLiteral("  示例: 7FF AA BB            → ID=0x7FF, Data=AA BB") << Qt::endl;
    qout << QStringLiteral("  示例: 1FFFFFFF 01 02 03 04 → 扩展帧 ID=0x1FFFFFFF") << Qt::endl;
    qout << QStringLiteral("输入 'quit' 退出") << Qt::endl;
    qout << "> " << Qt::flush;

    QObject::connect(reader, &StdinReader::lineReady,
                     channel, [channel](const QString& line) {
        if (line.isEmpty()) return;

        if (line.toLower() == "quit") {
            qout << QStringLiteral("正在关闭...") << Qt::endl;
            channel->close();
            QCoreApplication::quit();
            return;
        }

        // 解析格式: <CAN_ID_HEX> <DATA_HEX...>
        // 第一个空格前为 CAN ID (十六进制), 后面为数据字节 (十六进制, 空格分隔)
        int firstSpace = line.indexOf(' ');
        QString idStr;
        QString dataStr;

        if (firstSpace > 0) {
            idStr = line.left(firstSpace).trimmed();
            dataStr = line.mid(firstSpace + 1).trimmed();
        } else {
            // 只有 ID，没有数据 (例如远程帧)
            idStr = line.trimmed();
        }

        bool ok = false;
        quint32 canId = idStr.toUInt(&ok, 16);
        if (!ok) {
            qout << QStringLiteral("[错误] CAN ID 格式无效: %1 (应为十六进制, 如 100, 7FF)")
                        .arg(idStr) << Qt::endl;
            qout << "> " << Qt::flush;
            return;
        }

        // 标准帧 ID 范围检查 (0-0x7FF)
        if (canId > 0x7FF && canId <= 0x1FFFFFFF) {
            qout << QStringLiteral("  (使用扩展帧 ID=0x%1)")
                        .arg(canId, 8, 16, QChar('0')).toUpper() << Qt::endl;
        } else if (canId > 0x1FFFFFFF) {
            qout << QStringLiteral("[错误] CAN ID 超出范围: 0x%1 (最大 0x1FFFFFFF)")
                        .arg(canId, 8, 16, QChar('0')) << Qt::endl;
            qout << "> " << Qt::flush;
            return;
        }

        QByteArray data = QByteArray::fromHex(dataStr.remove(' ').toLatin1());
        if (data.size() > 8) {
            qout << QStringLiteral("[警告] CAN 数据超过 8 字节, 已截断") << Qt::endl;
            data = data.left(8);
        }

        QByteArray frame = buildCANFrame(canId, data);

        qout << QStringLiteral("[发送] ID=0x%1  DLC=%2  Data=[%3]")
                    .arg(canId, 3, 16, QChar('0')).toUpper()
                    .arg(data.size())
                    .arg(QString(data.toHex(' ').toUpper()))
             << Qt::endl;
        qout << "> " << Qt::flush;
        channel->send(frame);
    });
}

// ============== 各接口测试 ==============

// 同步读取一行（进入事件循环前使用）
static QString readLine()
{
    QTextStream qin(stdin);
    return qin.readLine().trimmed();
}

static void testSerialPort(StdinReader* reader)
{
    qout << QStringLiteral("\n========== 串口测试 (RS232/RS485/USB) ==========") << Qt::endl;
    qout << QStringLiteral("端口名称 (如 COM1): ") << Qt::flush;
    QString portName = readLine();

    qout << QStringLiteral("波特率 [9600]: ") << Qt::flush;
    QString baudStr = readLine();
    int baudRate = baudStr.isEmpty() ? 9600 : baudStr.toInt();

    auto* channel = CommChannelFactory::createChannel(ChannelType::SerialPort);
    connectSignals(channel);

    QVariantMap config;
    config["portName"] = portName;
    config["baudRate"] = baudRate;
    config["dataBits"] = QSerialPort::Data8;
    config["parity"] = QSerialPort::NoParity;
    config["stopBits"] = QSerialPort::OneStop;
    config["flowControl"] = QSerialPort::NoFlowControl;

    qout << QStringLiteral("正在打开 %1 @ %2 bps...").arg(portName).arg(baudRate) << Qt::endl;
    if (channel->open(config)) {
        qout << QStringLiteral("串口打开成功!") << Qt::endl;
        startInteractive(channel, reader);
    } else {
        qout << QStringLiteral("串口打开失败!") << Qt::endl;
        delete channel;
        QCoreApplication::quit();
    }
}

static void testTCP(StdinReader* reader)
{
    qout << QStringLiteral("\n========== TCP Client 测试 ==========") << Qt::endl;
    qout << QStringLiteral("主机地址 (如 192.168.1.100): ") << Qt::flush;
    QString host = readLine();

    qout << QStringLiteral("端口号 [5025]: ") << Qt::flush;
    QString portStr = readLine();
    int port = portStr.isEmpty() ? 5025 : portStr.toInt();

    auto* channel = CommChannelFactory::createChannel(ChannelType::TCP);
    connectSignals(channel);

    QVariantMap config;
    config["host"] = host;
    config["port"] = port;
    config["connectTimeout"] = 5000;

    qout << QStringLiteral("正在连接 %1:%2...").arg(host).arg(port) << Qt::endl;
    channel->open(config);
    startInteractive(channel, reader);
}

static void testGPIB(StdinReader* reader)
{
    qout << QStringLiteral("\n========== GPIB 测试 (NI-VISA) ==========") << Qt::endl;
    qout << QStringLiteral("GPIB 板卡号 [0]: ") << Qt::flush;
    QString boardStr = readLine();
    int boardIndex = boardStr.isEmpty() ? 0 : boardStr.toInt();

    qout << QStringLiteral("GPIB 主地址 (0-30): ") << Qt::flush;
    int primaryAddr = readLine().toInt();

    auto* channel = CommChannelFactory::createChannel(ChannelType::GPIB);
    connectSignals(channel);

    QVariantMap config;
    config["boardIndex"] = boardIndex;
    config["primaryAddress"] = primaryAddr;
    config["timeout"] = 5000;
    config["termCharEnabled"] = true;
    config["termChar"] = '\n';
    config["sendEndEnabled"] = true;

    qout << QStringLiteral("正在打开 GPIB%1::%2::INSTR...").arg(boardIndex).arg(primaryAddr) << Qt::endl;
    channel->open(config);
    startInteractive(channel, reader);
}

static void testCAN(StdinReader* reader)
{
    qout << QStringLiteral("\n========== CAN 测试 (周立功 ControlCAN) ==========") << Qt::endl;
    qout << QStringLiteral("设备类型 [4=USBCAN2]: ") << Qt::flush;
    QString typeStr = readLine();
    int deviceType = typeStr.isEmpty() ? 4 : typeStr.toInt();

    qout << QStringLiteral("设备索引 [0]: ") << Qt::flush;
    QString idxStr = readLine();
    int deviceIndex = idxStr.isEmpty() ? 0 : idxStr.toInt();

    qout << QStringLiteral("CAN 通道 [0]: ") << Qt::flush;
    QString chStr = readLine();
    int canIndex = chStr.isEmpty() ? 0 : chStr.toInt();

    qout << QStringLiteral("波特率 bps [500000]: ") << Qt::flush;
    QString baudStr = readLine();
    int baudRate = baudStr.isEmpty() ? 500000 : baudStr.toInt();

    auto* channel = CommChannelFactory::createChannel(ChannelType::CAN);
    connectSignals(channel);

    QVariantMap config;
    config["deviceType"] = deviceType;
    config["deviceIndex"] = deviceIndex;
    config["canIndex"] = canIndex;
    config["baudRate"] = baudRate;
    config["accCode"] = 0x00000000;
    config["accMask"] = 0xFFFFFFFF;
    config["filter"] = 0;
    config["mode"] = 0;

    qout << QStringLiteral("正在打开 CAN (类型=%1, 索引=%2, 通道=%3, %4 bps)...")
                .arg(deviceType).arg(deviceIndex).arg(canIndex).arg(baudRate)
         << Qt::endl;
    channel->open(config);
    startCANInteractive(channel, reader);  // CAN 专用交互模式
}

// ============== 主菜单 ==============

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qout << QStringLiteral("================================================") << Qt::endl;
    qout << QStringLiteral("  1CommIOModule 测试 Demo") << Qt::endl;
    qout << QStringLiteral("  1六合一通信接口测试工具") << Qt::endl;
    qout << QStringLiteral("================================================") << Qt::endl;
    qout << Qt::endl;
    qout << QStringLiteral("请选择要测试的接口:") << Qt::endl;
    qout << QStringLiteral("  1. RS232/RS485/USB (串口)") << Qt::endl;
    qout << QStringLiteral("  2. TCP Client (LAN)") << Qt::endl;
    qout << QStringLiteral("  3. GPIB (NI-VISA)") << Qt::endl;
    qout << QStringLiteral("  4. CAN (周立功 ControlCAN)") << Qt::endl;
    qout << QStringLiteral("  0. 退出") << Qt::endl;
    qout << QStringLiteral("\n选择 [1-4]: ") << Qt::flush;

    // 菜单和配置输入在主线程中同步读取（此时事件循环未启动，可以阻塞）
    QString choice = readLine();

    // 创建 stdin 读取线程（进入交互模式后才需要）
    StdinReader reader;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &reader, [&reader]() {
        reader.requestInterruption();
        reader.wait(1000);
    });

    switch (choice.toInt()) {
    case 1: testSerialPort(&reader); break;
    case 2: testTCP(&reader);        break;
    case 3: testGPIB(&reader);       break;
    case 4: testCAN(&reader);        break;
    case 0:
        qout << QStringLiteral("退出.") << Qt::endl;
        return 0;
    default:
        qout << QStringLiteral("无效选择!") << Qt::endl;
        return 1;
    }

    // 启动 stdin 读取线程，然后进入事件循环
    reader.start();
    return app.exec();
}

// moc 需要在 .cpp 中包含此文件（因为 Q_OBJECT 定义在 .cpp 中）
#include "main.moc"
