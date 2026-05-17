#include "ui/global_settings_dialog.h"

#include "ui/position_picker_widget.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace smartalarm {

namespace {
QWidget *withVerticalLabel(const QString &labelText, QWidget *field, QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *label = new QLabel(labelText, container);
    auto font = label->font();
    if (font.pointSize() > 1) {
        font.setPointSize(font.pointSize() - 1);
    }
    label->setFont(font);

    layout->addWidget(label);
    layout->addWidget(field);
    return container;
}
}

GlobalSettingsDialog::GlobalSettingsDialog(const GlobalSettings &settings, QWidget *parent)
    : QDialog(parent)
    , m_snooze(new QSpinBox(this))
    , m_position(new PositionPickerWidget(this))
{
    setWindowTitle(QStringLiteral("Settings"));
    resize(420, 320);
    setMinimumSize(360, 280);
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    m_snooze->setRange(1, 1440);
    m_snooze->setValue(settings.defaultSnoozeMinutes);
    m_position->setPosition(settings.notificationPosition);
    layout->addWidget(withVerticalLabel(QStringLiteral("Default snooze interval (minutes)"), m_snooze, this));
    layout->addWidget(withVerticalLabel(QStringLiteral("Notification position"), m_position, this));
    layout->addStretch(1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

GlobalSettings GlobalSettingsDialog::settings() const
{
    GlobalSettings settings;
    settings.defaultSnoozeMinutes = m_snooze->value();
    settings.notificationPosition = m_position->position();
    return settings;
}

} // namespace smartalarm
