#include "ui/notification_popup.h"

#include "ui/slide_to_dismiss.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace smartalarm {

NotificationPopup::NotificationPopup(Notification notification, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_notification(std::move(notification))
    , m_container(new QWidget(this))
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(360);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_container);

    auto *layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    auto *message = new QLabel(m_notification.message, m_container);
    message->setWordWrap(true);
    message->setFixedHeight(72);
    message->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *slide = new SlideToDismiss(m_container);
    auto *snooze = new QPushButton(QStringLiteral("Snooze"), m_container);
    layout->addWidget(message);
    layout->addWidget(slide);
    layout->addWidget(snooze, 0, Qt::AlignRight);
    connect(slide, &SlideToDismiss::dismissed, this, [this] { emit dismissed(m_notification.id); });
    connect(snooze, &QPushButton::clicked, this, [this] { emit snoozed(m_notification.id); });
    connect(&m_blinkTimer, &QTimer::timeout, this, [this] {
        m_blink = !m_blink;
        updateBorder();
    });
    m_blinkTimer.start(250);
    updateBorder();
}

QUuid NotificationPopup::notificationId() const
{
    return m_notification.id;
}

void NotificationPopup::updateBorder()
{
    const QColor color(m_notification.color);
    const auto border = m_blink ? color.name(QColor::HexRgb) : QStringLiteral("#ffffff");
    m_container->setStyleSheet(QStringLiteral("QWidget { background: #ffffff; border: 4px solid %1; } QLabel { border: none; } QPushButton { border: 1px solid #8a8a8a; padding: 3px 10px; }").arg(border));
}

} // namespace smartalarm
