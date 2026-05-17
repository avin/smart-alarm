#include "ui/notification_actions_delegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

namespace smartalarm {

NotificationActionsDelegate::NotificationActionsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NotificationActionsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    const auto editIcon = QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    const auto deleteIcon = QApplication::style()->standardIcon(QStyle::SP_TrashIcon);
    editIcon.paint(painter, editRect(option));
    deleteIcon.paint(painter, deleteRect(option));
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
