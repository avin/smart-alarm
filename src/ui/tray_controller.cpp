#include "ui/tray_controller.h"

#include "ui/app_icon.h"
#include "ui/lucide_icons.h"
#include "ui/main_window.h"

#include <QImage>
#include <QMenu>
#include <QPainter>

namespace smartalarm {

namespace {

QPixmap grayscalePixmap(const QIcon &icon, const QSize &size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        icon.paint(&painter, pixmap.rect());
    }

    auto image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int gray = qGray(pixel);
            line[x] = qRgba(gray, gray, gray, qAlpha(pixel));
        }
    }

    return QPixmap::fromImage(image);
}

} // namespace

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
    const QIcon baseIcon = appicon::alarm();
    if (enabled) {
        return baseIcon;
    }

    QPixmap pixmap = grayscalePixmap(baseIcon, QSize(64, 64));
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#D94841")), 7, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(10, 54), QPointF(54, 10));

    return QIcon(pixmap);
}

} // namespace smartalarm
