#pragma once

#include <QObject>
#include <QTimer>

namespace smartalarm {

class MinuteScheduler : public QObject {
    Q_OBJECT
public:
    explicit MinuteScheduler(QObject *parent = nullptr);

    void start();
    void stop();

signals:
    void minuteTick(const QDateTime &now);

private:
    void scheduleNext();

    QTimer m_timer;
    bool m_running = false;
};

} // namespace smartalarm
