#include "core/time_input_normalizer.h"

#include <QString>

namespace smartalarm {

QString TimeInputNormalizer::filteredText(const QString &input)
{
    QString result;
    bool colonUsed = false;
    int digitCount = 0;
    for (const QChar ch : input) {
        if (ch.isDigit()) {
            if (digitCount < 4) {
                result.append(ch);
                ++digitCount;
            }
        } else if (ch == QLatin1Char(':') && !colonUsed) {
            result.append(ch);
            colonUsed = true;
        }
    }
    return result;
}

std::optional<QTime> TimeInputNormalizer::normalize(const QString &input)
{
    const auto trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return std::nullopt;
    }

    int hours = 0;
    int minutes = 0;
    if (trimmed.contains(QLatin1Char(':'))) {
        const auto parts = trimmed.split(QLatin1Char(':'), Qt::KeepEmptyParts);
        const QString hourText = parts.value(0);
        const QString minuteText = parts.value(1);
        hours = hourText.isEmpty() ? 0 : hourText.left(2).toInt();
        minutes = minuteText.isEmpty() ? 0 : minuteText.left(2).toInt();
    } else {
        QString digits;
        for (const QChar ch : trimmed) {
            if (ch.isDigit() && digits.size() < 4) {
                digits.append(ch);
            }
        }
        if (digits.isEmpty()) {
            return std::nullopt;
        }
        if (digits.size() <= 2) {
            hours = digits.toInt();
            minutes = 0;
        } else if (digits.size() == 3) {
            hours = digits.left(2).toInt();
            minutes = digits.right(1).toInt();
        } else {
            hours = digits.left(2).toInt();
            minutes = digits.right(2).toInt();
        }
    }

    hours = std::clamp(hours, 0, 23);
    minutes = std::clamp(minutes, 0, 59);
    return QTime(hours, minutes);
}

QString TimeInputNormalizer::format(const std::optional<QTime> &time)
{
    return time ? time->toString(QStringLiteral("HH:mm")) : QString();
}

} // namespace smartalarm
