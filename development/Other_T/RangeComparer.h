// RangeComparer.h
// 公共工具：ASCII / HEX 区间比较
// 串口 / 网口 / GPIB 三模块共享，消除重复代码

#pragma once

#include <QByteArray>

namespace RangeComparer {

    // ASCII 区间比较
    // 将 received / expected 去除 \r \n 和空格后解析为 double，
    // 判断 |received - expected| <= tolerance
    // 返回值: true=在区间内, false=不在区间内或无法解析为数字
    bool compareAscii(const QByteArray &received,
                      const QByteArray &expected,
                      double tolerance);

    // 未来扩展
    // bool compareHex(const QByteArray &received,
    //                 const QByteArray &expected,
    //                 double tolerance);

} // namespace RangeComparer
