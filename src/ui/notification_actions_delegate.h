#pragma once

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
    QRect editRect(const QStyleOptionViewItem &option) const;
    QRect deleteRect(const QStyleOptionViewItem &option) const;
};

} // namespace smartalarm
