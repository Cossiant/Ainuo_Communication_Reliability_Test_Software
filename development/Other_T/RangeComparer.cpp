// RangeComparer.cpp
#include "RangeComparer.h"
#include "An30Layout.h"
#include <cmath>
#include <QStringList>

bool RangeComparer::compareAscii(const QByteArray &received,
                                  const QByteArray &expected,
                                  double tolerance)
{
    QByteArray recvClean = received;
    QByteArray expectClean = expected;
    recvClean.replace("\r", "").replace("\n", "").replace(" ", "");
    expectClean.replace("\r", "").replace("\n", "").replace(" ", "");

    if (recvClean.isEmpty() || expectClean.isEmpty()) return false;

    bool ok1 = false, ok2 = false;
    double recvVal   = recvClean.toDouble(&ok1);
    double expectVal = expectClean.toDouble(&ok2);

    if (!ok1 || !ok2) return false;
    return qAbs(recvVal - expectVal) <= tolerance;
}


// ═══════════════════════════════════════════════════════════════
//  AN3.0 HEX 帧区间比较
// ═══════════════════════════════════════════════════════════════
bool RangeComparer::compareHexFrame(const QByteArray &reference,
                                     const QByteArray &received,
                                     double tolerance,
                                     QString &detail)
{
    detail.clear();

    auto validate = [](const QByteArray& f) -> bool {
        if (f.size() < 7) return false;
        const uint8_t* d = reinterpret_cast<const uint8_t*>(f.constData());
        return (d[0] == 0x7B && d[f.size()-1] == 0x7D);
    };

    if (!validate(reference)) { detail = "参考帧结构错误"; return false; }
    if (!validate(received))  { detail = "实时帧结构错误"; return false; }

    const uint8_t* ref  = reinterpret_cast<const uint8_t*>(reference.constData());
    const uint8_t* recv = reinterpret_cast<const uint8_t*>(received.constData());

    uint8_t refCmdType  = ref[4],  refCmdWord  = ref[5];
    uint8_t recvCmdType = recv[4], recvCmdWord = recv[5];

    if (refCmdType != recvCmdType || refCmdWord != recvCmdWord) {
        detail = QString("命令码不一致: 参考 0x%1 0x%2 vs 实时 0x%3 0x%4")
                     .arg(refCmdType,2,16,QChar('0')).arg(refCmdWord,2,16,QChar('0'))
                     .arg(recvCmdType,2,16,QChar('0')).arg(recvCmdWord,2,16,QChar('0'));
        return false;
    }

    const CmdLayout* layout = An30Layout::instance().find(recvCmdType, recvCmdWord);
    if (!layout) {
        detail = QString("未注册命令: 0x%1 0x%2")
                     .arg(recvCmdType,2,16,QChar('0')).arg(recvCmdWord,2,16,QChar('0'));
        return false;
    }

    int refPayloadLen  = reference.size() - 8;
    int recvPayloadLen = received.size()  - 8;

    QVector<double> refVals  = An30Layout::instance().extractAll(
        *layout, ref  + 6, refPayloadLen);
    QVector<double> recvVals = An30Layout::instance().extractAll(
        *layout, recv + 6, recvPayloadLen);

    QStringList details;
    bool allPass = true;
    int n = qMin(refVals.size(), recvVals.size());

    for (int i = 0; i < n; ++i) {
        double diff = qAbs(recvVals[i] - refVals[i]);
        bool pass = (diff <= tolerance);

        QString fn = (i < layout->fields.size())
                     ? layout->fields[i].name : QString("字段%1").arg(i);

        details.append(QString("%1: 参考=%2 实际=%3 偏差=%4 %5")
                           .arg(fn)
                           .arg(refVals[i], 0, 'f', 3)
                           .arg(recvVals[i], 0, 'f', 3)
                           .arg(diff, 0, 'f', 3)
                           .arg(pass ? "✅" : "❌"));

        if (!pass) allPass = false;
    }

    if (refVals.size() != recvVals.size()) {
        details.append(QString("⚠字段数不一致: 参考%1 实时%2")
                           .arg(refVals.size()).arg(recvVals.size()));
        allPass = false;
    }

    detail = details.join(" | ");
    return allPass;
}
