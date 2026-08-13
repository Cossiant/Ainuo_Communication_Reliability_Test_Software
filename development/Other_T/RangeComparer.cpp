// RangeComparer.cpp
#include "RangeComparer.h"
#include "An30Layout.h"
#include <cmath>
#include <QStringList>

namespace {

// 解析 [±]mantissa[.fraction][E[±]exponent]，支持 +1.000000E+00、3.0E+01 等格式。
// 成功返回 true，并通过 value 输出数值；尾随非法字符则解析失败。
bool parseNumericText(const QByteArray &raw, double &value)
{
    QByteArray s = raw.trimmed();
    s.replace('\r', "").replace('\n', "").replace(' ', "");
    if (s.isEmpty()) {
        return false;
    }

    int i = 0;
    bool negative = false;
    if (s.at(0) == '+' || s.at(0) == '-') {
        negative = (s.at(0) == '-');
        i = 1;
    }

    bool sawDot = false;
    bool anyDigit = false;
    double mantissa = 0.0;
    double fracScale = 0.1;

    while (i < s.size()) {
        const char c = s.at(i);
        if (c >= '0' && c <= '9') {
            anyDigit = true;
            if (!sawDot) {
                mantissa = mantissa * 10.0 + (c - '0');
            } else {
                mantissa += (c - '0') * fracScale;
                fracScale *= 0.1;
            }
            ++i;
        } else if (c == '.' && !sawDot) {
            sawDot = true;
            ++i;
        } else {
            break;
        }
    }

    if (!anyDigit) {
        return false;
    }

    long long exponent = 0;
    if (i < s.size() && (s.at(i) == 'e' || s.at(i) == 'E')) {
        ++i;
        bool expNegative = false;
        if (i < s.size() && (s.at(i) == '+' || s.at(i) == '-')) {
            expNegative = (s.at(i) == '-');
            ++i;
        }

        const int expStart = i;
        while (i < s.size() && s.at(i) >= '0' && s.at(i) <= '9') {
            exponent = exponent * 10 + (s.at(i) - '0');
            if (exponent > 308) {
                return false;   // 超出 double 可表示范围
            }
            ++i;
        }
        if (i == expStart) {
            return false;       // E 后没有指数数字
        }
        if (expNegative) {
            exponent = -exponent;
        }
    }

    if (i != s.size()) {
        return false;           // 存在无法识别的尾随字符
    }

    if (negative) {
        mantissa = -mantissa;
    }
    if (exponent != 0) {
        mantissa *= std::pow(10.0, static_cast<double>(exponent));
    }

    value = mantissa;
    return true;
}

} // namespace

bool RangeComparer::compareAscii(const QByteArray &received,
                                  const QByteArray &expected,
                                  double tolerance,
                                  bool parseScientific)
{
    QByteArray recvClean = received;
    QByteArray expectClean = expected;
    recvClean.replace("\r", "").replace("\n", "").replace(" ", "");
    expectClean.replace("\r", "").replace("\n", "").replace(" ", "");

    if (recvClean.isEmpty() || expectClean.isEmpty()) return false;

    bool ok1 = false, ok2 = false;
    double recvVal   = 0.0;
    double expectVal = 0.0;

    if (parseScientific) {
        ok1 = parseNumericText(recvClean, recvVal);
        ok2 = parseNumericText(expectClean, expectVal);
    } else {
        recvVal   = recvClean.toDouble(&ok1);
        expectVal = expectClean.toDouble(&ok2);
    }

    if (!ok1 || !ok2) return false;
    return qAbs(recvVal - expectVal) <= tolerance;
}

bool RangeComparer::isNumericText(const QByteArray &text, bool allowScientific)
{
    QByteArray clean = text;
    clean.replace("\r", "").replace("\n", "").replace(" ", "");
    if (clean.isEmpty()) {
        return false;
    }

    if (allowScientific) {
        double value = 0.0;
        return parseNumericText(clean, value);
    }

    bool ok = false;
    clean.toDouble(&ok);
    return ok;
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
