#pragma once

#include "core/notification.h"

#include <QColor>
#include <QLabel>
#include <QVariantAnimation>
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
    void updateBorder(const QColor &borderColor);

    Notification m_notification;
    QWidget *m_container = nullptr;
    QVariantAnimation m_borderAnimation;
};

} // namespace smartalarm
