#pragma once

#include "core/notification.h"

#include <QLabel>
#include <QTimer>
#include <QWidget>

namespace smartalarm {

class SlideToDismiss;

class NotificationPopup : public QWidget {
    Q_OBJECT
public:
    explicit NotificationPopup(Notification notification, QWidget *parent = nullptr);

    QUuid notificationId() const;

signals:
    void dismissed(const QUuid &id);
    void snoozed(const QUuid &id);

private:
    void updateBorder();

    Notification m_notification;
    QWidget *m_container = nullptr;
    QTimer m_blinkTimer;
    bool m_blink = false;
};

} // namespace smartalarm
