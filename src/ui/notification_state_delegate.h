#pragma once

#include <QStyledItemDelegate>

namespace smartalarm {

class NotificationStateDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit NotificationStateDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:
    void paintEnabled(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintColor(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace smartalarm
