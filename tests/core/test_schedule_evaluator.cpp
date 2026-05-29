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

    void intervalResetOverridesTriggerGrid()
    {
        Notification notification;
        IntervalSchedule schedule;
        schedule.everyMinutes = 40;
        schedule.from = QTime(10, 0);
        schedule.to = QTime(13, 0);
        notification.schedule = schedule;

        RuntimeState runtime;
        runtime.intervalResetAt.insert(notification.id, QDateTime(QDate(2026, 5, 18), QTime(10, 10, 30)));

        QVERIFY(!ScheduleEvaluator::isNormalDue(notification, QDateTime(QDate(2026, 5, 18), QTime(10, 40)), runtime));
        QVERIFY(ScheduleEvaluator::isNormalDue(notification, QDateTime(QDate(2026, 5, 18), QTime(10, 51)), runtime));
        QCOMPARE(ScheduleEvaluator::nextOccurrence(notification, QDateTime(QDate(2026, 5, 18), QTime(10, 12)), runtime).value(), QDateTime(QDate(2026, 5, 18), QTime(10, 51)));
    }

    void nextOccurrenceSkipsAlreadyTriggeredMinute()
    {
        Notification notification;
        IntervalSchedule schedule;
        schedule.everyMinutes = 40;
        schedule.from = QTime(10, 0);
        schedule.to = QTime(13, 0);
        notification.schedule = schedule;

        RuntimeState runtime;
        runtime.lastTriggeredMinute.insert(notification.id, QDateTime(QDate(2026, 5, 18), QTime(10, 40)));

        QCOMPARE(ScheduleEvaluator::nextOccurrence(notification, QDateTime(QDate(2026, 5, 18), QTime(10, 40, 20)), runtime).value(), QDateTime(QDate(2026, 5, 18), QTime(11, 20)));
    }
};

QTEST_MAIN(TestScheduleEvaluator)
#include "test_schedule_evaluator.moc"
