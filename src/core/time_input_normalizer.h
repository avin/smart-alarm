#pragma once

#include <QTime>

#include <optional>

namespace smartalarm {

class TimeInputNormalizer {
public:
    static QString filteredText(const QString &input);
    static std::optional<QTime> normalize(const QString &input);
    static QString format(const std::optional<QTime> &time);
};

} // namespace smartalarm
