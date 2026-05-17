#include "ui/color_palette_widget.h"

#include "core/notification.h"

#include <QPainter>

namespace smartalarm {

namespace {
QIcon swatchIcon(const QString &color, bool selected)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.fillRect(selected ? pixmap.rect().adjusted(1, 1, -1, -1) : pixmap.rect(), QColor(color));
    return QIcon(pixmap);
}
}

ColorPaletteWidget::ColorPaletteWidget(QWidget *parent)
    : QWidget(parent)
    , m_colors(defaultColors())
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
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
        button->setIcon(swatchIcon(color, selected));
        button->setIconSize(QSize(16, 16));
        button->setStyleSheet(selected
            ? QStringLiteral("QPushButton { background: #FFFFFF; border: 2px solid #000000; padding: 2px; }")
            : QStringLiteral("QPushButton { background: %1; border: 1px solid #808080; padding: 2px; }").arg(color));
        connect(button, &QPushButton::clicked, this, [this, color] {
            m_color = color;
            rebuild();
            emit changed();
        });
        if (i > 0) {
            m_layout->addStretch(1);
        }
        m_layout->addWidget(button);
    }
}

} // namespace smartalarm
