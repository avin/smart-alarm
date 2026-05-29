#pragma once

#include "core/notification.h"

#include <optional>

namespace smartalarm {

enum class TriggerKind {
    None,
    Normal,
    Snooze
};

struct TriggerDecision {
    TriggerKind kind = TriggerKind::None;
    bool due = false;
};

class ScheduleEvaluator {
public:
    static bool isNormalDue(const Notification &notification, const QDateTime &now, const RuntimeState &runtime);
    static TriggerDecision evaluate(const Notification &notification, const QDateTime &now, const RuntimeState &runtime);
    static bool isOnceOverdue(const Notification &notification, const QDateTime &now);
    static std::optional<QDateTime> nextOccurrence(const Notification &notification, const QDateTime &from, const RuntimeState &runtime);
};

} // namespace smartalarm
