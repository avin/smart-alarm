#include "ui/time_edit.h"

#include "core/time_input_normalizer.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QStyle>

namespace smartalarm {

TimeEdit::TimeEdit(QWidget *parent)
    : QWidget(parent)
    , m_edit(new QLineEdit(this))
    , m_clearButton(new QToolButton(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_edit->setPlaceholderText(QStringLiteral("HH:mm"));
    m_edit->setMaxLength(5);
    m_clearButton->setParent(m_edit);
    m_clearButton->setText(QStringLiteral("X"));
    m_clearButton->setToolTip(QStringLiteral("Clear"));
    m_clearButton->setAutoRaise(true);
    m_clearButton->setCursor(Qt::ArrowCursor);
    layout->addWidget(m_edit);
    m_edit->setTextMargins(0, 0, 22, 0);
    m_edit->installEventFilter(this);
    connect(m_edit, &QLineEdit::textEdited, this, [this](const QString &text) {
        const auto filtered = TimeInputNormalizer::filteredText(text);
        if (filtered != text) {
            const auto pos = m_edit->cursorPosition();
            m_edit->setText(filtered);
            m_edit->setCursorPosition(qMin(pos, filtered.size()));
        }
        emit changed();
    });
    connect(m_clearButton, &QToolButton::clicked, this, [this] {
        m_edit->clear();
        emit changed();
    });
}

std::optional<QTime> TimeEdit::time() const
{
    return TimeInputNormalizer::normalize(m_edit->text());
}

void TimeEdit::setTime(std::optional<QTime> time)
{
    m_edit->setText(TimeInputNormalizer::format(time));
}

void TimeEdit::setInvalid(bool invalid)
{
    m_edit->setStyleSheet(invalid ? QStringLiteral("QLineEdit { border: 1px solid #D94841; }") : QString());
}

bool TimeEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::FocusOut) {
        normalizeText();
    } else if (watched == m_edit && event->type() == QEvent::Resize) {
        updateClearButtonGeometry();
    }
    return QWidget::eventFilter(watched, event);
}

void TimeEdit::normalizeText()
{
    m_edit->setText(TimeInputNormalizer::format(TimeInputNormalizer::normalize(m_edit->text())));
}

void TimeEdit::updateClearButtonGeometry()
{
    const int frameWidth = m_edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, m_edit);
    const int size = qMax(16, m_edit->height() - frameWidth * 2 - 2);
    m_clearButton->setFixedSize(size, size);
    m_clearButton->move(m_edit->rect().right() - frameWidth - size, (m_edit->height() - size) / 2);
}

} // namespace smartalarm
