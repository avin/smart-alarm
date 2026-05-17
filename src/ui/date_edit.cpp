#include "ui/date_edit.h"

#include <QCalendarWidget>
#include <QDialog>
#include <QEvent>
#include <QHBoxLayout>
#include <QStyle>
#include <QVBoxLayout>

namespace smartalarm {

DateEdit::DateEdit(QWidget *parent)
    : QWidget(parent)
    , m_edit(new QLineEdit(this))
    , m_clearButton(new QToolButton(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_edit->setReadOnly(true);
    m_clearButton->setParent(m_edit);
    m_clearButton->setText(QStringLiteral("X"));
    m_clearButton->setToolTip(QStringLiteral("Clear"));
    m_clearButton->setAutoRaise(true);
    m_clearButton->setCursor(Qt::ArrowCursor);
    layout->addWidget(m_edit);
    m_edit->setTextMargins(0, 0, 22, 0);
    m_edit->installEventFilter(this);
    connect(m_clearButton, &QToolButton::clicked, this, [this] {
        setDate(std::nullopt);
        emit changed();
    });
}

std::optional<QDate> DateEdit::date() const
{
    return m_date;
}

void DateEdit::setDate(std::optional<QDate> date)
{
    m_date = date;
    m_edit->setText(m_date ? m_date->toString(QStringLiteral("yyyy-MM-dd")) : QString());
}

void DateEdit::setInvalid(bool invalid)
{
    m_edit->setStyleSheet(invalid ? QStringLiteral("QLineEdit { border: 1px solid #D94841; }") : QString());
}

bool DateEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::MouseButtonPress) {
        openCalendar();
        return true;
    }
    if (watched == m_edit && event->type() == QEvent::Resize) {
        updateClearButtonGeometry();
    }
    return QWidget::eventFilter(watched, event);
}

void DateEdit::openCalendar()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Select date"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *calendar = new QCalendarWidget(&dialog);
    calendar->setGridVisible(true);
    calendar->setSelectedDate(m_date.value_or(QDate::currentDate()));
    layout->addWidget(calendar);
    connect(calendar, &QCalendarWidget::activated, &dialog, &QDialog::accept);
    if (dialog.exec() == QDialog::Accepted) {
        setDate(calendar->selectedDate());
        emit changed();
    }
}

void DateEdit::updateClearButtonGeometry()
{
    const int frameWidth = m_edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, m_edit);
    const int size = qMax(16, m_edit->height() - frameWidth * 2 - 2);
    m_clearButton->setFixedSize(size, size);
    m_clearButton->move(m_edit->rect().right() - frameWidth - size, (m_edit->height() - size) / 2);
}

} // namespace smartalarm
