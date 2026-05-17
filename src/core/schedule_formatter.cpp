#include "core/schedule_formatter.h"

namespace smartalarm {

namespace {

QString dateText(const std::optional<QDate> &date)
{
    return date ? date->toString(QStringLiteral("yyyy-MM-dd")) : QStringLiteral("-");
}

QString timeText(const std::optional<QTime> &time)
{
    return time ? time->toString(QStringLiteral("HH:mm")) : QStringLiteral("--:--");
}

QString daysText(const QVector<Weekday> &days)
{
    QStringList parts;
    for (const auto day : days) {
        parts << weekdayDisplayName(day);
    }
    return parts.join(QStringLiteral(", "));
}

QString rangeText(const std::optional<DateRange> &range)
{
    if (!range || (!range->start && !range->end)) {
        return {};
    }
    return QStringLiteral(", %1..%2").arg(dateText(range->start), dateText(range->end));
}

} // namespace

QString ScheduleFormatter::format(const Schedule &schedule)
{
    return std::visit([](const auto &value) -> QString {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            return QStringLiteral("Once %1 %2").arg(dateText(value.date), timeText(value.time));
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            return QStringLiteral("Weekly %1 %2%3").arg(daysText(value.days), timeText(value.time), rangeText(value.dateRange));
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            return QStringLiteral("Every %1 weeks on %2 %3").arg(value.everyWeeks).arg(weekdayDisplayName(value.weekday), timeText(value.time));
        } else {
            QString suffix;
            if (value.schedule) {
                suffix = QStringLiteral(", %1").arg(daysText(value.schedule->days));
            }
            return QStringLiteral("Every %1 min, %2..%3%4").arg(value.everyMinutes).arg(timeText(value.from), timeText(value.to), suffix);
        }
    }, schedule);
}

} // namespace smartalarm
