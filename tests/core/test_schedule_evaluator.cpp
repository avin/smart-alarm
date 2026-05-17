#include "core/schedule_evaluator.h"

#include <QtTest/QtTest>

using namespace smartalarm;

class TestScheduleEvaluator : public QObject {
    Q_OBJECT
private slots:
    void onceDueInSameMinute()
    {
        Notification notification;
        notification.schedule = OnceSchedule { QDate(2026, 5, 17), QTime(10, 55) };
        RuntimeState runtime;
        QVERIFY(ScheduleEvaluator::isNormalDue(notification, QDateTime(QDate(2026, 5, 17), QTime(10, 55, 40)), runtime));
    }

    void intervalTriggerGrid()
    {
        Notification notification;
        IntervalSchedule schedule;
        schedule.everyMinutes = 40;
        schedule.from = QTime(10, 0);
        schedule.to = QTime(13, 0);
        notification.schedule = schedule;
        RuntimeState runtime;
        QVERIFY(ScheduleEvaluator::isNormalDue(notification, QDateTime(QDate(2026, 5, 18), QTime(11, 20)), runtime));
        QVERIFY(!ScheduleEvaluator::isNormalDue(notification, QDateTime(QDate(2026, 5, 18), QTime(11, 30)), runtime));
    }
};

QTEST_MAIN(TestScheduleEvaluator)
#include "test_schedule_evaluator.moc"
