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

QDateTime firstGridTimeAtOrAfter(const QDateTime &gridStart, int everyMinutes, const QDateTime &from)
{
    if (from <= gridStart) {
        return gridStart;
    }
    const qint64 seconds = gridStart.secsTo(from);
    const qint64 intervalSeconds = qint64(everyMinutes) * 60;
    const qint64 steps = (seconds + intervalSeconds - 1) / intervalSeconds;
    return gridStart.addSecs(steps * intervalSeconds);
}

std::optional<QDateTime> intervalResetBase(const Notification &notification, const IntervalSchedule &schedule, const RuntimeState &runtime, const QDate &date)
{
    const auto resetAt = runtime.intervalResetAt.value(notification.id);
    if (!resetAt.isValid() || resetAt.date() != date || !schedule.from || !schedule.to) {
        return std::nullopt;
    }
    const auto resetMinute = normalizeToMinute(resetAt);
    if (!timeWithinWindow(resetMinute.time(), *schedule.from, *schedule.to)) {
        return std::nullopt;
    }
    return ceilToNextMinute(resetAt.addSecs(schedule.everyMinutes * 60));
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

bool intervalResetDue(const Notification &notification, const IntervalSchedule &schedule, const QDateTime &now, const RuntimeState &runtime)
{
    const auto base = intervalResetBase(notification, schedule, runtime, now.date());
    if (!base) {
        return false;
    }
    const auto nowMinute = normalizeToMinute(now);
    if (nowMinute < *base || !timeWithinWindow(nowMinute.time(), *schedule.from, *schedule.to)) {
        return false;
    }
    return base->secsTo(nowMinute) % (qint64(schedule.everyMinutes) * 60) == 0;
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

std::optional<QDateTime> nextOnceOccurrence(const OnceSchedule &schedule, const QDateTime &from)
{
    if (!schedule.date || !schedule.time) {
        return std::nullopt;
    }
    const QDateTime candidate(*schedule.date, *schedule.time);
    if (candidate >= from) {
        return candidate;
    }
    return std::nullopt;
}

std::optional<QDateTime> nextWeeklyOccurrence(const WeeklySchedule &schedule, const QDateTime &from)
{
    if (!schedule.time || schedule.days.isEmpty()) {
        return std::nullopt;
    }
    for (int offset = 0; offset <= 366 * 5; ++offset) {
        const auto date = from.date().addDays(offset);
        if (!containsWeekday(schedule.days, date.dayOfWeek()) || !dateInRange(date, schedule.dateRange)) {
            continue;
        }
        const QDateTime candidate(date, *schedule.time);
        if (candidate >= from) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::optional<QDateTime> nextNthWeekOccurrence(const NthWeekSchedule &schedule, const QDateTime &from)
{
    if (!schedule.time || !schedule.referenceDate) {
        return std::nullopt;
    }
    const auto startDate = std::max(from.date(), *schedule.referenceDate);
    for (int offset = 0; offset <= 366 * 25; ++offset) {
        const auto date = startDate.addDays(offset);
        if (schedule.endDate && date > *schedule.endDate) {
            return std::nullopt;
        }
        if (date.dayOfWeek() != static_cast<int>(schedule.weekday)) {
            continue;
        }
        const auto referenceWeek = mondayOfWeek(*schedule.referenceDate);
        const auto candidateWeek = mondayOfWeek(date);
        if (candidateWeek < referenceWeek) {
            continue;
        }
        const int weekDiff = referenceWeek.daysTo(candidateWeek) / 7;
        if (weekDiff % schedule.everyWeeks != 0) {
            continue;
        }
        const QDateTime candidate(date, *schedule.time);
        if (candidate >= from) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::optional<QDateTime> nextIntervalOccurrence(const Notification &notification, const IntervalSchedule &schedule, const QDateTime &from, const RuntimeState &runtime)
{
    if (!schedule.from || !schedule.to || *schedule.from > *schedule.to) {
        return std::nullopt;
    }
    for (int offset = 0; offset <= 366 * 5; ++offset) {
        const auto date = from.date().addDays(offset);
        if (!intervalLimitAllows(schedule, date)) {
            continue;
        }
        const QDateTime lowerBound = offset == 0 ? from : QDateTime(date, *schedule.from);
        std::optional<QDateTime> gridStart = intervalResetBase(notification, schedule, runtime, date);
        if (!gridStart && schedule.countFrom == CountFrom::Confirmation) {
            const auto lastDismissed = runtime.lastDismissedAt.value(notification.id);
            if (lastDismissed.isValid()) {
                const auto lastMinute = normalizeToMinute(lastDismissed);
                if (lastMinute.date() == date
                    && timeWithinWindow(lastMinute.time(), *schedule.from, *schedule.to)) {
                    gridStart = normalizeToMinute(lastMinute.addSecs(schedule.everyMinutes * 60));
                }
            }
        }
        if (!gridStart) {
            gridStart = QDateTime(date, *schedule.from);
        }
        const auto candidate = firstGridTimeAtOrAfter(*gridStart, schedule.everyMinutes, lowerBound);
        if (candidate.date() == date && timeWithinWindow(candidate.time(), *schedule.from, *schedule.to)) {
            return candidate;
        }
    }
    return std::nullopt;
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
            if (intervalResetBase(notification, schedule, runtime, now.date())) {
                return intervalResetDue(notification, schedule, now, runtime);
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

std::optional<QDateTime> ScheduleEvaluator::nextOccurrence(const Notification &notification, const QDateTime &from, const RuntimeState &runtime)
{
    if (!runtime.notificationsEnabled || !notification.enabled) {
        return std::nullopt;
    }
    auto searchFrom = normalizeToMinute(from);
    if (runtime.lastTriggeredMinute.value(notification.id) == searchFrom) {
        searchFrom = searchFrom.addSecs(60);
    }
    auto nextNormal = std::visit([&](const auto &schedule) -> std::optional<QDateTime> {
        using T = std::decay_t<decltype(schedule)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            return nextOnceOccurrence(schedule, searchFrom);
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            return nextWeeklyOccurrence(schedule, searchFrom);
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            return nextNthWeekOccurrence(schedule, searchFrom);
        } else {
            return nextIntervalOccurrence(notification, schedule, searchFrom, runtime);
        }
    }, notification.schedule);
    const auto snoozeAt = runtime.pendingSnooze.value(notification.id);
    if (snoozeAt.isValid()) {
        const auto nextSnooze = normalizeToMinute(snoozeAt);
        if (nextSnooze >= searchFrom && (!nextNormal || nextSnooze < *nextNormal)) {
            return nextSnooze;
        }
    }
    return nextNormal;
}

} // namespace smartalarm
