#include "ui/tray_controller.h"

#include "ui/lucide_icons.h"
#include "ui/main_window.h"

#include <QMenu>

namespace smartalarm {

TrayController::TrayController(AppController *controller, MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_mainWindow(mainWindow)
    , m_tray(this)
{
    auto *menu = new QMenu;
    auto *settings = menu->addAction(QStringLiteral("Notification settings"));
    settings->setIcon(lucide::icon(lucide::Icon::Settings));
    m_enabledAction = menu->addAction(QStringLiteral("Enabled"));
    m_enabledAction->setIcon(lucide::icon(m_controller->runtimeNotificationsEnabled() ? lucide::Icon::Bell : lucide::Icon::BellOff));
    m_enabledAction->setCheckable(true);
    m_enabledAction->setChecked(m_controller->runtimeNotificationsEnabled());
    menu->addSeparator();
    auto *exit = menu->addAction(QStringLiteral("Exit"));
    m_tray.setContextMenu(menu);
    m_tray.setToolTip(QStringLiteral("Smart Alarm"));
    m_tray.setIcon(makeIcon(m_controller->runtimeNotificationsEnabled()));
    connect(settings, &QAction::triggered, m_mainWindow, &MainWindow::showSettingsWindow);
    connect(exit, &QAction::triggered, this, &TrayController::exitRequested);
    connect(m_enabledAction, &QAction::toggled, m_controller, &AppController::setRuntimeNotificationsEnabled);
    connect(m_controller, &AppController::runtimeToggleChanged, this, [this](bool enabled) {
        QSignalBlocker blocker(m_enabledAction);
        m_enabledAction->setChecked(enabled);
        m_enabledAction->setIcon(lucide::icon(enabled ? lucide::Icon::Bell : lucide::Icon::BellOff));
        m_tray.setIcon(makeIcon(enabled));
    });
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            m_mainWindow->showSettingsWindow();
        }
    });
}

void TrayController::show()
{
    m_tray.show();
}

void TrayController::hide()
{
    m_tray.hide();
}

QIcon TrayController::makeIcon(bool enabled) const
{
    return lucide::icon(enabled ? lucide::Icon::AlarmClock : lucide::Icon::AlarmClockOff);
}

} // namespace smartalarm
