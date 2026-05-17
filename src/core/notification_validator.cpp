#include "core/notification_validator.h"

#include <QRegularExpression>

namespace smartalarm {

namespace {

void validateTime(const std::optional<QTime> &time, const QString &fieldPath, ValidationResult &result)
{
    if (!time || !time->isValid()) {
        result.add(ValidationErrorCode::Required, fieldPath);
    }
}

void validateDate(const std::optional<QDate> &date, const QString &fieldPath, ValidationResult &result)
{
    if (!date || !date->isValid()) {
        result.add(ValidationErrorCode::Required, fieldPath);
    }
}

} // namespace

ValidationResult NotificationValidator::validate(const Notification &notification)
{
    ValidationResult result;
    if (notification.message.trimmed().isEmpty()) {
        result.add(ValidationErrorCode::Required, QStringLiteral("message"));
    }
    if (notification.playCount < 0 || notification.playCount > 999) {
        result.add(ValidationErrorCode::OutOfRange, QStringLiteral("playCount"));
    }
    if (notification.volume < 0 || notification.volume > 100) {
        result.add(ValidationErrorCode::OutOfRange, QStringLiteral("volume"));
    }
    if (!isCanonicalHexColor(notification.color)) {
        result.add(ValidationErrorCode::Invalid, QStringLiteral("color"));
    }
    if (const auto *custom = std::get_if<CustomSound>(&notification.sound)) {
        if (custom->pattern.trimmed().isEmpty()) {
            result.add(ValidationErrorCode::Required, QStringLiteral("sound.custom.pattern"));
        }
    }

    std::visit([&](const auto &schedule) {
        using T = std::decay_t<decltype(schedule)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            validateDate(schedule.date, QStringLiteral("schedule.once.date"), result);
            validateTime(schedule.time, QStringLiteral("schedule.once.time"), result);
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            if (schedule.days.isEmpty()) {
                result.add(ValidationErrorCode::Required, QStringLiteral("schedule.weekly.days"));
            }
            validateTime(schedule.time, QStringLiteral("schedule.weekly.time"), result);
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            if (schedule.everyWeeks < 1 || schedule.everyWeeks > 999) {
                result.add(ValidationErrorCode::OutOfRange, QStringLiteral("schedule.nthWeek.everyWeeks"));
            }
            validateTime(schedule.time, QStringLiteral("schedule.nthWeek.time"), result);
            validateDate(schedule.referenceDate, QStringLiteral("schedule.nthWeek.referenceDate"), result);
        } else if constexpr (std::is_same_v<T, IntervalSchedule>) {
            if (schedule.everyMinutes < 1 || schedule.everyMinutes > 1440) {
                result.add(ValidationErrorCode::OutOfRange, QStringLiteral("schedule.interval.everyMinutes"));
            }
            validateTime(schedule.from, QStringLiteral("schedule.interval.from"), result);
            validateTime(schedule.to, QStringLiteral("schedule.interval.to"), result);
            if (schedule.snoozeMinutes < 0 || schedule.snoozeMinutes > 1440) {
                result.add(ValidationErrorCode::OutOfRange, QStringLiteral("schedule.interval.snoozeMinutes"));
            }
            if (schedule.schedule && schedule.schedule->days.isEmpty()) {
                const bool hasDateRange = schedule.schedule->dateRange.start || schedule.schedule->dateRange.end;
                if (hasDateRange) {
                    result.add(ValidationErrorCode::Required, QStringLiteral("schedule.interval.schedule.days"));
                }
            }
        }
    }, notification.schedule);

    return result;
}

} // namespace smartalarm
