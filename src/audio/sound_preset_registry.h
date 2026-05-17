#pragma once

#include "core/notification.h"

namespace smartalarm::audio {

struct SoundPresetInfo {
    SoundPreset preset;
    QString displayName;
    QString pattern;
};

class SoundPresetRegistry {
public:
    static QVector<SoundPresetInfo> presets();
    static QString patternFor(SoundPreset preset);
};

} // namespace smartalarm::audio
