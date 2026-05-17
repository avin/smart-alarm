#pragma once

#include "core/notification.h"

namespace smartalarm {

class ScheduleFormatter {
public:
    static QString format(const Schedule &schedule);
};

} // namespace smartalarm
