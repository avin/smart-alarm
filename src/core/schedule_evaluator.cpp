#include "core/schedule_evaluator.h"

#include <algorithm>

namespace smartalarm {

namespace {

bool sameMinute(const QDateTime &left, const QDateTime &right)
{
    return normalizeToMinute(left) == normalizeToMinute(right);
}

bool containsWeekday(const QVector<Weekday> &days, int qtDayOfWeek)
{
    const auto weekday = static_cast<Weekday>(qtDayOfWeek);
    return std::find(days.cbegin(), days.cend(), weekday) != days.cend();
}

bool dateInRange(const QDate &date, const std::optional<DateRange> &range)
{
    if (!range) {
        return true;
    }
    if (range->start && date < *range->start) {
        return false;
    }
    if (range->end && date > *range->end) {
        return false;
    }
    return true;
}

bool intervalLimitAllows(const IntervalSchedule &schedule, const QDate &date)
{
    if (!schedule.schedule) {
        return true;
    }
    if (!containsWeekday(schedule.schedule->days, date.dayOfWeek())) {
        return false;
    }
    const auto range = std::optional<DateRange>(schedule.schedule->dateRange);
    return dateInRange(date, range);
}

bool timeWithinWindow(const QTime &time, const QTime &from, const QTime &to)
{
    if (from > to) {
        return false;
    }
    return time >= from && time <= to;
}

int minutesOfDay(const QTime &time)
{
    return time.hour() * 60 + time.minute();
}

bool intervalTriggerDue(const IntervalSchedule &schedule, const QDateTime &now)
{
    if (!schedule.from || !schedule.to) {
        return false;
    }
    const auto nowTime = QTime(now.time().hour(), now.time().minute());
    if (!timeWithinWindow(nowTime, *schedule.from, *schedule.to)) {
        return false;
    }
    const int delta = minutesOfDay(nowTime) - minutesOfDay(*schedule.from);
    return delta >= 0 && delta % schedule.everyMinutes == 0;
}

bool intervalConfirmationDue(const Notification &notification, const IntervalSchedule &schedule, const QDateTime &now, const RuntimeState &runtime)
{
    const auto lastDismissed = runtime.lastDismissedAt.value(notification.id);
    if (!lastDismissed.isValid()) {
        return intervalTriggerDue(schedule, now);
    }
    if (!schedule.from || !schedule.to) {
        return false;
    }
    const auto nowMinute = normalizeToMinute(now);
    const auto lastMinute = normalizeToMinute(lastDismissed);
    const bool sameDay = lastMinute.date() == nowMinute.date();
    const bool lastInsideWindow = timeWithinWindow(lastMinute.time(), *schedule.from, *schedule.to);
    if (!sameDay || !lastInsideWindow) {
        return intervalTriggerDue(schedule, now);
    }
    const auto next = normalizeToMinute(lastMinute.addSecs(schedule.everyMinutes * 60));
    if (!timeWithinWindow(next.time(), *schedule.from, *schedule.to)) {
        return false;
    }
    return next == nowMinute;
}

} // namespace

bool ScheduleEvaluator::isNormalDue(const Notification &notification, const QDateTime &now, const RuntimeState &runtime)
{
    const auto nowMinute = normalizeToMinute(now);
    if (runtime.lastTriggeredMinute.value(notification.id) == nowMinute) {
        return false;
    }

    return std::visit([&](const auto &schedule) -> bool {
        using T = std::decay_t<decltype(schedule)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            if (!schedule.date || !schedule.time) {
                return false;
            }
            return schedule.date == now.date() && *schedule.time == QTime(now.time().hour(), now.time().minute());
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            if (!schedule.time || !containsWeekday(schedule.days, now.date().dayOfWeek())) {
                return false;
            }
            return dateInRange(now.date(), schedule.dateRange) && *schedule.time == QTime(now.time().hour(), now.time().minute());
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            if (!schedule.time || !schedule.referenceDate || *schedule.time != QTime(now.time().hour(), now.time().minute())) {
                return false;
            }
            if (schedule.endDate && now.date() > *schedule.endDate) {
                return false;
            }
            if (now.date() < *schedule.referenceDate) {
                return false;
            }
            if (now.date().dayOfWeek() != static_cast<int>(schedule.weekday)) {
                return false;
            }
            const auto referenceWeek = mondayOfWeek(*schedule.referenceDate);
            const auto candidateWeek = mondayOfWeek(now.date());
            if (candidateWeek < referenceWeek) {
                return false;
            }
            const int weekDiff = referenceWeek.daysTo(candidateWeek) / 7;
            return weekDiff % schedule.everyWeeks == 0;
        } else if constexpr (std::is_same_v<T, IntervalSchedule>) {
            if (!intervalLimitAllows(schedule, now.date())) {
                return false;
            }
            if (schedule.countFrom == CountFrom::Confirmation) {
                return intervalConfirmationDue(notification, schedule, now, runtime);
            }
            return intervalTriggerDue(schedule, now);
        }
    }, notification.schedule);
}

TriggerDecision ScheduleEvaluator::evaluate(const Notification &notification, const QDateTime &now, const RuntimeState &runtime)
{
    if (!runtime.notificationsEnabled || !notification.enabled) {
        return {};
    }
    if (isNormalDue(notification, now, runtime)) {
        return { TriggerKind::Normal, true };
    }
    const auto snoozeAt = runtime.pendingSnooze.value(notification.id);
    if (snoozeAt.isValid() && normalizeToMinute(now) >= normalizeToMinute(snoozeAt)) {
        return { TriggerKind::Snooze, true };
    }
    return {};
}

bool ScheduleEvaluator::isOnceOverdue(const Notification &notification, const QDateTime &now)
{
    const auto *once = std::get_if<OnceSchedule>(&notification.schedule);
    if (!once || !once->date || !once->time) {
        return false;
    }
    return QDateTime(*once->date, *once->time) < normalizeToMinute(now);
}

} // namespace smartalarm
