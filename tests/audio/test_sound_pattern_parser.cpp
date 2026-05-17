#include "audio/sound_pattern_parser.h"

#include <QtTest/QtTest>

using namespace smartalarm::audio;

class TestSoundPatternParser : public QObject {
    Q_OBJECT
private slots:
    void parsesNotesAndPauses()
    {
        const auto result = SoundPatternParser::parse(QStringLiteral("C5/200, _/50, 880/100/square"));
        QVERIFY(result.ok);
        QCOMPARE(result.segments.size(), 3);
        QVERIFY(result.segments.at(1).pause);
    }

    void rejectsEmptySegment()
    {
        QVERIFY(!SoundPatternParser::parse(QStringLiteral("880/100,")).ok);
    }
};

QTEST_MAIN(TestSoundPatternParser)
#include "test_sound_pattern_parser.moc"
