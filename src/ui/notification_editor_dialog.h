#pragma once

#include "audio/audio_player.h"
#include "core/notification.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolButton>

namespace smartalarm {

class ColorPaletteWidget;
class DateEdit;
class DayOfWeekSelector;
class TimeEdit;

class NotificationEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit NotificationEditorDialog(Notification notification, audio::PreviewPlayer *previewPlayer, bool previewEnabled, QWidget *parent = nullptr);

    Notification notification() const;

protected:
    void accept() override;

private:
    QWidget *createOnceTab();
    QWidget *createWeeklyTab();
    QWidget *createNthWeekTab();
    QWidget *createIntervalTab();
    void loadFromNotification(const Notification &notification);
    Notification buildNotification() const;
    bool validateAndMark();
    QString currentPattern() const;
    void updateCustomSoundVisibility();
    void playPreview();

    Notification m_original;
    audio::PreviewPlayer *m_previewPlayer = nullptr;
    bool m_previewEnabled = true;

    QLineEdit *m_message = nullptr;
    QCheckBox *m_enabled = nullptr;
    QTabWidget *m_tabs = nullptr;

    DateEdit *m_onceDate = nullptr;
    TimeEdit *m_onceTime = nullptr;

    DayOfWeekSelector *m_weeklyDays = nullptr;
    TimeEdit *m_weeklyTime = nullptr;
    QToolButton *m_weeklyRangeToggle = nullptr;
    QWidget *m_weeklyRangeContent = nullptr;
    DateEdit *m_weeklyStart = nullptr;
    DateEdit *m_weeklyEnd = nullptr;

    QSpinBox *m_nthEvery = nullptr;
    QComboBox *m_nthWeekday = nullptr;
    TimeEdit *m_nthTime = nullptr;
    DateEdit *m_nthReference = nullptr;
    DateEdit *m_nthEnd = nullptr;

    QSpinBox *m_intervalEvery = nullptr;
    TimeEdit *m_intervalFrom = nullptr;
    TimeEdit *m_intervalTo = nullptr;
    QRadioButton *m_countFromTrigger = nullptr;
    QRadioButton *m_countFromConfirmation = nullptr;
    QSpinBox *m_intervalSnooze = nullptr;
    QToolButton *m_intervalLimitToggle = nullptr;
    QWidget *m_intervalLimitContent = nullptr;
    DayOfWeekSelector *m_intervalDays = nullptr;
    DateEdit *m_intervalStart = nullptr;
    DateEdit *m_intervalEnd = nullptr;

    ColorPaletteWidget *m_color = nullptr;
    QComboBox *m_sound = nullptr;
    QWidget *m_customPatternContainer = nullptr;
    QLineEdit *m_customPattern = nullptr;
    QPushButton *m_help = nullptr;
    QSlider *m_volume = nullptr;
    QSpinBox *m_playCount = nullptr;
};

} // namespace smartalarm
