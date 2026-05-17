#pragma once

#include "app/app_controller.h"

#include <QAbstractTableModel>

namespace smartalarm {

class NotificationTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        EnabledColumn,
        ColorColumn,
        MessageColumn,
        ScheduleColumn,
        ActionsColumn,
        ColumnCount
    };

    explicit NotificationTableModel(AppController *controller, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    std::optional<Notification> notificationAt(int row) const;

private:
    AppController *m_controller = nullptr;
};

} // namespace smartalarm
