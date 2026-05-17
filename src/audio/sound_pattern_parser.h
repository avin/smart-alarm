#pragma once

#include "core/notification.h"

namespace smartalarm::audio {

struct SoundSegment {
    bool pause = false;
    double frequency = 0.0;
    int durationMs = 0;
    Waveform waveform = Waveform::Sine;
};

struct ParseResult {
    bool ok = false;
    QVector<SoundSegment> segments;
    QString errorMessage;
};

class SoundPatternParser {
public:
    static ParseResult parse(const QString &pattern);
};

} // namespace smartalarm::audio
