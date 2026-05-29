#include "ui/main_window.h"

#include "ui/app_icon.h"
#include "ui/global_settings_dialog.h"
#include "ui/lucide_icons.h"
#include "ui/notification_actions_delegate.h"
#include "ui/notification_editor_dialog.h"
#include "ui/notification_state_delegate.h"

#include <QCloseEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>

namespace smartalarm {

namespace {

QString notificationsStateText(bool enabled)
{
    return enabled ? QStringLiteral("Notifications enabled") : QStringLiteral("Notifications disabled");
}

bool hasIntervalSchedule(const Notification &notification)
{
    return std::holds_alternative<IntervalSchedule>(notification.schedule);
}

} // namespace

MainWindow::MainWindow(AppController *controller, audio::AudioQueue *audioQueue, audio::PreviewPlayer *previewPlayer, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
    , m_audioQueue(audioQueue)
    , m_previewPlayer(previewPlayer)
    , m_central(new QWidget(this))
{
    setWindowTitle(QStringLiteral("Smart Alarm"));
    setWindowIcon(appicon::alarm());
    resize(900, 520);
    setMinimumSize(720, 420);
    auto *layout = new QVBoxLayout(m_central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);
    setCentralWidget(m_central);
    createTopBar();
    createTable();
    connect(m_controller, &AppController::runtimeToggleChanged, this, [this](bool enabled) {
        m_runtimeToggle->setChecked(enabled);
    });
    connect(m_controller, &AppController::saveFailed, this, [this](const QString &message) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), message);
    });
}

void MainWindow::setExiting(bool exiting)
{
    m_exiting = exiting;
}

void MainWindow::showSettingsWindow()
{
    if (isMinimized()) {
        showNormal();
    } else {
        show();
    }
    raise();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_exiting) {
        event->accept();
        return;
    }
    event->ignore();
    hide();
}

void MainWindow::createTopBar()
{
    auto *bar = new QHBoxLayout;
    auto *newButton = new QPushButton(QStringLiteral("New notification"), this);
    newButton->setIcon(lucide::icon(lucide::Icon::Plus));
    bar->addWidget(newButton);
    bar->addStretch();
    m_runtimeToggle = new QToolButton(this);
    m_runtimeToggle->setIcon(lucide::icon(m_controller->runtimeNotificationsEnabled() ? lucide::Icon::Bell : lucide::Icon::BellOff));
    m_runtimeToggle->setCheckable(true);
    m_runtimeToggle->setChecked(m_controller->runtimeNotificationsEnabled());
    m_runtimeToggle->setToolTip(notificationsStateText(m_controller->runtimeNotificationsEnabled()));
    auto *settings = new QToolButton(this);
    settings->setIcon(lucide::icon(lucide::Icon::Settings));
    settings->setToolTip(QStringLiteral("Settings"));
    bar->addWidget(m_runtimeToggle);
    bar->addWidget(settings);
    qobject_cast<QVBoxLayout *>(m_central->layout())->addLayout(bar);
    connect(newButton, &QPushButton::clicked, this, &MainWindow::createNotification);
    connect(settings, &QToolButton::clicked, this, &MainWindow::openGlobalSettings);
    connect(m_runtimeToggle, &QToolButton::toggled, m_controller, &AppController::setRuntimeNotificationsEnabled);
    connect(m_controller, &AppController::runtimeToggleChanged, this, [this](bool enabled) {
        m_runtimeToggle->setIcon(lucide::icon(enabled ? lucide::Icon::Bell : lucide::Icon::BellOff));
        m_runtimeToggle->setToolTip(notificationsStateText(enabled));
    });
}

void MainWindow::createTable()
{
    m_table = new QTableView(this);
    m_model = new NotificationTableModel(m_controller, this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->setColumnWidth(NotificationTableModel::EnabledColumn, 36);
    m_table->setColumnWidth(NotificationTableModel::ColorColumn, 42);
    m_table->setColumnWidth(NotificationTableModel::ActionsColumn, 64);
    m_table->horizontalHeader()->setSectionResizeMode(NotificationTableModel::MessageColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(NotificationTableModel::ScheduleColumn, QHeaderView::Stretch);
    auto *delegate = new NotificationActionsDelegate(this);
    m_table->setItemDelegateForColumn(NotificationTableModel::ActionsColumn, delegate);
    auto *stateDelegate = new NotificationStateDelegate(this);
    m_table->setItemDelegateForColumn(NotificationTableModel::EnabledColumn, stateDelegate);
    m_table->setItemDelegateForColumn(NotificationTableModel::ColorColumn, stateDelegate);
    connect(delegate, &NotificationActionsDelegate::editRequested, this, &MainWindow::editNotification);
    connect(delegate, &NotificationActionsDelegate::deleteRequested, this, &MainWindow::deleteNotification);
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) {
            return;
        }
        if (index.column() == NotificationTableModel::MessageColumn) {
            editNotification(index.row());
        } else if (index.column() == NotificationTableModel::ScheduleColumn) {
            showScheduleDetails(index);
        }
    });
    qobject_cast<QVBoxLayout *>(m_central->layout())->addWidget(m_table, 1);
}

void MainWindow::createNotification()
{
    Notification notification;
    notification.schedule = OnceSchedule { QDate::currentDate(), std::nullopt };
    NotificationEditorDialog dialog(notification, m_previewPlayer, !m_audioQueue->isPlaying(), this);
    while (dialog.exec() == QDialog::Accepted) {
        if (saveEditorResult(std::nullopt, dialog)) {
            break;
        }
    }
}

void MainWindow::editNotification(int row)
{
    const auto notification = m_model->notificationAt(row);
    if (!notification) {
        return;
    }
    NotificationEditorDialog dialog(*notification, m_previewPlayer, !m_audioQueue->isPlaying(), this);
    while (dialog.exec() == QDialog::Accepted) {
        if (saveEditorResult(notification->id, dialog)) {
            break;
        }
    }
}

void MainWindow::deleteNotification(int row)
{
    const auto notification = m_model->notificationAt(row);
    if (!notification) {
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete"), QStringLiteral("Delete selected notification?")) != QMessageBox::Yes) {
        return;
    }
    m_controller->deleteNotification(notification->id);
}

void MainWindow::showScheduleDetails(const QModelIndex &index)
{
    if (!index.isValid() || index.column() != NotificationTableModel::ScheduleColumn) {
        return;
    }
    const auto notification = m_model->notificationAt(index.row());
    if (!notification) {
        return;
    }

    const auto rect = m_table->visualRect(index);
    const auto popupPosition = m_table->viewport()->mapToGlobal(rect.bottomLeft());
    QMenu menu(this);
    auto *nextAction = menu.addAction(nextNotificationText(*notification, QDateTime::currentDateTime()));
    nextAction->setEnabled(false);
    if (hasIntervalSchedule(*notification)) {
        menu.addSeparator();
        auto *resetAction = menu.addAction(QStringLiteral("Reset interval timer"));
        const auto selected = menu.exec(popupPosition);
        if (selected == resetAction) {
            const auto result = m_controller->resetIntervalTimer(notification->id, QDateTime::currentDateTime());
            if (!result.ok) {
                QMessageBox::warning(this, QStringLiteral("Reset failed"), result.errorMessage);
                return;
            }
            QToolTip::showText(popupPosition, nextNotificationText(*notification, QDateTime::currentDateTime()), m_table);
        }
        return;
    }
    menu.exec(popupPosition);
}

void MainWindow::openGlobalSettings()
{
    GlobalSettingsDialog dialog(m_controller->settings(), this);
    while (dialog.exec() == QDialog::Accepted) {
        const auto result = m_controller->updateSettings(dialog.settings());
        if (result.ok) {
            break;
        }
        QMessageBox::critical(this, QStringLiteral("Save failed"), result.errorMessage);
    }
}

bool MainWindow::saveEditorResult(const std::optional<QUuid> &existingId, NotificationEditorDialog &dialog)
{
    const auto notification = dialog.notification();
    const auto result = existingId
        ? m_controller->updateNotification(*existingId, notification)
        : m_controller->addNotification(notification);
    if (!result.ok) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), result.errorMessage);
    }
    return result.ok;
}

QString MainWindow::nextNotificationText(const Notification &notification, const QDateTime &now) const
{
    if (!m_controller->runtimeNotificationsEnabled()) {
        return QStringLiteral("Notifications are disabled");
    }
    if (!notification.enabled) {
        return QStringLiteral("Notification is disabled");
    }
    const auto next = m_controller->nextNotificationTime(notification.id, now);
    if (!next) {
        return QStringLiteral("No upcoming notification");
    }
    return QStringLiteral("Next notification: %1").arg(next->toString(QStringLiteral("yyyy-MM-dd HH:mm")));
}

} // namespace smartalarm
