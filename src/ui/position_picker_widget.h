#pragma once

#include "core/notification.h"

#include <QPushButton>
#include <QWidget>

namespace smartalarm {

class PositionPickerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PositionPickerWidget(QWidget *parent = nullptr);

    NotificationPosition position() const;
    void setPosition(NotificationPosition position);

signals:
    void changed();

private:
    void updateButtons();
    NotificationPosition m_position = NotificationPosition::TopRight;
    QHash<NotificationPosition, QPushButton *> m_buttons;
};

} // namespace smartalarm
