#include "ui/notification_popup.h"

#include "ui/app_icon.h"
#include "ui/slide_to_dismiss.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace smartalarm {

namespace {
constexpr int PopupWidth = 380;
constexpr int ContentMargin = 20;
constexpr int BorderWidth = 8;
}

NotificationPopup::NotificationPopup(Notification notification, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_notification(std::move(notification))
    , m_container(new QWidget(this))
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowIcon(appicon::alarm());
    setFixedWidth(PopupWidth);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_container);

    auto *layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(ContentMargin, ContentMargin, ContentMargin, ContentMargin);
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

    const QColor borderColor(m_notification.color);
    m_borderAnimation.setDuration(500);
    m_borderAnimation.setLoopCount(-1);
    m_borderAnimation.setKeyValueAt(0.0, borderColor);
    m_borderAnimation.setKeyValueAt(0.5, QColor(Qt::white));
    m_borderAnimation.setKeyValueAt(1.0, borderColor);
    connect(&m_borderAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        updateBorder(value.value<QColor>());
    });
    updateBorder(borderColor);
    m_borderAnimation.start();
}

QUuid NotificationPopup::notificationId() const
{
    return m_notification.id;
}

void NotificationPopup::updateBorder(const QColor &borderColor)
{
    m_container->setStyleSheet(QStringLiteral("QWidget { background: #ffffff; border: %1px solid %2; } QLabel { border: none; } QPushButton { border: 1px solid #8a8a8a; padding: 3px 10px; }")
        .arg(BorderWidth)
        .arg(borderColor.name(QColor::HexRgb)));
}

} // namespace smartalarm
