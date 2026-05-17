#include "audio/audio_player.h"

#include "audio/tone_generator.h"

#include <QAudioDevice>
#include <QMediaDevices>

namespace smartalarm::audio {

namespace {
constexpr int FeedIntervalMs = 10;
constexpr int BufferDurationMs = 180;
constexpr int RepeatGapMs = 300;
constexpr int PlaybackTailSilenceMs = 300;
}

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
    , m_format(ToneGenerator::defaultFormat())
{
    m_feedTimer.setInterval(FeedIntervalMs);
    connect(&m_feedTimer, &QTimer::timeout, this, &AudioPlayer::feedAudio);
}

bool AudioPlayer::isPlaying() const
{
    return m_playing;
}

void AudioPlayer::playSegments(const QVector<SoundSegment> &segments, int volume, int playCount)
{
    stop();
    const auto device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        qWarning("No default audio output device is available.");
        emit finished();
        return;
    }
    const auto format = playbackFormat(device);
    auto pcm = ToneGenerator::generatePcm(segments, volume, format);
    if (pcm.isEmpty()) {
        qWarning("Generated audio buffer is empty.");
        emit finished();
        return;
    }
    startPlayback(std::move(pcm), playCount, format, device);
}

void AudioPlayer::stop()
{
    m_feedTimer.stop();
    if (m_sink) {
        m_sink->stop();
        m_sink.reset();
    }
    m_output = nullptr;
    m_pcm.clear();
    m_repeatGapPcm.clear();
    m_tailPcm.clear();
    m_offset = 0;
    m_repeatGapOffset = 0;
    m_tailOffset = 0;
    m_completedLoops = 0;
    m_writingRepeatGap = false;
    m_draining = false;
    m_playing = false;
}

QAudioFormat AudioPlayer::playbackFormat(const QAudioDevice &device) const
{
    const auto preferred = device.preferredFormat();
    if (preferred.isValid() && preferred.sampleFormat() != QAudioFormat::Unknown) {
        return preferred;
    }
    const auto fallback = ToneGenerator::defaultFormat();
    if (device.isFormatSupported(fallback)) {
        return fallback;
    }
    return fallback;
}

void AudioPlayer::startPlayback(QByteArray pcm, int playCount, const QAudioFormat &format, const QAudioDevice &device)
{
    if (pcm.isEmpty()) {
        qWarning("Audio buffer is empty.");
        emit finished();
        return;
    }

    m_format = format;
    m_pcm = std::move(pcm);
    m_repeatGapPcm = playCount == 1 ? QByteArray() : ToneGenerator::generateSilence(RepeatGapMs, m_format);
    m_tailPcm = ToneGenerator::generateSilence(PlaybackTailSilenceMs, m_format);
    m_playCount = playCount;
    m_completedLoops = 0;
    m_offset = 0;
    m_repeatGapOffset = 0;
    m_tailOffset = 0;
    m_writingRepeatGap = false;
    m_draining = false;

    m_sink = std::make_unique<QAudioSink>(device, m_format);
    const auto bytesPerSec = bytesPerSecond();
    if (bytesPerSec > 0) {
        m_sink->setBufferSize(bytesPerSec * BufferDurationMs / 1000);
    }
    connect(m_sink.get(), &QAudioSink::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::IdleState && m_draining) {
            finishPlayback();
        } else if (state == QAudio::StoppedState && m_sink && m_sink->error() != QAudio::NoError) {
            qWarning() << "Audio playback stopped with error:" << m_sink->error();
            finishPlayback();
        }
    });
    m_output = m_sink->start();
    if (!m_output) {
        qWarning("Failed to start audio output.");
        m_sink.reset();
        emit finished();
        return;
    }

    m_playing = true;
    feedAudio();
    m_feedTimer.start();
}

void AudioPlayer::feedAudio()
{
    if (!m_playing || !m_sink || !m_output || m_pcm.isEmpty()) {
        return;
    }

    if (m_playCount > 0 && m_completedLoops >= m_playCount) {
        qsizetype bytesFree = static_cast<qsizetype>(m_sink->bytesFree());
        while (bytesFree > 0 && m_tailOffset < m_tailPcm.size()) {
            if (!writeFromBuffer(m_tailPcm, &m_tailOffset, &bytesFree)) {
                break;
            }
        }
        if (m_tailOffset >= m_tailPcm.size()) {
            m_draining = true;
            m_feedTimer.stop();
        }
        return;
    }

    qsizetype bytesFree = static_cast<qsizetype>(m_sink->bytesFree());
    while (bytesFree > 0 && (m_playCount == 0 || m_completedLoops < m_playCount)) {
        if (m_writingRepeatGap) {
            if (!writeFromBuffer(m_repeatGapPcm, &m_repeatGapOffset, &bytesFree)) {
                break;
            }
            if (m_repeatGapOffset >= m_repeatGapPcm.size()) {
                m_repeatGapOffset = 0;
                m_writingRepeatGap = false;
            }
        } else {
            if (!writeFromBuffer(m_pcm, &m_offset, &bytesFree)) {
                break;
            }
            if (m_offset >= m_pcm.size()) {
                m_offset = 0;
                if (m_playCount > 0) {
                    ++m_completedLoops;
                }
                if ((m_playCount == 0 || m_completedLoops < m_playCount) && !m_repeatGapPcm.isEmpty()) {
                    m_writingRepeatGap = true;
                }
            }
        }
    }
}

void AudioPlayer::finishPlayback()
{
    const bool wasPlaying = m_playing;
    stop();
    if (wasPlaying) {
        emit finished();
    }
}

qint64 AudioPlayer::bytesPerSecond() const
{
    return static_cast<qint64>(m_format.sampleRate()) * m_format.channelCount() * m_format.bytesPerSample();
}

bool AudioPlayer::writeFromBuffer(const QByteArray &buffer, qsizetype *offset, qsizetype *bytesFree)
{
    if (buffer.isEmpty() || !offset || !bytesFree || !m_output || *bytesFree <= 0 || *offset >= buffer.size()) {
        return false;
    }
    const qsizetype remaining = buffer.size() - *offset;
    const qsizetype chunkSize = qMin(*bytesFree, remaining);
    const qint64 written = m_output->write(buffer.constData() + *offset, chunkSize);
    if (written <= 0) {
        return false;
    }
    *offset += written;
    *bytesFree -= written;
    return true;
}

PreviewPlayer::PreviewPlayer(QObject *parent)
    : QObject(parent)
    , m_player(this)
{
}

bool PreviewPlayer::isPlaying() const
{
    return m_player.isPlaying();
}

void PreviewPlayer::playSegments(const QVector<SoundSegment> &segments, int volume)
{
    m_player.playSegments(segments, volume, 1);
}

void PreviewPlayer::stop()
{
    m_player.stop();
}

} // namespace smartalarm::audio
