#include "ui/notification_table_model.h"

#include "core/schedule_evaluator.h"
#include "core/schedule_formatter.h"

#include <QBrush>
#include <QFont>

namespace smartalarm {

NotificationTableModel::NotificationTableModel(AppController *controller, QObject *parent)
    : QAbstractTableModel(parent)
    , m_controller(controller)
{
    connect(m_controller, &AppController::notificationsChanged, this, [this] {
        beginResetModel();
        endResetModel();
    });
}

int NotificationTableModel::rowCount(const QModelIndex &) const
{
    return m_controller ? m_controller->notifications().size() : 0;
}

int NotificationTableModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant NotificationTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_controller || index.row() >= m_controller->notifications().size()) {
        return {};
    }
    const auto &notification = m_controller->notifications().at(index.row());
    const bool overdue = ScheduleEvaluator::isOnceOverdue(notification, QDateTime::currentDateTime());
    if (index.column() == EnabledColumn && role == Qt::CheckStateRole) {
        return notification.enabled ? Qt::Checked : Qt::Unchecked;
    }
    if (index.column() == ColorColumn && role == Qt::DecorationRole) {
        return QColor(notification.color);
    }
    if (index.column() == MessageColumn && role == Qt::DisplayRole) {
        return notification.message;
    }
    if (index.column() == ScheduleColumn && role == Qt::DisplayRole) {
        return ScheduleFormatter::format(notification.schedule);
    }
    if (index.column() == ScheduleColumn && role == Qt::ForegroundRole && overdue) {
        return QBrush(QColor(QStringLiteral("#D94841")));
    }
    if (index.column() == ScheduleColumn && role == Qt::FontRole && overdue) {
        QFont font;
        font.setStrikeOut(true);
        return font;
    }
    if (index.column() == ActionsColumn && role == Qt::DisplayRole) {
        return QString();
    }
    return {};
}

QVariant NotificationTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case MessageColumn: return QStringLiteral("Message");
    case ScheduleColumn: return QStringLiteral("Schedule");
    case ActionsColumn: return QStringLiteral("Actions");
    default: return QString();
    }
}

Qt::ItemFlags NotificationTableModel::flags(const QModelIndex &index) const
{
    auto flags = QAbstractTableModel::flags(index);
    flags &= ~Qt::ItemIsSelectable;
    if (index.column() == EnabledColumn) {
        flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    }
    return flags;
}

bool NotificationTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != EnabledColumn || role != Qt::CheckStateRole || !m_controller) {
        return false;
    }
    const auto notification = notificationAt(index.row());
    if (!notification) {
        return false;
    }
    const bool enabled = value.toInt() == Qt::Checked;
    const auto result = m_controller->setNotificationEnabled(notification->id, enabled);
    if (result.ok) {
        emit dataChanged(index, index, { Qt::CheckStateRole });
    }
    return result.ok;
}

std::optional<Notification> NotificationTableModel::notificationAt(int row) const
{
    if (!m_controller || row < 0 || row >= m_controller->notifications().size()) {
        return std::nullopt;
    }
    return m_controller->notifications().at(row);
}

} // namespace smartalarm
