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

std::optional<Notification> AppController::notificationByUuid(const QUuid &id) const
{
    if (const auto *notification = findNotification(id)) {
        return *notification;
    }
    if (m_runtime.runtimeOnlyNotifications.contains(id)) {
        return m_runtime.runtimeOnlyNotifications.value(id);
    }
    return std::nullopt;
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
                m_runtime.intervalResetAt.remove(id);
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
        m_runtime.intervalResetAt.remove(id);
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
                m_runtime.intervalResetAt.remove(id);
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

RuntimeTriggerResult AppController::triggerRuntimeNotification(const RuntimeNotificationOptions &options)
{
    Notification notification;
    notification.id = QUuid::createUuid();
    notification.enabled = true;
    notification.message = options.message;
    notification.color = options.color;
    notification.volume = options.volume;
    notification.playCount = options.playCount;
    notification.sound = options.sound;
    notification.schedule = OnceSchedule { QDate::currentDate(), QTime::currentTime() };

    if (notification.message.trimmed().isEmpty()) {
        return { { false, QStringLiteral("Message is required") }, {} };
    }
    if (!isCanonicalHexColor(notification.color)) {
        return { { false, QStringLiteral("Color is invalid") }, {} };
    }
    notification.color = canonicalHexColor(notification.color);
    if (notification.volume < 0 || notification.volume > 100) {
        return { { false, QStringLiteral("Volume is out of range") }, {} };
    }
    if (notification.playCount < 0 || notification.playCount > 999) {
        return { { false, QStringLiteral("Play count is out of range") }, {} };
    }
    if (options.snoozeMinutes < 0 || options.snoozeMinutes > 1440) {
        return { { false, QStringLiteral("Snooze minutes is out of range") }, {} };
    }
    if (const auto *custom = std::get_if<CustomSound>(&notification.sound); custom && custom->pattern.trimmed().isEmpty()) {
        return { { false, QStringLiteral("Custom sound pattern is required") }, {} };
    }
    if (!m_popupManager) {
        return { { false, QStringLiteral("Notification runtime is not available") }, {} };
    }

    m_runtime.runtimeOnlyNotifications.insert(notification.id, notification);
    m_runtime.runtimeOnlySnoozeMinutes.insert(notification.id, options.snoozeMinutes);
    triggerNotification(notification);
    return { { true, {} }, notification.id };
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

bool AppController::hasActiveNotification(const QUuid &id) const
{
    return m_popupManager && m_popupManager->hasNotification(id);
}

int AppController::activeNotificationCount() const
{
    return m_popupManager ? m_popupManager->activeCount() : 0;
}

QVector<QUuid> AppController::activeNotificationIds() const
{
    return m_popupManager ? m_popupManager->activeNotificationIds() : QVector<QUuid> {};
}

bool AppController::isRuntimeOnlyNotification(const QUuid &id) const
{
    return m_runtime.runtimeOnlyNotifications.contains(id);
}

bool AppController::isAudioPlaying() const
{
    return m_audioQueue && m_audioQueue->isPlaying();
}

std::optional<QDateTime> AppController::nextNotificationTime(const QUuid &id, const QDateTime &from) const
{
    const auto *notification = findNotification(id);
    if (!notification) {
        return std::nullopt;
    }
    return ScheduleEvaluator::nextOccurrence(*notification, from, m_runtime);
}

OperationResult AppController::resetIntervalTimer(const QUuid &id, const QDateTime &now)
{
    const auto *notification = findNotification(id);
    if (!notification) {
        return { false, QStringLiteral("Notification was not found") };
    }
    if (!std::holds_alternative<IntervalSchedule>(notification->schedule)) {
        return { false, QStringLiteral("Notification does not use an interval schedule") };
    }
    m_runtime.pendingSnooze.remove(id);
    m_runtime.lastDismissedAt.remove(id);
    m_runtime.intervalResetAt.insert(id, now);
    return { true, {} };
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

    if (!m_runtime.notificationsEnabled) {
        return;
    }
    const auto runtimeIds = m_runtime.runtimeOnlyNotifications.keys();
    for (const auto &id : runtimeIds) {
        const auto snoozeAt = m_runtime.pendingSnooze.value(id);
        if (!snoozeAt.isValid() || normalizeToMinute(now) < normalizeToMinute(snoozeAt)) {
            continue;
        }
        const auto notification = m_runtime.runtimeOnlyNotifications.value(id);
        m_runtime.pendingSnooze.remove(id);
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
            m_runtime.intervalResetAt.remove(id);
        }
    }
    if (m_popupManager) {
        m_popupManager->closeNotification(id);
    }
    if (m_runtime.runtimeOnlyNotifications.contains(id)) {
        m_runtime.runtimeOnlyNotifications.remove(id);
        m_runtime.runtimeOnlySnoozeMinutes.remove(id);
        m_runtime.pendingSnooze.remove(id);
    }
}

void AppController::snoozeNotification(const QUuid &id)
{
    const auto persistentNotification = findNotification(id);
    const auto runtimeNotification = m_runtime.runtimeOnlyNotifications.value(id);
    const auto *notification = persistentNotification ? persistentNotification : (runtimeNotification.id.isNull() ? nullptr : &runtimeNotification);
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
    if (m_runtime.runtimeOnlySnoozeMinutes.contains(notification.id)) {
        const int minutes = m_runtime.runtimeOnlySnoozeMinutes.value(notification.id);
        if (minutes > 0) {
            return minutes;
        }
    }
    if (const auto *interval = std::get_if<IntervalSchedule>(&notification.schedule)) {
        if (interval->snoozeMinutes > 0) {
            return interval->snoozeMinutes;
        }
    }
    return m_config.settings.defaultSnoozeMinutes;
}

} // namespace smartalarm
