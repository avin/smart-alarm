#include "app/minute_scheduler.h"

#include "core/notification.h"

namespace smartalarm {

MinuteScheduler::MinuteScheduler(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (!m_running) {
            return;
        }
        emit minuteTick(QDateTime::currentDateTime());
        scheduleNext();
    });
}

void MinuteScheduler::start()
{
    if (m_running) {
        return;
    }
    m_running = true;
    emit minuteTick(QDateTime::currentDateTime());
    scheduleNext();
}

void MinuteScheduler::stop()
{
    m_running = false;
    m_timer.stop();
}

void MinuteScheduler::scheduleNext()
{
    const auto now = QDateTime::currentDateTime();
    const auto next = normalizeToMinute(now).addSecs(60);
    const auto delay = qMax<qint64>(1, now.msecsTo(next));
    m_timer.start(static_cast<int>(qMin<qint64>(delay, std::numeric_limits<int>::max())));
}

} // namespace smartalarm
