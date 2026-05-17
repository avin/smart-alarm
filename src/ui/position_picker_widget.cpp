#include "ui/position_picker_widget.h"

#include <QGridLayout>

namespace smartalarm {

PositionPickerWidget::PositionPickerWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    const QVector<std::pair<NotificationPosition, QPoint>> positions = {
        { NotificationPosition::TopLeft, { 0, 0 } },
        { NotificationPosition::TopRight, { 0, 2 } },
        { NotificationPosition::Center, { 1, 1 } },
        { NotificationPosition::BottomLeft, { 2, 0 } },
        { NotificationPosition::BottomRight, { 2, 2 } },
    };
    for (const auto &[position, point] : positions) {
        auto *button = new QPushButton(this);
        button->setFixedSize(54, 36);
        button->setCheckable(true);
        button->setToolTip(notificationPositionToString(position));
        layout->addWidget(button, point.x(), point.y());
        m_buttons.insert(position, button);
        connect(button, &QPushButton::clicked, this, [this, position] {
            m_position = position;
            updateButtons();
            emit changed();
        });
    }
    updateButtons();
}

NotificationPosition PositionPickerWidget::position() const
{
    return m_position;
}

void PositionPickerWidget::setPosition(NotificationPosition position)
{
    m_position = position;
    updateButtons();
}

void PositionPickerWidget::updateButtons()
{
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        it.value()->setChecked(it.key() == m_position);
        it.value()->setStyleSheet(it.key() == m_position
            ? QStringLiteral("QPushButton { background: #0078d7; border: 1px solid #004f8f; }")
            : QStringLiteral("QPushButton { background: #ffffff; border: 1px solid #808080; }"));
    }
}

} // namespace smartalarm
