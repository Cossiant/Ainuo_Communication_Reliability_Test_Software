// StickySplitter.h
// 公共工具：粘包分割
// 串口 / 网口共享

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace StickySplitter {

    // 将 comboBox 中的文本转换为真实分隔符字节
    // "\\n" → "\n"
    // "\\r" → "\r"
    // "\\r\\n" → "\r\n"
    QByteArray delimiterFromComboText(const QString &text);

    // 按分隔符拆分 data，返回分割后的列表
    // 剩余的不完整片段追加到列表末尾
    QList<QByteArray> split(const QByteArray &data,
                             const QByteArray &delimiter);

} // namespace StickySplitter
