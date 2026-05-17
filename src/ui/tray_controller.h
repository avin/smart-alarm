#pragma once

#include "app/app_controller.h"

#include <QAction>
#include <QSystemTrayIcon>

namespace smartalarm {

class MainWindow;

class TrayController : public QObject {
    Q_OBJECT
public:
    TrayController(AppController *controller, MainWindow *mainWindow, QObject *parent = nullptr);

    void show();
    void hide();

signals:
    void exitRequested();

private:
    QIcon makeIcon(bool enabled) const;

    AppController *m_controller = nullptr;
    MainWindow *m_mainWindow = nullptr;
    QSystemTrayIcon m_tray;
    QAction *m_enabledAction = nullptr;
};

} // namespace smartalarm
