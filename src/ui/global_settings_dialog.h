#pragma once

#include "core/notification.h"

#include <QDialog>
#include <QSpinBox>

namespace smartalarm {

class PositionPickerWidget;

class GlobalSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit GlobalSettingsDialog(const GlobalSettings &settings, QWidget *parent = nullptr);

    GlobalSettings settings() const;

private:
    QSpinBox *m_snooze = nullptr;
    PositionPickerWidget *m_position = nullptr;
};

} // namespace smartalarm
