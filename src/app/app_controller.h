#pragma once

#include "audio/audio_queue.h"
#include "audio/audio_player.h"
#include "core/notification.h"
#include "storage/config_store.h"

#include <QObject>

#include <optional>

namespace smartalarm {

class PopupManager;

struct OperationResult {
    bool ok = false;
    QString errorMessage;
};

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(ConfigStore store, QObject *parent = nullptr);

    LoadResult load();
    void setRuntimeServices(PopupManager *popupManager, audio::AudioQueue *audioQueue, audio::PreviewPlayer *previewPlayer);

    const AppConfig &config() const;
    const QVector<Notification> &notifications() const;
    const GlobalSettings &settings() const;
    RuntimeState &runtime();

    OperationResult addNotification(const Notification &notification);
    OperationResult updateNotification(const QUuid &id, const Notification &notification);
    OperationResult deleteNotification(const QUuid &id);
    OperationResult setNotificationEnabled(const QUuid &id, bool enabled);
    OperationResult updateSettings(const GlobalSettings &settings);

    bool runtimeNotificationsEnabled() const;
    void setRuntimeNotificationsEnabled(bool enabled);
    std::optional<QDateTime> nextNotificationTime(const QUuid &id, const QDateTime &from) const;
    OperationResult resetIntervalTimer(const QUuid &id, const QDateTime &now);

    void handleMinuteTick(const QDateTime &now);
    void dismissNotification(const QUuid &id);
    void snoozeNotification(const QUuid &id);
    void stopAllRuntime();

signals:
    void notificationsChanged();
    void settingsChanged();
    void runtimeToggleChanged(bool enabled);
    void saveFailed(const QString &message);

private:
    OperationResult saveReplacing(AppConfig nextConfig);
    Notification *findNotification(const QUuid &id);
    const Notification *findNotification(const QUuid &id) const;
    void triggerNotification(const Notification &notification);
    QString patternFor(const Notification &notification) const;
    int resolvedSnoozeMinutes(const Notification &notification) const;

    ConfigStore m_store;
    AppConfig m_config;
    RuntimeState m_runtime;
    PopupManager *m_popupManager = nullptr;
    audio::AudioQueue *m_audioQueue = nullptr;
    audio::PreviewPlayer *m_previewPlayer = nullptr;
};

} // namespace smartalarm
