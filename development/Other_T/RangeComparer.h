// RangeComparer.h
#pragma once
#include <QByteArray>
#include <QString>

namespace RangeComparer {

    bool compareAscii(const QByteArray &received,
                      const QByteArray &expected,
                      double tolerance);

    /// AN3.0 HEX 帧区间比较（自动识别命令码）
    bool compareHexFrame(const QByteArray &reference,   // 列B参考帧
                          const QByteArray &received,    // 设备实时回复
                          double tolerance,
                          QString &detail);             // 输出明细

}
