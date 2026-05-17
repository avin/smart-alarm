#include "app/app_controller.h"

#include "audio/sound_pattern_parser.h"
#include "audio/sound_preset_registry.h"
#include "audio/tone_generator.h"
#include "core/notification_validator.h"
#include "core/schedule_evaluator.h"
#include "ui/popup_manager.h"

namespace smartalarm {

AppController::AppController(ConfigStore store, QObject *parent)
    : QObject(parent)
    , m_store(std::move(store))
{
}

LoadResult AppController::load()
{
    auto result = m_store.load();
    m_config = result.config;
    return result;
}

void AppController::setRuntimeServices(PopupManager *popupManager, audio::AudioQueue *audioQueue, audio::PreviewPlayer *previewPlayer)
{
    m_popupManager = popupManager;
    m_audioQueue = audioQueue;
    m_previewPlayer = previewPlayer;
}

const AppConfig &AppController::config() const
{
    return m_config;
}

const QVector<Notification> &AppController::notifications() const
{
    return m_config.notifications;
}

const GlobalSettings &AppController::settings() const
{
    return m_config.settings;
}

RuntimeState &AppController::runtime()
{
    return m_runtime;
}

OperationResult AppController::addNotification(const Notification &notification)
{
    auto validation = NotificationValidator::validate(notification);
    if (!validation.ok()) {
        return { false, QStringLiteral("Notification is invalid") };
    }
    auto next = m_config;
    next.notifications.push_back(notification);
    return saveReplacing(std::move(next));
}

OperationResult AppController::updateNotification(const QUuid &id, const Notification &notification)
{
    auto validation = NotificationValidator::validate(notification);
    if (!validation.ok()) {
        return { false, QStringLiteral("Notification is invalid") };
    }
    auto next = m_config;
    for (auto &item : next.notifications) {
        if (item.id == id) {
            item = notification;
            item.id = id;
            const auto result = saveReplacing(std::move(next));
            if (result.ok) {
                m_runtime.pendingSnooze.remove(id);
            }
            return result;
        }
    }
    return { false, QStringLiteral("Notification was not found") };
}

OperationResult AppController::deleteNotification(const QUuid &id)
{
    auto next = m_config;
    const qsizetype before = next.notifications.size();
    next.notifications.erase(std::remove_if(next.notifications.begin(), next.notifications.end(), [&](const Notification &item) {
        return item.id == id;
    }), next.notifications.end());
    if (before == next.notifications.size()) {
        return { false, QStringLiteral("Notification was not found") };
    }
    const auto result = saveReplacing(std::move(next));
    if (result.ok) {
        m_runtime.pendingSnooze.remove(id);
        m_runtime.lastDismissedAt.remove(id);
        if (m_audioQueue) m_audioQueue->stopNotification(id);
        if (m_popupManager) m_popupManager->closeNotification(id);
    }
    return result;
}

OperationResult AppController::setNotificationEnabled(const QUuid &id, bool enabled)
{
    auto next = m_config;
    for (auto &item : next.notifications) {
        if (item.id == id) {
            item.enabled = enabled;
            const auto result = saveReplacing(std::move(next));
            if (result.ok && !enabled) {
                m_runtime.pendingSnooze.remove(id);
                if (m_audioQueue) m_audioQueue->stopNotification(id);
                if (m_popupManager) m_popupManager->closeNotification(id);
            }
            return result;
        }
    }
    return { false, QStringLiteral("Notification was not found") };
}

OperationResult AppController::updateSettings(const GlobalSettings &settings)
{
    if (settings.defaultSnoozeMinutes < 1 || settings.defaultSnoozeMinutes > 1440) {
        return { false, QStringLiteral("Settings are invalid") };
    }
    auto next = m_config;
    next.settings = settings;
    const auto result = saveReplacing(std::move(next));
    if (result.ok && m_popupManager) {
        m_popupManager->setPosition(settings.notificationPosition);
    }
    return result;
}

bool AppController::runtimeNotificationsEnabled() const
{
    return m_runtime.notificationsEnabled;
}

void AppController::setRuntimeNotificationsEnabled(bool enabled)
{
    if (m_runtime.notificationsEnabled == enabled) {
        return;
    }
    m_runtime.notificationsEnabled = enabled;
    emit runtimeToggleChanged(enabled);
}

void AppController::handleMinuteTick(const QDateTime &now)
{
    for (const auto &notification : m_config.notifications) {
        const auto decision = ScheduleEvaluator::evaluate(notification, now, m_runtime);
        if (!decision.due) {
            continue;
        }
        if (decision.kind == TriggerKind::Normal) {
            m_runtime.pendingSnooze.remove(notification.id);
        } else if (decision.kind == TriggerKind::Snooze) {
            m_runtime.pendingSnooze.remove(notification.id);
        }
        m_runtime.lastTriggeredMinute.insert(notification.id, normalizeToMinute(now));
        triggerNotification(notification);
    }
}

void AppController::dismissNotification(const QUuid &id)
{
    if (m_audioQueue) {
        m_audioQueue->stopNotification(id);
    }
    const auto *notification = findNotification(id);
    if (notification && std::holds_alternative<IntervalSchedule>(notification->schedule)) {
        const auto &interval = std::get<IntervalSchedule>(notification->schedule);
        if (interval.countFrom == CountFrom::Confirmation) {
            m_runtime.lastDismissedAt.insert(id, QDateTime::currentDateTime());
        }
    }
    if (m_popupManager) {
        m_popupManager->closeNotification(id);
    }
}

void AppController::snoozeNotification(const QUuid &id)
{
    const auto *notification = findNotification(id);
    if (!notification) {
        return;
    }
    if (m_audioQueue) {
        m_audioQueue->stopNotification(id);
    }
    const int minutes = qMax(1, resolvedSnoozeMinutes(*notification));
    m_runtime.pendingSnooze.insert(id, ceilToNextMinute(QDateTime::currentDateTime().addSecs(minutes * 60)));
    if (m_popupManager) {
        m_popupManager->closeNotification(id);
    }
}

void AppController::stopAllRuntime()
{
    if (m_previewPlayer) {
        m_previewPlayer->stop();
    }
    if (m_audioQueue) {
        m_audioQueue->clear();
    }
    if (m_popupManager) {
        m_popupManager->closeAll();
    }
}

OperationResult AppController::saveReplacing(AppConfig nextConfig)
{
    const auto result = m_store.save(nextConfig);
    if (!result.ok) {
        emit saveFailed(result.errorMessage);
        return { false, result.errorMessage };
    }
    m_config = std::move(nextConfig);
    emit notificationsChanged();
    emit settingsChanged();
    return { true, {} };
}

Notification *AppController::findNotification(const QUuid &id)
{
    for (auto &notification : m_config.notifications) {
        if (notification.id == id) {
            return &notification;
        }
    }
    return nullptr;
}

const Notification *AppController::findNotification(const QUuid &id) const
{
    for (const auto &notification : m_config.notifications) {
        if (notification.id == id) {
            return &notification;
        }
    }
    return nullptr;
}

void AppController::triggerNotification(const Notification &notification)
{
    if (!m_popupManager || m_popupManager->hasNotification(notification.id)) {
        return;
    }
    m_popupManager->showNotification(notification);
    if (notification.volume <= 0 || !m_audioQueue) {
        return;
    }
    const auto parsed = audio::SoundPatternParser::parse(patternFor(notification));
    if (!parsed.ok) {
        qWarning().noquote() << "Cannot play notification sound:" << parsed.errorMessage;
        return;
    }
    m_audioQueue->enqueue({ notification.id, parsed.segments, notification.volume, notification.playCount });
}

QString AppController::patternFor(const Notification &notification) const
{
    return std::visit([](const auto &sound) {
        using T = std::decay_t<decltype(sound)>;
        if constexpr (std::is_same_v<T, PresetSound>) {
            return audio::SoundPresetRegistry::patternFor(sound.preset);
        } else {
            return sound.pattern;
        }
    }, notification.sound);
}

int AppController::resolvedSnoozeMinutes(const Notification &notification) const
{
    if (const auto *interval = std::get_if<IntervalSchedule>(&notification.schedule)) {
        if (interval->snoozeMinutes > 0) {
            return interval->snoozeMinutes;
        }
    }
    return m_config.settings.defaultSnoozeMinutes;
}

} // namespace smartalarm
