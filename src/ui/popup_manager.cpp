#include "ui/popup_manager.h"

#include "ui/notification_popup.h"

#include <QGuiApplication>
#include <QScreen>

namespace smartalarm {

namespace {
constexpr int Margin = 16;
constexpr int Gap = 12;
}

PopupManager::PopupManager(QObject *parent)
    : QObject(parent)
{
}

void PopupManager::setPosition(NotificationPosition position)
{
    m_position = position;
    repositionAll();
}

bool PopupManager::hasNotification(const QUuid &id) const
{
    return m_popups.contains(id);
}

int PopupManager::activeCount() const
{
    return m_popups.size();
}

QVector<QUuid> PopupManager::activeNotificationIds() const
{
    return m_order;
}

void PopupManager::showNotification(const Notification &notification)
{
    if (m_popups.contains(notification.id)) {
        return;
    }
    auto *popup = new NotificationPopup(notification);
    m_popups.insert(notification.id, popup);
    m_order.push_back(notification.id);
    connect(popup, &NotificationPopup::dismissed, this, &PopupManager::dismissed);
    connect(popup, &NotificationPopup::snoozed, this, &PopupManager::snoozed);
    connect(popup, &QObject::destroyed, this, [this, id = notification.id] {
        m_popups.remove(id);
        m_order.removeAll(id);
        repositionAll();
    });
    popup->show();
    repositionAll();
}

void PopupManager::closeNotification(const QUuid &id)
{
    auto *popup = m_popups.take(id);
    m_order.removeAll(id);
    if (popup) {
        popup->close();
        popup->deleteLater();
    }
    repositionAll();
}

void PopupManager::closeAll()
{
    const auto ids = m_order;
    for (const auto &id : ids) {
        closeNotification(id);
    }
}

void PopupManager::repositionAll()
{
    const auto *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }
    const QRect area = screen->availableGeometry();
    int totalHeight = 0;
    for (const auto &id : m_order) {
        if (auto *popup = m_popups.value(id)) {
            totalHeight += popup->height();
        }
    }
    totalHeight += qMax(0, m_order.size() - 1) * Gap;

    int y = area.top() + Margin;
    if (m_position == NotificationPosition::BottomLeft || m_position == NotificationPosition::BottomRight) {
        y = area.bottom() - Margin;
    } else if (m_position == NotificationPosition::Center) {
        y = area.center().y() - totalHeight / 2;
    }

    for (const auto &id : m_order) {
        auto *popup = m_popups.value(id);
        if (!popup) {
            continue;
        }
        const int x = [&] {
            switch (m_position) {
            case NotificationPosition::TopLeft:
            case NotificationPosition::BottomLeft:
                return area.left() + Margin;
            case NotificationPosition::TopRight:
            case NotificationPosition::BottomRight:
                return area.right() - Margin - popup->width();
            case NotificationPosition::Center:
                return area.center().x() - popup->width() / 2;
            }
            return area.right() - Margin - popup->width();
        }();
        if (m_position == NotificationPosition::BottomLeft || m_position == NotificationPosition::BottomRight) {
            y -= popup->height();
            popup->move(x, y);
            y -= Gap;
        } else {
            popup->move(x, y);
            y += popup->height() + Gap;
        }
    }
}

} // namespace smartalarm
