#pragma once

#include <QPersistentModelIndex>
#include <QStyledItemDelegate>

namespace smartalarm {

class NotificationActionsDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit NotificationActionsDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

signals:
    void editRequested(int row);
    void deleteRequested(int row);

private:
    enum class ActionButton {
        None,
        Edit,
        Delete,
    };

    void drawButton(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, ActionButton button) const;
    ActionButton buttonAt(const QStyleOptionViewItem &option, const QPoint &position) const;
    QRect buttonRect(const QStyleOptionViewItem &option, ActionButton button) const;
    QRect editRect(const QStyleOptionViewItem &option) const;
    QRect deleteRect(const QStyleOptionViewItem &option) const;

    mutable QPersistentModelIndex m_pressedIndex;
    mutable ActionButton m_pressedButton = ActionButton::None;
};

} // namespace smartalarm
