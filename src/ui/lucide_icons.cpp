#include "ui/lucide_icons.h"

namespace smartalarm::lucide {

QString path(Icon icon)
{
    switch (icon) {
    case Icon::AlarmClock:
        return QStringLiteral(":/icons/alarm-clock.svg");
    case Icon::AlarmClockOff:
        return QStringLiteral(":/icons/alarm-clock-off.svg");
    case Icon::Bell:
        return QStringLiteral(":/icons/bell.svg");
    case Icon::BellOff:
        return QStringLiteral(":/icons/bell-off.svg");
    case Icon::ChevronDown:
        return QStringLiteral(":/icons/chevron-down.svg");
    case Icon::ChevronRight:
        return QStringLiteral(":/icons/chevron-right.svg");
    case Icon::CircleQuestionMark:
        return QStringLiteral(":/icons/circle-question-mark.svg");
    case Icon::Play:
        return QStringLiteral(":/icons/play.svg");
    case Icon::Plus:
        return QStringLiteral(":/icons/plus.svg");
    case Icon::RefreshCw:
        return QStringLiteral(":/icons/refresh-cw.svg");
    case Icon::Settings:
        return QStringLiteral(":/icons/settings.svg");
    case Icon::SquarePen:
        return QStringLiteral(":/icons/square-pen.svg");
    case Icon::Trash2:
        return QStringLiteral(":/icons/trash-2.svg");
    case Icon::X:
        return QStringLiteral(":/icons/x.svg");
    }
    return {};
}

QIcon icon(Icon icon)
{
    return QIcon(path(icon));
}

} // namespace smartalarm::lucide
