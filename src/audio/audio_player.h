#pragma once

#include <QAudioDevice>
#include <QAudioSink>
#include <QIODevice>
#include <QObject>
#include <QTimer>

#include "audio/sound_pattern_parser.h"

namespace smartalarm::audio {

class AudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr);

    bool isPlaying() const;
    void playSegments(const QVector<SoundSegment> &segments, int volume, int playCount);
    void stop();

signals:
    void finished();

private:
    QAudioFormat playbackFormat(const QAudioDevice &device) const;
    void startPlayback(QByteArray pcm, int playCount, const QAudioFormat &format, const QAudioDevice &device);
    void feedAudio();
    void finishPlayback();
    qint64 bytesPerSecond() const;
    bool writeFromBuffer(const QByteArray &buffer, qsizetype *offset, qsizetype *bytesFree);

    QAudioFormat m_format;
    std::unique_ptr<QAudioSink> m_sink;
    QIODevice *m_output = nullptr;
    QTimer m_feedTimer;
    QByteArray m_pcm;
    QByteArray m_repeatGapPcm;
    QByteArray m_tailPcm;
    qsizetype m_offset = 0;
    qsizetype m_repeatGapOffset = 0;
    qsizetype m_tailOffset = 0;
    int m_playCount = 1;
    int m_completedLoops = 0;
    bool m_writingRepeatGap = false;
    bool m_draining = false;
    bool m_playing = false;
};

class PreviewPlayer : public QObject {
    Q_OBJECT
public:
    explicit PreviewPlayer(QObject *parent = nullptr);
    bool isPlaying() const;
    void playSegments(const QVector<SoundSegment> &segments, int volume);
    void stop();

private:
    AudioPlayer m_player;
};

} // namespace smartalarm::audio
