#include "audio/sound_pattern_parser.h"
#include "audio/tone_generator.h"

#include <QtTest/QtTest>

using namespace smartalarm::audio;

class TestToneGenerator : public QObject {
    Q_OBJECT
private slots:
    void generatesInt16AndFloat()
    {
        const auto parsed = SoundPatternParser::parse(QStringLiteral("880/100/sine"));
        QVERIFY(parsed.ok);

        auto int16Format = ToneGenerator::defaultFormat();
        int16Format.setSampleFormat(QAudioFormat::Int16);
        QVERIFY(!ToneGenerator::generatePcm(parsed.segments, 70, int16Format).isEmpty());

        auto floatFormat = ToneGenerator::defaultFormat();
        floatFormat.setSampleFormat(QAudioFormat::Float);
        QVERIFY(!ToneGenerator::generatePcm(parsed.segments, 70, floatFormat).isEmpty());
    }

    void fadesNoteEdges()
    {
        QCOMPARE(ToneGenerator::fadeEnvelope(0, 1000, 100), 0.0);
        QVERIFY(ToneGenerator::fadeEnvelope(50, 1000, 100) > 0.0);
        QVERIFY(ToneGenerator::fadeEnvelope(50, 1000, 100) < 1.0);
        QCOMPARE(ToneGenerator::fadeEnvelope(500, 1000, 100), 1.0);
        QCOMPARE(ToneGenerator::fadeEnvelope(999, 1000, 100), 0.0);
    }

    void fadesGeneratedToneEdges()
    {
        SoundSegment segment;
        segment.frequency = 880;
        segment.durationMs = 100;
        segment.waveform = smartalarm::Waveform::Square;

        auto format = ToneGenerator::defaultFormat();
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        const auto pcm = ToneGenerator::generatePcm({ segment }, 100, format);
        QVERIFY(pcm.size() >= static_cast<int>(sizeof(qint16)) * 2);

        const auto *samples = reinterpret_cast<const qint16 *>(pcm.constData());
        const int sampleCount = pcm.size() / static_cast<int>(sizeof(qint16));
        QCOMPARE(samples[0], 0);
        QCOMPARE(samples[sampleCount - 1], 0);
        QVERIFY(std::any_of(samples, samples + sampleCount, [](qint16 sample) { return std::abs(sample) > 1000; }));
    }

    void generatesSilence()
    {
        auto format = ToneGenerator::defaultFormat();
        const auto silence = ToneGenerator::generateSilence(300, format);
        QVERIFY(!silence.isEmpty());
        QVERIFY(std::all_of(silence.cbegin(), silence.cend(), [](char value) { return value == '\0'; }));
    }
};

QTEST_MAIN(TestToneGenerator)
#include "test_tone_generator.moc"
