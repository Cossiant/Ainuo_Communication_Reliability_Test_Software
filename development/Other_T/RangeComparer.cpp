// RangeComparer.cpp

#include "RangeComparer.h"
#include <cmath>

bool RangeComparer::compareAscii(const QByteArray &received,
                                  const QByteArray &expected,
                                  double tolerance)
{
    QByteArray recvClean = received;
    QByteArray expectClean = expected;
    recvClean.replace("\r", "").replace("\n", "").replace(" ", "");
    expectClean.replace("\r", "").replace("\n", "").replace(" ", "");

    if (recvClean.isEmpty() || expectClean.isEmpty())
        return false;

    bool ok1 = false, ok2 = false;
    double recvVal   = recvClean.toDouble(&ok1);
    double expectVal = expectClean.toDouble(&ok2);

    if (!ok1 || !ok2)
        return false;   // 无法解析为数字 → 判定为不匹配

    return qAbs(recvVal - expectVal) <= tolerance;
}
