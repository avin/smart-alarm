#include "storage/config_store.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace smartalarm;

class TestConfigStore : public QObject {
    Q_OBJECT
private slots:
    void missingFileUsesDefaults()
    {
        QTemporaryDir dir;
        ConfigStore store(dir.filePath(QStringLiteral("settings.json")));
        const auto result = store.load();
        QCOMPARE(result.status, LoadStatus::MissingDefaults);
        QCOMPARE(result.config.settings.defaultSnoozeMinutes, 1);
    }

    void saveAndLoadNotification()
    {
        QTemporaryDir dir;
        ConfigStore store(dir.filePath(QStringLiteral("settings.json")));
        AppConfig config;
        Notification notification;
        notification.message = QStringLiteral("Stand up");
        notification.schedule = OnceSchedule { QDate(2026, 5, 17), QTime(10, 55) };
        config.notifications.push_back(notification);
        QVERIFY(store.save(config).ok);
        const auto result = store.load();
        QCOMPARE(result.status, LoadStatus::Loaded);
        QCOMPARE(result.config.notifications.size(), 1);
        QCOMPARE(result.config.notifications.first().message, QStringLiteral("Stand up"));
    }
};

QTEST_MAIN(TestConfigStore)
#include "test_config_store.moc"
