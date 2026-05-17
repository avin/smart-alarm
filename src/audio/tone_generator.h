#pragma once

#include "audio/sound_pattern_parser.h"

#include <QAudioFormat>
#include <QByteArray>

namespace smartalarm::audio {

class ToneGenerator {
public:
    static constexpr int SampleRate = 44100;
    static constexpr int NoteFadeMs = 25;

    static QAudioFormat defaultFormat();
    static QByteArray generatePcm(const QVector<SoundSegment> &segments, int volume, const QAudioFormat &format);
    static QByteArray generateSilence(int durationMs, const QAudioFormat &format);
    static double fadeEnvelope(int sampleIndex, int sampleCount, int fadeSamples);
};

} // namespace smartalarm::audio
