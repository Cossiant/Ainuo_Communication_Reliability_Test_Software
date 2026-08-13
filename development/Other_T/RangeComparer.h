// RangeComparer.h
#pragma once
#include <QByteArray>
#include <QString>

namespace RangeComparer {

    bool compareAscii(const QByteArray &received,
                      const QByteArray &expected,
                      double tolerance,
                      bool parseScientific = false);

    /// 判断文本是否是纯数值（可选的科学计数法）。
    /// 用于在“字符串精确比对”与“数值区间判断”之间自动分流。
    bool isNumericText(const QByteArray &text,
                       bool allowScientific = false);

    /// AN3.0 HEX 帧区间比较（自动识别命令码）
    bool compareHexFrame(const QByteArray &reference,   // 列B参考帧
                          const QByteArray &received,    // 设备实时回复
                          double tolerance,
                          QString &detail);             // 输出明细

}
