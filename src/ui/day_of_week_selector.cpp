#include "ui/day_of_week_selector.h"

#include <QHBoxLayout>

namespace smartalarm {

DayOfWeekSelector::DayOfWeekSelector(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    for (const auto day : allWeekdays()) {
        auto *button = new QPushButton(weekdayDisplayName(day), this);
        button->setCheckable(true);
        button->setMinimumWidth(42);
        layout->addWidget(button);
        m_buttons.insert(day, button);
        connect(button, &QPushButton::toggled, this, [this] {
            setInvalid(false);
            emit changed();
        });
    }
    layout->addStretch();
}

QVector<Weekday> DayOfWeekSelector::selectedDays() const
{
    QVector<Weekday> days;
    for (const auto day : allWeekdays()) {
        if (m_buttons.value(day)->isChecked()) {
            days.push_back(day);
        }
    }
    return days;
}

void DayOfWeekSelector::setSelectedDays(const QVector<Weekday> &days)
{
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        it.value()->setChecked(days.contains(it.key()));
    }
}

void DayOfWeekSelector::setInvalid(bool invalid)
{
    Q_UNUSED(invalid);
    for (auto *button : m_buttons) {
        button->setStyleSheet(button->isChecked()
            ? QStringLiteral("QPushButton:checked { background: #0078d7; color: white; }")
            : QString());
    }
}

} // namespace smartalarm
