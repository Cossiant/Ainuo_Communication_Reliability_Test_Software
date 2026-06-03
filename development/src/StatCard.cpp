#include "../include/StatCard.h"
#include <QPainter>
#include <QPainterPath>

StatCard::StatCard(const QString& title, const QString& value, QWidget* parent)
    : QWidget(parent), m_title(title), m_value(value)
{
    setFixedSize(190, 110);
    setObjectName("StatCard");
    setStyleSheet("#StatCard{background-color:transparent}");
}

void StatCard::setValue(const QString& value)
{
    m_value = value;
    update();
}

void StatCard::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // 背景（圆角矩形，与 Ela 亚克力风格一致）
    QRectF bgRect = rect().adjusted(1, 1, -1, -1);
    painter.setPen(QPen(QColor(200, 200, 200, 60), 1));
    painter.setBrush(QColor(255, 255, 255, 255));
    painter.drawRoundedRect(bgRect, 8, 8);

    // 标题
    QFont font = painter.font();
    font.setWeight(QFont::DemiBold);
    font.setPixelSize(15);
    painter.setFont(font);
    painter.setPen(QColor(140, 140, 140));
    painter.drawText(QRectF(12, 14, width() - 24, 24),
                     Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // 数值
    font.setWeight(QFont::Bold);
    font.setPixelSize(36);
    painter.setFont(font);
    painter.setPen(QColor(50, 50, 50));
    painter.drawText(QRectF(12, 42, width() - 24, 50),
                     Qt::AlignLeft | Qt::AlignVCenter, m_value);
}
