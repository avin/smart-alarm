#include "ui/tray_controller.h"

#include "ui/main_window.h"

#include <QMenu>
#include <QPainter>

namespace smartalarm {

TrayController::TrayController(AppController *controller, MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_mainWindow(mainWindow)
    , m_tray(this)
{
    auto *menu = new QMenu;
    auto *settings = menu->addAction(QStringLiteral("Notification settings"));
    m_enabledAction = menu->addAction(QStringLiteral("Enabled"));
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
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#303030")), 2));
    painter.setBrush(QColor(QStringLiteral("#f7f7f7")));
    painter.drawEllipse(QRectF(6, 7, 20, 20));
    painter.drawLine(QPointF(16, 17), QPointF(16, 11));
    painter.drawLine(QPointF(16, 17), QPointF(21, 20));
    painter.drawLine(QPointF(10, 6), QPointF(7, 3));
    painter.drawLine(QPointF(22, 6), QPointF(25, 3));
    if (!enabled) {
        painter.setPen(QPen(QColor(QStringLiteral("#D94841")), 4));
        painter.drawLine(QPointF(6, 26), QPointF(26, 6));
    }
    return QIcon(pixmap);
}

} // namespace smartalarm
