#include "audio/sound_pattern_parser.h"

#include <QRegularExpression>

#include <cmath>

namespace smartalarm::audio {

namespace {

constexpr int MaxSegments = 128;
constexpr int MaxDurationMs = 60000;

std::optional<double> noteFrequency(QString note)
{
    note = note.trimmed();
    static const QRegularExpression re(QStringLiteral("^([A-Ga-g])([#b]?)([0-8])$"));
    const auto match = re.match(note);
    if (!match.hasMatch()) {
        return std::nullopt;
    }
    const auto letter = match.captured(1).toUpper();
    const auto accidental = match.captured(2);
    const int octave = match.captured(3).toInt();
    int semitone = 0;
    if (letter == QStringLiteral("C")) semitone = 0;
    else if (letter == QStringLiteral("D")) semitone = 2;
    else if (letter == QStringLiteral("E")) semitone = 4;
    else if (letter == QStringLiteral("F")) semitone = 5;
    else if (letter == QStringLiteral("G")) semitone = 7;
    else if (letter == QStringLiteral("A")) semitone = 9;
    else if (letter == QStringLiteral("B")) semitone = 11;
    if (accidental == QStringLiteral("#")) ++semitone;
    if (accidental == QStringLiteral("b")) --semitone;
    const int midi = (octave + 1) * 12 + semitone;
    if (midi < 21 || midi > 108) {
        return std::nullopt;
    }
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

std::optional<double> parseFrequency(const QString &value)
{
    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (ok) {
        if (numeric < 20.0 || numeric > 20000.0) {
            return std::nullopt;
        }
        return numeric;
    }
    const auto note = noteFrequency(value);
    if (!note || *note < 20.0 || *note > 20000.0) {
        return std::nullopt;
    }
    return note;
}

} // namespace

ParseResult SoundPatternParser::parse(const QString &pattern)
{
    ParseResult result;
    const auto parts = pattern.split(QLatin1Char(','), Qt::KeepEmptyParts);
    if (parts.isEmpty() || parts.size() > MaxSegments) {
        result.errorMessage = QStringLiteral("Pattern must contain 1..128 segments");
        return result;
    }
    int totalDuration = 0;
    for (const auto &rawPart : parts) {
        const auto part = rawPart.trimmed();
        if (part.isEmpty()) {
            result.errorMessage = QStringLiteral("Empty segment");
            return result;
        }
        const auto tokens = part.split(QLatin1Char('/'), Qt::KeepEmptyParts);
        if (tokens.size() < 2 || tokens.size() > 3) {
            result.errorMessage = QStringLiteral("Invalid segment format");
            return result;
        }
        bool durationOk = false;
        const int duration = tokens.at(1).trimmed().toInt(&durationOk);
        if (!durationOk || duration < 1 || duration > 10000) {
            result.errorMessage = QStringLiteral("Invalid duration");
            return result;
        }
        totalDuration += duration;
        if (totalDuration > MaxDurationMs) {
            result.errorMessage = QStringLiteral("Pattern is too long");
            return result;
        }

        SoundSegment segment;
        segment.durationMs = duration;
        const auto tone = tokens.at(0).trimmed();
        if (tone == QStringLiteral("_")) {
            segment.pause = true;
        } else {
            const auto frequency = parseFrequency(tone);
            if (!frequency) {
                result.errorMessage = QStringLiteral("Invalid frequency");
                return result;
            }
            segment.frequency = *frequency;
        }
        if (tokens.size() == 3) {
            const auto waveform = waveformFromString(tokens.at(2).trimmed());
            if (!waveform) {
                result.errorMessage = QStringLiteral("Invalid waveform");
                return result;
            }
            segment.waveform = *waveform;
        }
        result.segments.push_back(segment);
    }
    result.ok = true;
    return result;
}

} // namespace smartalarm::audio
