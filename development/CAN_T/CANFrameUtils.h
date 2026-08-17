#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace CANFrameUtils {

// 解析 "<CAN_ID十六进制> [FD] <数据HEX>" 文本。
// canIdRaw 为带 CAN_EFF_FLAG 标志的 32 位 ID；
// 数据超过 8 字节时自动按 CAN FD 处理，也可用 FD 关键字强制 FD。
bool parseFrameText(const QString &text,
                    quint32 &canIdRaw,
                    QByteArray &data,
                    bool &isFd,
                    bool &brs,
                    QString *errorMessage = nullptr);

// 统一帧格式：1 字节标志 + 4 字节大端 canIdRaw + 数据
QByteArray encodeFrame(quint32 canIdRaw,
                       const QByteArray &data,
                       bool isFd,
                       bool brs);

bool decodeFrame(const QByteArray &frame,
                 quint32 &canIdRaw,
                 QByteArray &data,
                 bool &isFd,
                 bool &brs);

QString formatFrame(const QByteArray &frame);

} // namespace CANFrameUtils
