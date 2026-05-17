#include "ui/notification_state_delegate.h"

#include "ui/notification_table_model.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

namespace smartalarm {

namespace {
QStyleOptionViewItem baseOptionWithoutSelection(const QStyleOptionViewItem &option)
{
    auto base = option;
    base.state &= ~QStyle::State_Selected;
    base.state &= ~QStyle::State_HasFocus;
    return base;
}

QRect centeredRect(const QRect &bounds, const QSize &size)
{
    return QRect(
        bounds.left() + (bounds.width() - size.width()) / 2,
        bounds.top() + (bounds.height() - size.height()) / 2,
        size.width(),
        size.height());
}
}

NotificationStateDelegate::NotificationStateDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NotificationStateDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const auto base = baseOptionWithoutSelection(option);
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &base, painter);

    if (index.column() == NotificationTableModel::EnabledColumn) {
        paintEnabled(painter, base, index);
        return;
    }
    if (index.column() == NotificationTableModel::ColorColumn) {
        paintColor(painter, base, index);
    }
}

bool NotificationStateDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &, const QModelIndex &index)
{
    if (!model || index.column() != NotificationTableModel::EnabledColumn) {
        return false;
    }

    const auto current = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    if (event->type() == QEvent::MouseButtonRelease) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) {
            return false;
        }
        return model->setData(index, current ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
    }

    if (event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Space || key->key() == Qt::Key_Select) {
            return model->setData(index, current ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
        }
    }
    return false;
}

void NotificationStateDelegate::paintEnabled(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionButton checkbox;
    checkbox.state = QStyle::State_Enabled;
    checkbox.state |= index.data(Qt::CheckStateRole).toInt() == Qt::Checked ? QStyle::State_On : QStyle::State_Off;

    const auto *style = QApplication::style();
    const auto indicatorSize = QSize(
        style->pixelMetric(QStyle::PM_IndicatorWidth),
        style->pixelMetric(QStyle::PM_IndicatorHeight));
    checkbox.rect = centeredRect(option.rect, indicatorSize);
    style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &checkbox, painter);
}

void NotificationStateDelegate::paintColor(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QColor color = index.data(Qt::DecorationRole).value<QColor>();
    if (!color.isValid()) {
        return;
    }

    const auto swatch = centeredRect(option.rect, QSize(16, 16));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(swatch.adjusted(1, 1, -1, -1), color);
    painter->setPen(QPen(QColor(QStringLiteral("#808080")), 1));
    painter->drawRect(swatch.adjusted(0, 0, -1, -1));
    painter->restore();
}

} // namespace smartalarm
