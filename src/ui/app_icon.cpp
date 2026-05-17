#include "ui/app_icon.h"

#include <QString>

namespace smartalarm::appicon {

QIcon alarm()
{
    return QIcon(QStringLiteral(":/icons/tray-alarm.svg"));
}

} // namespace smartalarm::appicon
