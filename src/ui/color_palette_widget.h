#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QWidget>

namespace smartalarm {

class ColorPaletteWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColorPaletteWidget(QWidget *parent = nullptr);

    QString color() const;
    void setColor(QString color);
    static QStringList defaultColors();

signals:
    void changed();

private:
    void rebuild();
    QString m_color = QStringLiteral("#D94841");
    QStringList m_colors;
    QGridLayout *m_layout = nullptr;
};

} // namespace smartalarm
