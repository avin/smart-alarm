#pragma once

#include "core/notification.h"

#include <QPushButton>
#include <QWidget>

namespace smartalarm {

class DayOfWeekSelector : public QWidget {
    Q_OBJECT
public:
    explicit DayOfWeekSelector(QWidget *parent = nullptr);

    QVector<Weekday> selectedDays() const;
    void setSelectedDays(const QVector<Weekday> &days);
    void setInvalid(bool invalid);

signals:
    void changed();

private:
    QHash<Weekday, QPushButton *> m_buttons;
};

} // namespace smartalarm
