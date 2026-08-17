#include "CANFrameUtils.h"

#include <QStringList>

namespace CANFrameUtils {

namespace {
constexpr quint32 kExtFlag = 0x80000000U;

// 与 ZLGCAN canframe.h 保持一致
constexpr int kClassicMaxLen = 8;
constexpr int kFdMaxLen = 64;

enum FrameFlagByte {
    FlagFd = 1 << 0,
    FlagBrs = 1 << 1,
    FlagExt = 1 << 2
};
} // namespace

bool parseFrameText(const QString &text,
                    quint32 &canIdRaw,
                    QByteArray &data,
                    bool &isFd,
                    bool &brs,
                    QString *errorMessage)
{
    const QStringList tokens = text.split(' ', QString::SkipEmptyParts);
    if (tokens.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("CAN 帧内容为空");
        return false;
    }

    bool ok = false;
    const quint32 id = tokens.at(0).toUInt(&ok, 16);
    if (!ok || id > 0x1FFFFFFF) {
        if (errorMessage) *errorMessage = QStringLiteral("CAN ID 格式无效：%1").arg(tokens.at(0));
        return false;
    }

    int dataStart = 1;
    bool fd = false;
    if (tokens.size() > 1
        && tokens.at(1).compare(QStringLiteral("FD"), Qt::CaseInsensitive) == 0) {
        fd = true;
        dataStart = 2;
    }

    QString hexText;
    for (int i = dataStart; i < tokens.size(); ++i) {
        hexText += tokens.at(i);
    }
    QByteArray parsedData = QByteArray::fromHex(hexText.toLatin1());
    if (!hexText.isEmpty() && parsedData.size() != hexText.size() / 2) {
        if (errorMessage) *errorMessage = QStringLiteral("CAN 数据包含非法 HEX 字符");
        return false;
    }

    if (!fd && parsedData.size() > kClassicMaxLen) {
        fd = true;
    }
    if (fd && parsedData.size() > kFdMaxLen) {
        if (errorMessage) *errorMessage = QStringLiteral("CAN FD 数据超过 64 字节");
        return false;
    }

    canIdRaw = id | (id > 0x7FF ? kExtFlag : 0);
    data = parsedData;
    isFd = fd;
    brs = fd;   // FD 帧默认开启 BRS
    return true;
}

QByteArray encodeFrame(quint32 canIdRaw,
                       const QByteArray &data,
                       bool isFd,
                       bool brs)
{
    QByteArray frame;
    frame.reserve(5 + data.size());

    quint8 flags = 0;
    if (isFd) flags |= FlagFd;
    if (brs) flags |= FlagBrs;
    if (canIdRaw & kExtFlag) flags |= FlagExt;
    frame.append(static_cast<char>(flags));

    frame.append(static_cast<char>((canIdRaw >> 24) & 0xFF));
    frame.append(static_cast<char>((canIdRaw >> 16) & 0xFF));
    frame.append(static_cast<char>((canIdRaw >> 8) & 0xFF));
    frame.append(static_cast<char>(canIdRaw & 0xFF));
    frame.append(data);
    return frame;
}

bool decodeFrame(const QByteArray &frame,
                 quint32 &canIdRaw,
                 QByteArray &data,
                 bool &isFd,
                 bool &brs)
{
    if (frame.size() < 5) {
        return false;
    }

    const quint8 flags = static_cast<unsigned char>(frame.at(0));
    canIdRaw = (static_cast<unsigned char>(frame.at(1)) << 24)
             | (static_cast<unsigned char>(frame.at(2)) << 16)
             | (static_cast<unsigned char>(frame.at(3)) << 8)
             |  static_cast<unsigned char>(frame.at(4));
    data = frame.mid(5);
    isFd = (flags & FlagFd) != 0;
    brs = (flags & FlagBrs) != 0;
    return true;
}

QString formatFrame(const QByteArray &frame)
{
    quint32 canIdRaw = 0;
    QByteArray data;
    bool isFd = false;
    bool brs = false;
    if (!decodeFrame(frame, canIdRaw, data, isFd, brs)) {
        return QStringLiteral("HEX=[%1]").arg(QString(frame.toHex(' ').toUpper()));
    }

    const bool isExt = (canIdRaw & kExtFlag) != 0;
    const quint32 id = canIdRaw & 0x1FFFFFFFU;
    const QString typeText = isFd
            ? (brs ? QStringLiteral("CANFD(BRS)") : QStringLiteral("CANFD"))
            : QStringLiteral("CAN");

    return QStringLiteral("ID=0x%1%2 %3 DLC=%4 Data=[%5]")
            .arg(id, isExt ? 8 : 3, 16, QChar('0'))
            .arg(isExt ? QStringLiteral("(扩展)") : QString())
            .arg(typeText)
            .arg(data.size())
            .arg(QString(data.toHex(' ').toUpper()))
            .toUpper();
}

} // namespace CANFrameUtils
