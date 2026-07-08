#ifndef STATCARD_H
#define STATCARD_H

#include <QWidget>
#include <QString>

class StatCard : public QWidget
{
    Q_OBJECT
public:
    explicit StatCard(const QString& title, const QString& value,
                      QWidget* parent = nullptr);

    void setValue(const QString& value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_value;
};

#endif
