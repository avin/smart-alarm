#pragma once

#include "audio/audio_player.h"

#include <QUuid>
#include <QQueue>

namespace smartalarm::audio {

struct AudioTask {
    QUuid notificationId;
    QVector<SoundSegment> segments;
    int volume = 70;
    int playCount = 1;
};

class AudioQueue : public QObject {
    Q_OBJECT
public:
    explicit AudioQueue(QObject *parent = nullptr);

    bool isPlaying() const;
    void enqueue(AudioTask task);
    void stopNotification(const QUuid &notificationId);
    void clear();

private:
    void playNext();

    QQueue<AudioTask> m_queue;
    AudioPlayer m_player;
    std::optional<QUuid> m_currentId;
};

} // namespace smartalarm::audio
