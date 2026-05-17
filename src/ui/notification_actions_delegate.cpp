#include "ui/notification_actions_delegate.h"

#include "ui/lucide_icons.h"

#include <QApplication>
#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

namespace smartalarm {

NotificationActionsDelegate::NotificationActionsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NotificationActionsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    drawButton(painter, option, index, ActionButton::Edit);
    drawButton(painter, option, index, ActionButton::Delete);
}

bool NotificationActionsDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (!model) {
        return false;
    }
    if (event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseButtonRelease) {
        return false;
    }
    const auto *mouse = static_cast<QMouseEvent *>(event);
    if (mouse->button() != Qt::LeftButton) {
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        m_pressedButton = buttonAt(option, mouse->pos());
        m_pressedIndex = m_pressedButton == ActionButton::None ? QPersistentModelIndex() : QPersistentModelIndex(index);
        if (m_pressedButton != ActionButton::None) {
            emit model->dataChanged(index, index, {});
            return true;
        }
        return false;
    }

    if (event->type() != QEvent::MouseButtonRelease) {
        return false;
    }

    const auto releasedButton = buttonAt(option, mouse->pos());
    const auto pressedButton = m_pressedIndex == index ? m_pressedButton : ActionButton::None;
    m_pressedIndex = QPersistentModelIndex();
    m_pressedButton = ActionButton::None;
    emit model->dataChanged(index, index, {});

    if (pressedButton == ActionButton::Edit && releasedButton == ActionButton::Edit) {
        emit editRequested(index.row());
        return true;
    }
    if (pressedButton == ActionButton::Delete && releasedButton == ActionButton::Delete) {
        emit deleteRequested(index.row());
        return true;
    }
    return pressedButton != ActionButton::None;
}

void NotificationActionsDelegate::drawButton(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, ActionButton button) const
{
    QStyleOptionButton buttonOption;
    buttonOption.rect = buttonRect(option, button);
    buttonOption.state = QStyle::State_Enabled | QStyle::State_Raised;
    if (m_pressedIndex == index && m_pressedButton == button) {
        buttonOption.state &= ~QStyle::State_Raised;
        buttonOption.state |= QStyle::State_Sunken;
    }
    buttonOption.icon = lucide::icon(button == ActionButton::Edit ? lucide::Icon::SquarePen : lucide::Icon::Trash2);
    buttonOption.iconSize = QSize(16, 16);

    auto *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_PushButton, &buttonOption, painter, option.widget);
}

NotificationActionsDelegate::ActionButton NotificationActionsDelegate::buttonAt(const QStyleOptionViewItem &option, const QPoint &position) const
{
    if (editRect(option).contains(position)) {
        return ActionButton::Edit;
    }
    if (deleteRect(option).contains(position)) {
        return ActionButton::Delete;
    }
    return ActionButton::None;
}

QRect NotificationActionsDelegate::buttonRect(const QStyleOptionViewItem &option, ActionButton button) const
{
    const int buttonSize = qMin(28, qMax(22, option.rect.height() - 6));
    const int spacing = 6;
    const int totalWidth = buttonSize * 2 + spacing;
    const int left = option.rect.left() + (option.rect.width() - totalWidth) / 2;
    const int top = option.rect.top() + (option.rect.height() - buttonSize) / 2;
    const int offset = button == ActionButton::Delete ? buttonSize + spacing : 0;
    return QRect(left + offset, top, buttonSize, buttonSize);
}

QRect NotificationActionsDelegate::editRect(const QStyleOptionViewItem &option) const
{
    return buttonRect(option, ActionButton::Edit);
}

QRect NotificationActionsDelegate::deleteRect(const QStyleOptionViewItem &option) const
{
    return buttonRect(option, ActionButton::Delete);
}

} // namespace smartalarm
