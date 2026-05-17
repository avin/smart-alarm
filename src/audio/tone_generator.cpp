#include "audio/tone_generator.h"

#include <QtMath>

namespace smartalarm::audio {

namespace {

double sampleFor(Waveform waveform, double phase)
{
    switch (waveform) {
    case Waveform::Sine: return std::sin(phase);
    case Waveform::Square: return std::sin(phase) >= 0.0 ? 1.0 : -1.0;
    case Waveform::Triangle: return (2.0 / M_PI) * std::asin(std::sin(phase));
    case Waveform::Sawtooth: return 2.0 * (phase / (2.0 * M_PI) - std::floor(0.5 + phase / (2.0 * M_PI)));
    }
    return 0.0;
}

} // namespace

double ToneGenerator::fadeEnvelope(int sampleIndex, int sampleCount, int fadeSamples)
{
    if (fadeSamples <= 0 || sampleCount <= 1) {
        return 1.0;
    }
    if (sampleIndex < fadeSamples) {
        const double t = static_cast<double>(sampleIndex) / fadeSamples;
        return 0.5 - 0.5 * std::cos(M_PI * t);
    }
    const int fadeOutStart = sampleCount - fadeSamples;
    if (sampleIndex >= fadeOutStart) {
        const double t = static_cast<double>(sampleCount - 1 - sampleIndex) / fadeSamples;
        return 0.5 - 0.5 * std::cos(M_PI * t);
    }
    return 1.0;
}

QAudioFormat ToneGenerator::defaultFormat()
{
    QAudioFormat format;
    format.setSampleRate(SampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

QByteArray ToneGenerator::generatePcm(const QVector<SoundSegment> &segments, int volume, const QAudioFormat &format)
{
    QByteArray pcm;
    if (!format.isValid() || format.channelCount() <= 0 || format.sampleRate() <= 0) {
        return pcm;
    }

    const double volumeScale = std::clamp(volume, 0, 100) / 100.0;
    const int bytesPerSample = format.bytesPerSample();
    if (bytesPerSample <= 0) {
        return pcm;
    }

    for (const auto &segment : segments) {
        const int sampleCount = qMax(1, format.sampleRate() * segment.durationMs / 1000);
        const int fadeSamples = qMin(sampleCount / 2, format.sampleRate() * NoteFadeMs / 1000);
        const int oldSize = pcm.size();
        pcm.resize(oldSize + sampleCount * format.channelCount() * bytesPerSample);
        char *out = pcm.data() + oldSize;
        for (int i = 0; i < sampleCount; ++i) {
            double value = 0.0;
            if (!segment.pause) {
                const double phase = 2.0 * M_PI * segment.frequency * i / format.sampleRate();
                value = sampleFor(segment.waveform, phase) * fadeEnvelope(i, sampleCount, fadeSamples);
            }
            const double scaled = std::clamp(value * volumeScale, -1.0, 1.0);
            for (int channel = 0; channel < format.channelCount(); ++channel) {
                switch (format.sampleFormat()) {
                case QAudioFormat::UInt8: {
                    const auto sample = static_cast<quint8>((scaled + 1.0) * 127.5);
                    memcpy(out, &sample, sizeof(sample));
                    break;
                }
                case QAudioFormat::Int16: {
                    const auto sample = static_cast<qint16>(scaled * 28000);
                    memcpy(out, &sample, sizeof(sample));
                    break;
                }
                case QAudioFormat::Int32: {
                    const auto sample = static_cast<qint32>(scaled * 2000000000.0);
                    memcpy(out, &sample, sizeof(sample));
                    break;
                }
                case QAudioFormat::Float: {
                    const auto sample = static_cast<float>(scaled);
                    memcpy(out, &sample, sizeof(sample));
                    break;
                }
                default:
                    break;
                }
                out += bytesPerSample;
            }
        }
    }
    return pcm;
}

QByteArray ToneGenerator::generateSilence(int durationMs, const QAudioFormat &format)
{
    if (!format.isValid() || format.channelCount() <= 0 || format.sampleRate() <= 0 || format.bytesPerSample() <= 0 || durationMs <= 0) {
        return {};
    }
    const int sampleCount = qMax(1, format.sampleRate() * durationMs / 1000);
    QByteArray silence(sampleCount * format.channelCount() * format.bytesPerSample(), Qt::Uninitialized);
    if (format.sampleFormat() == QAudioFormat::UInt8) {
        memset(silence.data(), 128, static_cast<size_t>(silence.size()));
    } else {
        silence.fill('\0');
    }
    return silence;
}

} // namespace smartalarm::audio
