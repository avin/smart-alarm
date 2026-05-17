#include "core/time_input_normalizer.h"

#include <QtTest/QtTest>

using namespace smartalarm;

class TestTimeInputNormalizer : public QObject {
    Q_OBJECT
private slots:
    void normalizesExamples()
    {
        QCOMPARE(TimeInputNormalizer::normalize(QStringLiteral("2")).value(), QTime(2, 0));
        QCOMPARE(TimeInputNormalizer::normalize(QStringLiteral("299")).value(), QTime(23, 9));
        QCOMPARE(TimeInputNormalizer::normalize(QStringLiteral("1130")).value(), QTime(11, 30));
        QCOMPARE(TimeInputNormalizer::normalize(QStringLiteral("2460")).value(), QTime(23, 59));
        QCOMPARE(TimeInputNormalizer::normalize(QStringLiteral(":7")).value(), QTime(0, 7));
    }
};

QTEST_MAIN(TestTimeInputNormalizer)
#include "test_time_input_normalizer.moc"
