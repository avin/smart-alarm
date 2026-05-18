#include "core/notification.h"

#include <QColor>

namespace smartalarm {

QString weekdayToString(Weekday weekday)
{
    switch (weekday) {
    case Weekday::Mon: return QStringLiteral("mon");
    case Weekday::Tue: return QStringLiteral("tue");
    case Weekday::Wed: return QStringLiteral("wed");
    case Weekday::Thu: return QStringLiteral("thu");
    case Weekday::Fri: return QStringLiteral("fri");
    case Weekday::Sat: return QStringLiteral("sat");
    case Weekday::Sun: return QStringLiteral("sun");
    }
    return QStringLiteral("mon");
}

QString weekdayDisplayName(Weekday weekday)
{
    switch (weekday) {
    case Weekday::Mon: return QStringLiteral("Mon");
    case Weekday::Tue: return QStringLiteral("Tue");
    case Weekday::Wed: return QStringLiteral("Wed");
    case Weekday::Thu: return QStringLiteral("Thu");
    case Weekday::Fri: return QStringLiteral("Fri");
    case Weekday::Sat: return QStringLiteral("Sat");
    case Weekday::Sun: return QStringLiteral("Sun");
    }
    return QStringLiteral("Mon");
}

std::optional<Weekday> weekdayFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("mon")) return Weekday::Mon;
    if (normalized == QStringLiteral("tue")) return Weekday::Tue;
    if (normalized == QStringLiteral("wed")) return Weekday::Wed;
    if (normalized == QStringLiteral("thu")) return Weekday::Thu;
    if (normalized == QStringLiteral("fri")) return Weekday::Fri;
    if (normalized == QStringLiteral("sat")) return Weekday::Sat;
    if (normalized == QStringLiteral("sun")) return Weekday::Sun;
    return std::nullopt;
}

QVector<Weekday> allWeekdays()
{
    return { Weekday::Mon, Weekday::Tue, Weekday::Wed, Weekday::Thu, Weekday::Fri, Weekday::Sat, Weekday::Sun };
}

QString notificationPositionToString(NotificationPosition position)
{
    switch (position) {
    case NotificationPosition::TopLeft: return QStringLiteral("top_left");
    case NotificationPosition::TopRight: return QStringLiteral("top_right");
    case NotificationPosition::BottomLeft: return QStringLiteral("bottom_left");
    case NotificationPosition::BottomRight: return QStringLiteral("bottom_right");
    case NotificationPosition::Center: return QStringLiteral("center");
    }
    return QStringLiteral("top_right");
}

std::optional<NotificationPosition> notificationPositionFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("top_left")) return NotificationPosition::TopLeft;
    if (normalized == QStringLiteral("top_right")) return NotificationPosition::TopRight;
    if (normalized == QStringLiteral("bottom_left")) return NotificationPosition::BottomLeft;
    if (normalized == QStringLiteral("bottom_right")) return NotificationPosition::BottomRight;
    if (normalized == QStringLiteral("center")) return NotificationPosition::Center;
    return std::nullopt;
}

QString countFromToString(CountFrom countFrom)
{
    return countFrom == CountFrom::Confirmation ? QStringLiteral("confirmation") : QStringLiteral("trigger");
}

std::optional<CountFrom> countFromFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("trigger")) return CountFrom::Trigger;
    if (normalized == QStringLiteral("confirmation")) return CountFrom::Confirmation;
    return std::nullopt;
}

QString soundPresetToString(SoundPreset preset)
{
    switch (preset) {
    case SoundPreset::ClassicBeep: return QStringLiteral("classic_beep");
    case SoundPreset::DoubleBeep: return QStringLiteral("double_beep");
    case SoundPreset::DigitalAlert: return QStringLiteral("digital_alert");
    case SoundPreset::GentleChime: return QStringLiteral("gentle_chime");
    case SoundPreset::Urgent: return QStringLiteral("urgent");
    case SoundPreset::SoftPulse: return QStringLiteral("soft_pulse");
    case SoundPreset::HighLowAlert: return QStringLiteral("high_low_alert");
    case SoundPreset::TriplePulse: return QStringLiteral("triple_pulse");
    }
    return QStringLiteral("gentle_chime");
}

QString soundPresetDisplayName(SoundPreset preset)
{
    switch (preset) {
    case SoundPreset::ClassicBeep: return QStringLiteral("Classic beep");
    case SoundPreset::DoubleBeep: return QStringLiteral("Double beep");
    case SoundPreset::DigitalAlert: return QStringLiteral("Digital alert");
    case SoundPreset::GentleChime: return QStringLiteral("Gentle chime");
    case SoundPreset::Urgent: return QStringLiteral("Urgent");
    case SoundPreset::SoftPulse: return QStringLiteral("Soft pulse");
    case SoundPreset::HighLowAlert: return QStringLiteral("High-low alert");
    case SoundPreset::TriplePulse: return QStringLiteral("Triple pulse");
    }
    return QStringLiteral("Gentle chime");
}

std::optional<SoundPreset> soundPresetFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("classic_beep")) return SoundPreset::ClassicBeep;
    if (normalized == QStringLiteral("double_beep")) return SoundPreset::DoubleBeep;
    if (normalized == QStringLiteral("digital_alert")) return SoundPreset::DigitalAlert;
    if (normalized == QStringLiteral("gentle_chime")) return SoundPreset::GentleChime;
    if (normalized == QStringLiteral("urgent")) return SoundPreset::Urgent;
    if (normalized == QStringLiteral("soft_pulse")) return SoundPreset::SoftPulse;
    if (normalized == QStringLiteral("high_low_alert")) return SoundPreset::HighLowAlert;
    if (normalized == QStringLiteral("triple_pulse")) return SoundPreset::TriplePulse;
    return std::nullopt;
}

QString waveformToString(Waveform waveform)
{
    switch (waveform) {
    case Waveform::Sine: return QStringLiteral("sine");
    case Waveform::Square: return QStringLiteral("square");
    case Waveform::Triangle: return QStringLiteral("triangle");
    case Waveform::Sawtooth: return QStringLiteral("sawtooth");
    }
    return QStringLiteral("sine");
}

std::optional<Waveform> waveformFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("sine")) return Waveform::Sine;
    if (normalized == QStringLiteral("square")) return Waveform::Square;
    if (normalized == QStringLiteral("triangle")) return Waveform::Triangle;
    if (normalized == QStringLiteral("sawtooth")) return Waveform::Sawtooth;
    return std::nullopt;
}

QDate mondayOfWeek(const QDate &date)
{
    return date.addDays(1 - date.dayOfWeek());
}

QDateTime normalizeToMinute(const QDateTime &dateTime)
{
    const QTime time(dateTime.time().hour(), dateTime.time().minute());
    return QDateTime(dateTime.date(), time);
}

QDateTime ceilToNextMinute(const QDateTime &dateTime)
{
    auto normalized = normalizeToMinute(dateTime);
    if (dateTime.time().second() > 0 || dateTime.time().msec() > 0) {
        normalized = normalized.addSecs(60);
    }
    return normalized;
}

bool isCanonicalHexColor(const QString &value)
{
    if (value.size() != 7 || !value.startsWith(QLatin1Char('#'))) {
        return false;
    }
    for (int i = 1; i < value.size(); ++i) {
        const auto ch = value.at(i);
        if (!ch.isDigit() && (ch.toLower() < QLatin1Char('a') || ch.toLower() > QLatin1Char('f'))) {
            return false;
        }
    }
    return QColor(value).isValid();
}

QString canonicalHexColor(const QString &value)
{
    const QColor color(value);
    if (!color.isValid()) {
        return QStringLiteral("#D94841");
    }
    return color.name(QColor::HexRgb).toUpper();
}

} // namespace smartalarm
