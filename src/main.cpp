#include "app/app_controller.h"
#include "app/command_server.h"
#include "app/minute_scheduler.h"
#include "audio/audio_player.h"
#include "audio/audio_queue.h"
#include "ui/main_window.h"
#include "ui/popup_manager.h"
#include "ui/tray_controller.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QSystemTrayIcon>

using namespace smartalarm;

namespace {

void applyApplicationStyle(QApplication &app)
{
    if (auto *fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app.setStyle(fusion);
    }
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#f0f0f0")));
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f5f5f5")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#ffffe1")));
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#f0f0f0")));
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#0078d7")));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);
    app.setStyleSheet(QStringLiteral(
        "QToolTip { color: #000000; background-color: #ffffe1; border: 1px solid #b5b5b5; }"
        "QMenu::item:selected { background-color: #0078d7; color: #ffffff; }"));
}

QString lockPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("smart-alarm.lock"));
}

void applyApplicationMetadata()
{
    QCoreApplication::setOrganizationName(QStringLiteral("SmartAlarm"));
    QCoreApplication::setApplicationName(QStringLiteral("SmartAlarm"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    applyApplicationMetadata();
    QApplication::setApplicationDisplayName(QStringLiteral("Smart Alarm"));
    QApplication::setQuitOnLastWindowClosed(false);
    applyApplicationStyle(app);

    QLockFile lock(lockPath());
    lock.setStaleLockTime(0);
    if (!lock.tryLock(100)) {
        QMessageBox::warning(nullptr, QStringLiteral("Smart Alarm"), QStringLiteral("Application is already running"));
        return 0;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QStringLiteral("Smart Alarm"), QStringLiteral("System tray is not available"));
        return 1;
    }

    AppController controller(ConfigStore {});
    controller.load();

    audio::AudioQueue audioQueue;
    audio::PreviewPlayer previewPlayer;
    PopupManager popupManager;
    popupManager.setPosition(controller.settings().notificationPosition);
    controller.setRuntimeServices(&popupManager, &audioQueue, &previewPlayer);

    MainWindow mainWindow(&controller, &audioQueue, &previewPlayer);
    TrayController tray(&controller, &mainWindow);
    MinuteScheduler scheduler;
    CommandServer commandServer(&controller);

    QObject::connect(&scheduler, &MinuteScheduler::minuteTick, &controller, &AppController::handleMinuteTick);
    QObject::connect(&popupManager, &PopupManager::dismissed, &controller, &AppController::dismissNotification);
    QObject::connect(&popupManager, &PopupManager::snoozed, &controller, &AppController::snoozeNotification);
    QObject::connect(&tray, &TrayController::exitRequested, &app, [&] {
        scheduler.stop();
        mainWindow.setExiting(true);
        controller.stopAllRuntime();
        tray.hide();
        app.quit();
    });

    tray.show();
    if (!commandServer.start()) {
        qWarning("Failed to start CLI command server.");
    }
    scheduler.start();
    return app.exec();
}
