#include "ui/color_palette_widget.h"

#include "core/notification.h"

namespace smartalarm {

ColorPaletteWidget::ColorPaletteWidget(QWidget *parent)
    : QWidget(parent)
    , m_colors(defaultColors())
    , m_layout(new QGridLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
    rebuild();
}

QString ColorPaletteWidget::color() const
{
    return m_color;
}

void ColorPaletteWidget::setColor(QString color)
{
    m_color = canonicalHexColor(color);
    if (!m_colors.contains(m_color)) {
        m_colors << m_color;
    }
    rebuild();
}

QStringList ColorPaletteWidget::defaultColors()
{
    return {
        QStringLiteral("#D94841"), QStringLiteral("#E66A2C"), QStringLiteral("#E6A700"), QStringLiteral("#C3A635"),
        QStringLiteral("#5B9E45"), QStringLiteral("#2C9B6F"), QStringLiteral("#1C9B9B"), QStringLiteral("#1C7ED6"),
        QStringLiteral("#4C6EF5"), QStringLiteral("#7048E8"), QStringLiteral("#9C36B5"), QStringLiteral("#C2255C"),
        QStringLiteral("#D6336C"), QStringLiteral("#868E96"), QStringLiteral("#495057"), QStringLiteral("#212529"),
        QStringLiteral("#F03E3E"), QStringLiteral("#F76707"), QStringLiteral("#37B24D"), QStringLiteral("#1098AD")
    };
}

void ColorPaletteWidget::rebuild()
{
    while (auto *item = m_layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (int i = 0; i < m_colors.size(); ++i) {
        const auto color = m_colors.at(i);
        auto *button = new QPushButton(this);
        button->setFixedSize(24, 24);
        button->setToolTip(color);
        const bool selected = color.compare(m_color, Qt::CaseInsensitive) == 0;
        button->setStyleSheet(QStringLiteral("QPushButton { background: %1; border: %2px solid %3; padding: 2px; }")
            .arg(color)
            .arg(selected ? 3 : 1)
            .arg(selected ? QStringLiteral("#000000") : QStringLiteral("#808080")));
        connect(button, &QPushButton::clicked, this, [this, color] {
            m_color = color;
            rebuild();
            emit changed();
        });
        m_layout->addWidget(button, i / 10, i % 10);
    }
}

} // namespace smartalarm
