#pragma once

#include "app/app_controller.h"
#include "audio/audio_queue.h"
#include "audio/audio_player.h"
#include "ui/notification_table_model.h"

#include <QMainWindow>
#include <QTableView>

#include <optional>

class QToolButton;

namespace smartalarm {

class NotificationEditorDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppController *controller, audio::AudioQueue *audioQueue, audio::PreviewPlayer *previewPlayer, QWidget *parent = nullptr);

    void setExiting(bool exiting);
    void showSettingsWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createTopBar();
    void createTable();
    void createNotification();
    void editNotification(int row);
    void deleteNotification(int row);
    void showScheduleDetails(const QModelIndex &index);
    void openGlobalSettings();
    bool saveEditorResult(const std::optional<QUuid> &existingId, NotificationEditorDialog &dialog);
    QString nextNotificationText(const Notification &notification, const QDateTime &now) const;

    AppController *m_controller = nullptr;
    audio::AudioQueue *m_audioQueue = nullptr;
    audio::PreviewPlayer *m_previewPlayer = nullptr;
    NotificationTableModel *m_model = nullptr;
    QTableView *m_table = nullptr;
    QWidget *m_central = nullptr;
    QToolButton *m_runtimeToggle = nullptr;
    bool m_exiting = false;
};

} // namespace smartalarm
