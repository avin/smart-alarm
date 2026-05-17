#pragma once

#include "core/notification.h"

#include <QObject>

namespace smartalarm {

class NotificationPopup;

class PopupManager : public QObject {
    Q_OBJECT
public:
    explicit PopupManager(QObject *parent = nullptr);

    void setPosition(NotificationPosition position);
    bool hasNotification(const QUuid &id) const;
    void showNotification(const Notification &notification);
    void closeNotification(const QUuid &id);
    void closeAll();

signals:
    void dismissed(const QUuid &id);
    void snoozed(const QUuid &id);

private:
    void repositionAll();

    NotificationPosition m_position = NotificationPosition::TopRight;
    QHash<QUuid, NotificationPopup *> m_popups;
    QVector<QUuid> m_order;
};

} // namespace smartalarm
