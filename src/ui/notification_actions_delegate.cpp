#include "ui/notification_actions_delegate.h"

#include "ui/lucide_icons.h"

#include <QMouseEvent>
#include <QPainter>

namespace smartalarm {

NotificationActionsDelegate::NotificationActionsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NotificationActionsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    lucide::icon(lucide::Icon::SquarePen).paint(painter, editRect(option));
    lucide::icon(lucide::Icon::Trash2).paint(painter, deleteRect(option));
}

bool NotificationActionsDelegate::editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() != QEvent::MouseButtonRelease) {
        return false;
    }
    const auto *mouse = static_cast<QMouseEvent *>(event);
    if (editRect(option).contains(mouse->pos())) {
        emit editRequested(index.row());
        return true;
    }
    if (deleteRect(option).contains(mouse->pos())) {
        emit deleteRequested(index.row());
        return true;
    }
    return false;
}

QRect NotificationActionsDelegate::editRect(const QStyleOptionViewItem &option) const
{
    return QRect(option.rect.left() + 10, option.rect.top() + 5, 22, 22);
}

QRect NotificationActionsDelegate::deleteRect(const QStyleOptionViewItem &option) const
{
    return QRect(option.rect.left() + 42, option.rect.top() + 5, 22, 22);
}

} // namespace smartalarm
