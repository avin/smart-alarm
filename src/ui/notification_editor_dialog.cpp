#include "ui/notification_editor_dialog.h"

#include "audio/sound_pattern_parser.h"
#include "audio/sound_preset_registry.h"
#include "audio/tone_generator.h"
#include "core/notification_validator.h"
#include "ui/color_palette_widget.h"
#include "ui/date_edit.h"
#include "ui/day_of_week_selector.h"
#include "ui/time_edit.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

namespace smartalarm {

namespace {
constexpr int CustomSoundIndex = 6;

QLabel *createFieldLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    auto font = label->font();
    if (font.pointSize() > 1) {
        font.setPointSize(font.pointSize() - 1);
    }
    label->setFont(font);
    return label;
}

QWidget *withVerticalLabel(const QString &labelText, QWidget *field, QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(createFieldLabel(labelText, container));
    layout->addWidget(field);
    return container;
}

void addLabeledField(QVBoxLayout *layout, const QString &labelText, QWidget *field, QWidget *parent)
{
    layout->addWidget(withVerticalLabel(labelText, field, parent));
}

QWidget *createCollapsibleSection(const QString &title, QWidget *content, QToolButton *&toggle, QWidget *parent)
{
    auto *container = new QWidget(parent);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    toggle = new QToolButton(container);
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setAutoRaise(true);
    content->setVisible(false);

    QObject::connect(toggle, &QToolButton::toggled, content, [toggle, content](bool expanded) {
        content->setVisible(expanded);
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    });

    layout->addWidget(toggle);
    layout->addWidget(content);
    return container;
}

void setCollapsibleExpanded(QToolButton *toggle, QWidget *content, bool expanded)
{
    if (!toggle || !content) {
        return;
    }
    toggle->setChecked(expanded);
    content->setVisible(expanded);
    toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
}
}

NotificationEditorDialog::NotificationEditorDialog(Notification notification, audio::PreviewPlayer *previewPlayer, bool previewEnabled, QWidget *parent)
    : QDialog(parent)
    , m_original(std::move(notification))
    , m_previewPlayer(previewPlayer)
    , m_previewEnabled(previewEnabled)
{
    setWindowTitle(QStringLiteral("Notification"));
    resize(640, 620);
    setMinimumSize(560, 520);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    auto *top = new QHBoxLayout;
    m_message = new QLineEdit(this);
    m_enabled = new QCheckBox(QStringLiteral("Enabled"), this);
    top->addWidget(withVerticalLabel(QStringLiteral("Message"), m_message, this), 1);
    top->addWidget(m_enabled);
    mainLayout->addLayout(top);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createOnceTab(), QStringLiteral("Once"));
    m_tabs->addTab(createWeeklyTab(), QStringLiteral("Weekly"));
    m_tabs->addTab(createNthWeekTab(), QStringLiteral("Nth Week"));
    m_tabs->addTab(createIntervalTab(), QStringLiteral("Interval"));
    mainLayout->addWidget(m_tabs, 1);

    auto *common = new QVBoxLayout;
    common->setContentsMargins(0, 0, 0, 0);
    common->setSpacing(6);
    m_color = new ColorPaletteWidget(this);
    addLabeledField(common, QStringLiteral("Color"), m_color, this);

    auto *soundRowWidget = new QWidget(this);
    auto *soundRow = new QHBoxLayout(soundRowWidget);
    soundRow->setContentsMargins(0, 0, 0, 0);
    soundRow->setSpacing(2);
    m_sound = new QComboBox(this);
    for (const auto &preset : audio::SoundPresetRegistry::presets()) {
        m_sound->addItem(preset.displayName, static_cast<int>(preset.preset));
    }
    m_sound->addItem(QStringLiteral("Custom pattern..."));
    auto *play = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QString(), this);
    play->setToolTip(QStringLiteral("Play"));
    play->setEnabled(previewEnabled);
    soundRow->addWidget(m_sound, 1);
    soundRow->addWidget(play);
    addLabeledField(common, QStringLiteral("Sound"), soundRowWidget, this);

    auto *customRowWidget = new QWidget(this);
    auto *customRow = new QHBoxLayout(customRowWidget);
    customRow->setContentsMargins(0, 0, 0, 0);
    customRow->setSpacing(2);
    m_customPattern = new QLineEdit(this);
    m_help = new QPushButton(QStringLiteral("?"), this);
    customRow->addWidget(m_customPattern, 1);
    customRow->addWidget(m_help);
    m_customPatternContainer = withVerticalLabel(QStringLiteral("Custom pattern"), customRowWidget, this);
    common->addWidget(m_customPatternContainer);

    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    addLabeledField(common, QStringLiteral("Volume"), m_volume, this);
    m_playCount = new QSpinBox(this);
    m_playCount->setRange(0, 999);
    addLabeledField(common, QStringLiteral("Play count (0 = until dismissed)"), m_playCount, this);
    mainLayout->addLayout(common);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &NotificationEditorDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_sound, qOverload<int>(&QComboBox::currentIndexChanged), this, &NotificationEditorDialog::updateCustomSoundVisibility);
    connect(play, &QPushButton::clicked, this, &NotificationEditorDialog::playPreview);
    connect(m_help, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, QStringLiteral("Custom pattern"),
            QStringLiteral("Use frequency/duration, note/duration or _/duration. Optional waveform: sine, square, triangle, sawtooth.\nExamples:\n880/150, _/50, 660/150\nC5/200, E5/200, G5/300"));
    });

    loadFromNotification(m_original);
}

QWidget *NotificationEditorDialog::createOnceTab()
{
    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    m_onceDate = new DateEdit(widget);
    m_onceTime = new TimeEdit(widget);
    addLabeledField(layout, QStringLiteral("Date"), m_onceDate, widget);
    addLabeledField(layout, QStringLiteral("Time"), m_onceTime, widget);
    layout->addStretch(1);
    return widget;
}

QWidget *NotificationEditorDialog::createWeeklyTab()
{
    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    m_weeklyDays = new DayOfWeekSelector(widget);
    m_weeklyTime = new TimeEdit(widget);
    m_weeklyRangeContent = new QWidget(widget);
    auto *rangeLayout = new QVBoxLayout(m_weeklyRangeContent);
    rangeLayout->setContentsMargins(16, 0, 0, 0);
    rangeLayout->setSpacing(6);
    m_weeklyStart = new DateEdit(m_weeklyRangeContent);
    m_weeklyEnd = new DateEdit(m_weeklyRangeContent);
    addLabeledField(rangeLayout, QStringLiteral("Start date"), m_weeklyStart, m_weeklyRangeContent);
    addLabeledField(rangeLayout, QStringLiteral("End date"), m_weeklyEnd, m_weeklyRangeContent);
    addLabeledField(layout, QStringLiteral("Days"), m_weeklyDays, widget);
    addLabeledField(layout, QStringLiteral("Time"), m_weeklyTime, widget);
    layout->addWidget(createCollapsibleSection(QStringLiteral("Date range (optional)"), m_weeklyRangeContent, m_weeklyRangeToggle, widget));
    layout->addStretch(1);
    return widget;
}

QWidget *NotificationEditorDialog::createNthWeekTab()
{
    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    m_nthEvery = new QSpinBox(widget);
    m_nthEvery->setRange(1, 999);
    m_nthWeekday = new QComboBox(widget);
    for (const auto day : allWeekdays()) {
        m_nthWeekday->addItem(weekdayDisplayName(day), static_cast<int>(day));
    }
    m_nthTime = new TimeEdit(widget);
    m_nthReference = new DateEdit(widget);
    m_nthEnd = new DateEdit(widget);
    addLabeledField(layout, QStringLiteral("Every"), m_nthEvery, widget);
    addLabeledField(layout, QStringLiteral("Weeks on"), m_nthWeekday, widget);
    addLabeledField(layout, QStringLiteral("Time"), m_nthTime, widget);
    addLabeledField(layout, QStringLiteral("Reference date"), m_nthReference, widget);
    addLabeledField(layout, QStringLiteral("End date (optional)"), m_nthEnd, widget);
    layout->addStretch(1);
    return widget;
}

QWidget *NotificationEditorDialog::createIntervalTab()
{
    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    m_intervalEvery = new QSpinBox(widget);
    m_intervalEvery->setRange(1, 1440);
    m_intervalFrom = new TimeEdit(widget);
    m_intervalTo = new TimeEdit(widget);
    m_countFromTrigger = new QRadioButton(QStringLiteral("From trigger"), widget);
    m_countFromConfirmation = new QRadioButton(QStringLiteral("From confirmation"), widget);
    auto *countRowWidget = new QWidget(widget);
    auto *countRow = new QHBoxLayout(countRowWidget);
    countRow->setContentsMargins(0, 0, 0, 0);
    countRow->setSpacing(6);
    countRow->addWidget(m_countFromTrigger);
    countRow->addWidget(m_countFromConfirmation);
    m_intervalSnooze = new QSpinBox(widget);
    m_intervalSnooze->setRange(0, 1440);
    m_intervalLimitContent = new QWidget(widget);
    auto *limitLayout = new QVBoxLayout(m_intervalLimitContent);
    limitLayout->setContentsMargins(16, 0, 0, 0);
    limitLayout->setSpacing(6);
    m_intervalDays = new DayOfWeekSelector(m_intervalLimitContent);
    m_intervalStart = new DateEdit(m_intervalLimitContent);
    m_intervalEnd = new DateEdit(m_intervalLimitContent);
    addLabeledField(limitLayout, QStringLiteral("Days"), m_intervalDays, m_intervalLimitContent);
    addLabeledField(limitLayout, QStringLiteral("Start date"), m_intervalStart, m_intervalLimitContent);
    addLabeledField(limitLayout, QStringLiteral("End date"), m_intervalEnd, m_intervalLimitContent);
    addLabeledField(layout, QStringLiteral("Every minutes"), m_intervalEvery, widget);
    addLabeledField(layout, QStringLiteral("From"), m_intervalFrom, widget);
    addLabeledField(layout, QStringLiteral("To"), m_intervalTo, widget);
    addLabeledField(layout, QStringLiteral("Count from"), countRowWidget, widget);
    addLabeledField(layout, QStringLiteral("Snooze (minutes, 0 = use global default)"), m_intervalSnooze, widget);
    layout->addWidget(createCollapsibleSection(QStringLiteral("Schedule (optional)"), m_intervalLimitContent, m_intervalLimitToggle, widget));
    layout->addStretch(1);
    return widget;
}

void NotificationEditorDialog::loadFromNotification(const Notification &notification)
{
    m_message->setText(notification.message);
    m_enabled->setChecked(notification.enabled);
    m_color->setColor(notification.color);
    m_volume->setValue(notification.volume);
    m_playCount->setValue(notification.playCount);

    if (const auto *preset = std::get_if<PresetSound>(&notification.sound)) {
        m_sound->setCurrentIndex(static_cast<int>(preset->preset));
    } else if (const auto *custom = std::get_if<CustomSound>(&notification.sound)) {
        m_sound->setCurrentIndex(CustomSoundIndex);
        m_customPattern->setText(custom->pattern);
    }

    std::visit([&](const auto &schedule) {
        using T = std::decay_t<decltype(schedule)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            m_tabs->setCurrentIndex(0);
            m_onceDate->setDate(schedule.date);
            m_onceTime->setTime(schedule.time);
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            m_tabs->setCurrentIndex(1);
            m_weeklyDays->setSelectedDays(schedule.days);
            m_weeklyTime->setTime(schedule.time);
            setCollapsibleExpanded(m_weeklyRangeToggle, m_weeklyRangeContent, schedule.dateRange.has_value());
            if (schedule.dateRange) {
                m_weeklyStart->setDate(schedule.dateRange->start);
                m_weeklyEnd->setDate(schedule.dateRange->end);
            }
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            m_tabs->setCurrentIndex(2);
            m_nthEvery->setValue(schedule.everyWeeks);
            m_nthWeekday->setCurrentIndex(static_cast<int>(schedule.weekday) - 1);
            m_nthTime->setTime(schedule.time);
            m_nthReference->setDate(schedule.referenceDate);
            m_nthEnd->setDate(schedule.endDate);
        } else {
            m_tabs->setCurrentIndex(3);
            m_intervalEvery->setValue(schedule.everyMinutes);
            m_intervalFrom->setTime(schedule.from);
            m_intervalTo->setTime(schedule.to);
            m_countFromTrigger->setChecked(schedule.countFrom == CountFrom::Trigger);
            m_countFromConfirmation->setChecked(schedule.countFrom == CountFrom::Confirmation);
            m_intervalSnooze->setValue(schedule.snoozeMinutes);
            setCollapsibleExpanded(m_intervalLimitToggle, m_intervalLimitContent, schedule.schedule.has_value());
            if (schedule.schedule) {
                m_intervalDays->setSelectedDays(schedule.schedule->days);
                m_intervalStart->setDate(schedule.schedule->dateRange.start);
                m_intervalEnd->setDate(schedule.schedule->dateRange.end);
            }
        }
    }, notification.schedule);
    updateCustomSoundVisibility();
}

Notification NotificationEditorDialog::notification() const
{
    return buildNotification();
}

Notification NotificationEditorDialog::buildNotification() const
{
    auto notification = m_original;
    notification.message = m_message->text().trimmed();
    notification.enabled = m_enabled->isChecked();
    notification.color = m_color->color();
    notification.volume = m_volume->value();
    notification.playCount = m_playCount->value();
    if (m_sound->currentIndex() == CustomSoundIndex) {
        notification.sound = CustomSound { m_customPattern->text().trimmed() };
    } else {
        notification.sound = PresetSound { static_cast<SoundPreset>(m_sound->currentData().toInt()) };
    }

    switch (m_tabs->currentIndex()) {
    case 0:
        notification.schedule = OnceSchedule { m_onceDate->date(), m_onceTime->time() };
        break;
    case 1: {
        WeeklySchedule schedule;
        schedule.days = m_weeklyDays->selectedDays();
        schedule.time = m_weeklyTime->time();
        if (m_weeklyStart->date() || m_weeklyEnd->date()) {
            schedule.dateRange = DateRange { m_weeklyStart->date(), m_weeklyEnd->date() };
        }
        notification.schedule = schedule;
        break;
    }
    case 2: {
        NthWeekSchedule schedule;
        schedule.everyWeeks = m_nthEvery->value();
        schedule.weekday = static_cast<Weekday>(m_nthWeekday->currentData().toInt());
        schedule.time = m_nthTime->time();
        schedule.referenceDate = m_nthReference->date();
        schedule.endDate = m_nthEnd->date();
        notification.schedule = schedule;
        break;
    }
    default: {
        IntervalSchedule schedule;
        schedule.everyMinutes = m_intervalEvery->value();
        schedule.from = m_intervalFrom->time();
        schedule.to = m_intervalTo->time();
        schedule.countFrom = m_countFromConfirmation->isChecked() ? CountFrom::Confirmation : CountFrom::Trigger;
        schedule.snoozeMinutes = m_intervalSnooze->value();
        if (!m_intervalDays->selectedDays().isEmpty() || m_intervalStart->date() || m_intervalEnd->date()) {
            schedule.schedule = IntervalScheduleLimit { m_intervalDays->selectedDays(), DateRange { m_intervalStart->date(), m_intervalEnd->date() } };
        }
        notification.schedule = schedule;
        break;
    }
    }
    return notification;
}

void NotificationEditorDialog::accept()
{
    if (!validateAndMark()) {
        return;
    }
    QDialog::accept();
}

bool NotificationEditorDialog::validateAndMark()
{
    m_message->setStyleSheet({});
    m_onceDate->setInvalid(false);
    m_onceTime->setInvalid(false);
    m_weeklyDays->setInvalid(false);
    m_weeklyTime->setInvalid(false);
    m_nthTime->setInvalid(false);
    m_nthReference->setInvalid(false);
    m_intervalFrom->setInvalid(false);
    m_intervalTo->setInvalid(false);
    m_intervalDays->setInvalid(false);
    m_customPattern->setStyleSheet({});

    const auto candidate = buildNotification();
    auto result = NotificationValidator::validate(candidate);
    if (m_sound->currentIndex() == CustomSoundIndex) {
        const auto parsed = audio::SoundPatternParser::parse(m_customPattern->text());
        if (!parsed.ok) {
            result.add(ValidationErrorCode::Invalid, QStringLiteral("sound.custom.pattern"));
        }
    }
    for (const auto &error : result.errors) {
        if (error.fieldPath == QStringLiteral("message")) m_message->setStyleSheet(QStringLiteral("QLineEdit { border: 1px solid #D94841; }"));
        else if (error.fieldPath == QStringLiteral("schedule.once.date")) m_onceDate->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.once.time")) m_onceTime->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.weekly.days")) m_weeklyDays->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.weekly.time")) m_weeklyTime->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.nthWeek.time")) m_nthTime->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.nthWeek.referenceDate")) m_nthReference->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.interval.from")) m_intervalFrom->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.interval.to")) m_intervalTo->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("schedule.interval.schedule.days")) m_intervalDays->setInvalid(true);
        else if (error.fieldPath == QStringLiteral("sound.custom.pattern")) m_customPattern->setStyleSheet(QStringLiteral("QLineEdit { border: 1px solid #D94841; }"));
    }
    if (!result.ok()) {
        QMessageBox::warning(this, QStringLiteral("Validation"), QStringLiteral("Please fix highlighted fields."));
        return false;
    }
    return true;
}

QString NotificationEditorDialog::currentPattern() const
{
    if (m_sound->currentIndex() == CustomSoundIndex) {
        return m_customPattern->text();
    }
    return audio::SoundPresetRegistry::patternFor(static_cast<SoundPreset>(m_sound->currentData().toInt()));
}

void NotificationEditorDialog::updateCustomSoundVisibility()
{
    const bool custom = m_sound->currentIndex() == CustomSoundIndex;
    m_customPatternContainer->setVisible(custom);
}

void NotificationEditorDialog::playPreview()
{
    if (!m_previewEnabled || !m_previewPlayer) {
        return;
    }
    const auto parsed = audio::SoundPatternParser::parse(currentPattern());
    if (!parsed.ok) {
        m_customPattern->setStyleSheet(QStringLiteral("QLineEdit { border: 1px solid #D94841; }"));
        return;
    }
    m_customPattern->setStyleSheet({});
    m_previewPlayer->playSegments(parsed.segments, m_volume->value());
}

} // namespace smartalarm
