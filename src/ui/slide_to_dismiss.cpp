#include "ui/slide_to_dismiss.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace smartalarm {

namespace {
constexpr int HandleWidth = 42;
}

SlideToDismiss::SlideToDismiss(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(34);
    setFocusPolicy(Qt::StrongFocus);
}

int SlideToDismiss::handleX() const
{
    return m_handleX;
}

void SlideToDismiss::setHandleX(int x)
{
    m_handleX = std::clamp(x, 0, maxHandleX());
    update();
}

void SlideToDismiss::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), QColor(QStringLiteral("#eeeeee")));
    painter.setPen(QColor(QStringLiteral("#a0a0a0")));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    painter.setPen(QColor(QStringLiteral("#555555")));
    painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Slide to dismiss"));
    const QRect handle(m_handleX, 0, HandleWidth, height());
    painter.fillRect(handle.adjusted(2, 2, -2, -2), QColor(QStringLiteral("#ffffff")));
    painter.setPen(QColor(QStringLiteral("#777777")));
    painter.drawRect(handle.adjusted(2, 2, -2, -2));
    painter.drawText(handle, Qt::AlignCenter, QStringLiteral(">"));
}

void SlideToDismiss::mousePressEvent(QMouseEvent *event)
{
    const QRect handle(m_handleX, 0, HandleWidth, height());
    if (handle.contains(event->pos())) {
        m_dragging = true;
        m_dragOffset = event->pos() - handle.topLeft();
    }
}

void SlideToDismiss::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        setHandleX(event->pos().x() - m_dragOffset.x());
    }
}

void SlideToDismiss::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_dragging) {
        return;
    }
    m_dragging = false;
    if (m_handleX >= maxHandleX() * 0.9) {
        emit dismissed();
    } else {
        animateBack();
    }
}

void SlideToDismiss::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
        emit dismissed();
        return;
    }
    QWidget::keyPressEvent(event);
}

int SlideToDismiss::maxHandleX() const
{
    return qMax(0, width() - HandleWidth);
}

void SlideToDismiss::animateBack()
{
    auto *animation = new QPropertyAnimation(this, "handleX", this);
    animation->setDuration(140);
    animation->setStartValue(m_handleX);
    animation->setEndValue(0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace smartalarm
