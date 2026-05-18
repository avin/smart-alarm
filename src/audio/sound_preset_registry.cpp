#include "audio/sound_preset_registry.h"

namespace smartalarm::audio {

QVector<SoundPresetInfo> SoundPresetRegistry::presets()
{
    return {
        { SoundPreset::ClassicBeep, soundPresetDisplayName(SoundPreset::ClassicBeep), QStringLiteral("880/250/sine") },
        { SoundPreset::DoubleBeep, soundPresetDisplayName(SoundPreset::DoubleBeep), QStringLiteral("880/140/sine, _/90, 880/140/sine") },
        { SoundPreset::DigitalAlert, soundPresetDisplayName(SoundPreset::DigitalAlert), QStringLiteral("950/90/square, _/50, 1200/90/square, _/50, 950/120/square") },
        { SoundPreset::GentleChime, soundPresetDisplayName(SoundPreset::GentleChime), QStringLiteral("C5/220/sine, E5/220/sine, G5/360/sine") },
        { SoundPreset::Urgent, soundPresetDisplayName(SoundPreset::Urgent), QStringLiteral("1200/120/square, _/60, 1200/120/square, _/60, 850/220/square") },
        { SoundPreset::SoftPulse, soundPresetDisplayName(SoundPreset::SoftPulse), QStringLiteral("440/180/sine, _/120, 554/180/sine") },
        { SoundPreset::HighLowAlert,
            soundPresetDisplayName(SoundPreset::HighLowAlert),
            QStringLiteral("880/150/sine, _/50/sine, 660/150/sine, _/150/sine, 880/150/sine, _/50/sine, 660/150/sine") },
        { SoundPreset::TriplePulse,
            soundPresetDisplayName(SoundPreset::TriplePulse),
            QStringLiteral("600/80/sine, _/120/sine, 600/80/sine, _/120/sine, 600/80/sine, _/320/sine, 800/80/sine, _/120/sine, 800/80/sine") },
    };
}

QString SoundPresetRegistry::patternFor(SoundPreset preset)
{
    for (const auto &item : presets()) {
        if (item.preset == preset) {
            return item.pattern;
        }
    }
    return QStringLiteral("C5/220/sine, E5/220/sine, G5/360/sine");
}

} // namespace smartalarm::audio
