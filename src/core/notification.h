#pragma once

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QTime>
#include <QUuid>
#include <QVector>

#include <optional>
#include <variant>

namespace smartalarm {

enum class Weekday {
    Mon = 1,
    Tue,
    Wed,
    Thu,
    Fri,
    Sat,
    Sun
};

enum class NotificationPosition {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center
};

enum class CountFrom {
    Trigger,
    Confirmation
};

enum class SoundPreset {
    ClassicBeep,
    DoubleBeep,
    DigitalAlert,
    GentleChime,
    Urgent,
    SoftPulse,
    HighLowAlert,
    TriplePulse
};

enum class Waveform {
    Sine,
    Square,
    Triangle,
    Sawtooth
};

struct DateRange {
    std::optional<QDate> start;
    std::optional<QDate> end;
};

struct OnceSchedule {
    std::optional<QDate> date;
    std::optional<QTime> time;
};

struct WeeklySchedule {
    QVector<Weekday> days;
    std::optional<QTime> time;
    std::optional<DateRange> dateRange;
};

struct NthWeekSchedule {
    int everyWeeks = 1;
    Weekday weekday = Weekday::Mon;
    std::optional<QTime> time;
    std::optional<QDate> referenceDate;
    std::optional<QDate> endDate;
};

struct IntervalScheduleLimit {
    QVector<Weekday> days;
    DateRange dateRange;
};

struct IntervalSchedule {
    int everyMinutes = 40;
    std::optional<QTime> from = QTime(0, 0);
    std::optional<QTime> to = QTime(23, 59);
    CountFrom countFrom = CountFrom::Trigger;
    int snoozeMinutes = 0;
    std::optional<IntervalScheduleLimit> schedule;
};

using Schedule = std::variant<OnceSchedule, WeeklySchedule, NthWeekSchedule, IntervalSchedule>;

struct PresetSound {
    SoundPreset preset = SoundPreset::GentleChime;
};

struct CustomSound {
    QString pattern;
};

using SoundSpec = std::variant<PresetSound, CustomSound>;

struct Notification {
    QUuid id = QUuid::createUuid();
    bool enabled = true;
    QString message;
    QString color = QStringLiteral("#D94841");
    int volume = 70;
    int playCount = 0;
    SoundSpec sound = PresetSound {};
    Schedule schedule = OnceSchedule { QDate::currentDate(), std::nullopt };
};

struct GlobalSettings {
    int defaultSnoozeMinutes = 1;
    NotificationPosition notificationPosition = NotificationPosition::TopRight;
};

struct AppConfig {
    GlobalSettings settings;
    QVector<Notification> notifications;
};

struct RuntimeState {
    bool notificationsEnabled = true;
    QHash<QUuid, QDateTime> lastTriggeredMinute;
    QHash<QUuid, QDateTime> pendingSnooze;
    QHash<QUuid, QDateTime> lastDismissedAt;
    QHash<QUuid, QDateTime> intervalResetAt;
};

QString weekdayToString(Weekday weekday);
QString weekdayDisplayName(Weekday weekday);
std::optional<Weekday> weekdayFromString(const QString &value);
QVector<Weekday> allWeekdays();

QString notificationPositionToString(NotificationPosition position);
std::optional<NotificationPosition> notificationPositionFromString(const QString &value);

QString countFromToString(CountFrom countFrom);
std::optional<CountFrom> countFromFromString(const QString &value);

QString soundPresetToString(SoundPreset preset);
QString soundPresetDisplayName(SoundPreset preset);
std::optional<SoundPreset> soundPresetFromString(const QString &value);

QString waveformToString(Waveform waveform);
std::optional<Waveform> waveformFromString(const QString &value);

QDate mondayOfWeek(const QDate &date);
QDateTime normalizeToMinute(const QDateTime &dateTime);
QDateTime ceilToNextMinute(const QDateTime &dateTime);
bool isCanonicalHexColor(const QString &value);
QString canonicalHexColor(const QString &value);

} // namespace smartalarm
