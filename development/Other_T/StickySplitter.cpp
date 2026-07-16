// StickySplitter.cpp

#include "StickySplitter.h"

QByteArray StickySplitter::delimiterFromComboText(const QString &text)
{
    if (text == "\\r")      return QByteArray("\r");
    if (text == "\\r\\n")   return QByteArray("\r\n");
    return QByteArray("\n"); // 默认
}

QList<QByteArray> StickySplitter::split(const QByteArray &data,
                                         const QByteArray &delimiter)
{
    QList<QByteArray> parts;
    int start = 0;
    int dlen  = delimiter.size();

    while (start < data.size()) {
        int idx = data.indexOf(delimiter, start);
        if (idx == -1) {
            QByteArray remaining = data.mid(start);
            if (!remaining.isEmpty())
                parts.append(remaining);
            break;
        }
        QByteArray segment = data.mid(start, idx - start + dlen);
        if (!segment.isEmpty())
            parts.append(segment);
        start = idx + dlen;
    }
    return parts;
}
