#include "audio/audio_queue.h"

namespace smartalarm::audio {

AudioQueue::AudioQueue(QObject *parent)
    : QObject(parent)
    , m_player(this)
{
    connect(&m_player, &AudioPlayer::finished, this, &AudioQueue::playNext, Qt::QueuedConnection);
}

bool AudioQueue::isPlaying() const
{
    return m_player.isPlaying();
}

void AudioQueue::enqueue(AudioTask task)
{
    if (task.segments.isEmpty() || task.volume <= 0) {
        return;
    }
    m_queue.enqueue(std::move(task));
    if (!m_currentId) {
        playNext();
    }
}

void AudioQueue::stopNotification(const QUuid &notificationId)
{
    if (m_currentId && *m_currentId == notificationId) {
        m_player.stop();
        m_currentId.reset();
    }
    QQueue<AudioTask> filtered;
    while (!m_queue.isEmpty()) {
        auto task = m_queue.dequeue();
        if (task.notificationId != notificationId) {
            filtered.enqueue(std::move(task));
        }
    }
    m_queue = std::move(filtered);
    if (!m_currentId) {
        playNext();
    }
}

void AudioQueue::clear()
{
    m_queue.clear();
    m_player.stop();
    m_currentId.reset();
}

void AudioQueue::playNext()
{
    if (m_queue.isEmpty()) {
        m_currentId.reset();
        return;
    }
    const auto task = m_queue.dequeue();
    m_currentId = task.notificationId;
    m_player.playSegments(task.segments, task.volume, task.playCount);
}

} // namespace smartalarm::audio
