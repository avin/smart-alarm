#pragma once

#include <QIcon>
#include <QString>

namespace smartalarm::lucide {

enum class Icon {
    AlarmClock,
    AlarmClockOff,
    Bell,
    BellOff,
    ChevronDown,
    ChevronRight,
    CircleQuestionMark,
    Play,
    Plus,
    Settings,
    SquarePen,
    Trash2,
};

QIcon icon(Icon icon);
QString path(Icon icon);

} // namespace smartalarm::lucide
