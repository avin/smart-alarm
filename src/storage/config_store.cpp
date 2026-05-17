#include "storage/config_store.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace smartalarm {

namespace {

constexpr auto ConfigVersion = "1";
constexpr auto ConfigFileName = "settings.json";

QJsonArray weekdaysToJson(const QVector<Weekday> &days)
{
    QJsonArray array;
    for (const auto day : days) {
        array.append(weekdayToString(day));
    }
    return array;
}

std::optional<QVector<Weekday>> weekdaysFromJson(const QJsonValue &value)
{
    if (!value.isArray()) {
        return std::nullopt;
    }
    QVector<Weekday> days;
    for (const auto item : value.toArray()) {
        if (!item.isString()) {
            return std::nullopt;
        }
        const auto weekday = weekdayFromString(item.toString());
        if (!weekday) {
            return std::nullopt;
        }
        if (!days.contains(*weekday)) {
            days.push_back(*weekday);
        }
    }
    return days;
}

QJsonObject dateRangeToJson(const DateRange &range)
{
    QJsonObject object;
    if (range.start) {
        object.insert(QStringLiteral("start"), range.start->toString(QStringLiteral("yyyy-MM-dd")));
    }
    if (range.end) {
        object.insert(QStringLiteral("end"), range.end->toString(QStringLiteral("yyyy-MM-dd")));
    }
    return object;
}

std::optional<DateRange> dateRangeFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    DateRange range;
    const auto object = value.toObject();
    if (object.contains(QStringLiteral("start"))) {
        const auto date = QDate::fromString(object.value(QStringLiteral("start")).toString(), QStringLiteral("yyyy-MM-dd"));
        if (!date.isValid()) {
            return std::nullopt;
        }
        range.start = date;
    }
    if (object.contains(QStringLiteral("end"))) {
        const auto date = QDate::fromString(object.value(QStringLiteral("end")).toString(), QStringLiteral("yyyy-MM-dd"));
        if (!date.isValid()) {
            return std::nullopt;
        }
        range.end = date;
    }
    return range;
}

std::optional<QDate> readDate(const QJsonObject &object, const QString &key)
{
    if (!object.contains(key) || !object.value(key).isString()) {
        return std::nullopt;
    }
    const auto date = QDate::fromString(object.value(key).toString(), QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return std::nullopt;
    }
    return date;
}

std::optional<QTime> readTime(const QJsonObject &object, const QString &key)
{
    if (!object.contains(key) || !object.value(key).isString()) {
        return std::nullopt;
    }
    const auto time = QTime::fromString(object.value(key).toString(), QStringLiteral("HH:mm"));
    if (!time.isValid()) {
        return std::nullopt;
    }
    return time;
}

std::optional<SoundSpec> soundFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    const auto type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("preset")) {
        const auto preset = soundPresetFromString(object.value(QStringLiteral("preset")).toString());
        if (!preset) {
            return std::nullopt;
        }
        return SoundSpec(PresetSound { *preset });
    }
    if (type == QStringLiteral("custom")) {
        const auto pattern = object.value(QStringLiteral("pattern")).toString();
        if (pattern.trimmed().isEmpty()) {
            return std::nullopt;
        }
        return SoundSpec(CustomSound { pattern });
    }
    return std::nullopt;
}

QJsonObject soundToJson(const SoundSpec &sound)
{
    return std::visit([](const auto &value) {
        QJsonObject object;
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, PresetSound>) {
            object.insert(QStringLiteral("type"), QStringLiteral("preset"));
            object.insert(QStringLiteral("preset"), soundPresetToString(value.preset));
        } else {
            object.insert(QStringLiteral("type"), QStringLiteral("custom"));
            object.insert(QStringLiteral("pattern"), value.pattern);
        }
        return object;
    }, sound);
}

QJsonObject scheduleToJson(const Schedule &schedule)
{
    return std::visit([](const auto &value) {
        QJsonObject object;
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, OnceSchedule>) {
            object.insert(QStringLiteral("type"), QStringLiteral("once"));
            if (value.date) object.insert(QStringLiteral("date"), value.date->toString(QStringLiteral("yyyy-MM-dd")));
            if (value.time) object.insert(QStringLiteral("time"), value.time->toString(QStringLiteral("HH:mm")));
        } else if constexpr (std::is_same_v<T, WeeklySchedule>) {
            object.insert(QStringLiteral("type"), QStringLiteral("weekly"));
            object.insert(QStringLiteral("days"), weekdaysToJson(value.days));
            if (value.time) object.insert(QStringLiteral("time"), value.time->toString(QStringLiteral("HH:mm")));
            if (value.dateRange && (value.dateRange->start || value.dateRange->end)) {
                object.insert(QStringLiteral("dateRange"), dateRangeToJson(*value.dateRange));
            }
        } else if constexpr (std::is_same_v<T, NthWeekSchedule>) {
            object.insert(QStringLiteral("type"), QStringLiteral("nth_week"));
            object.insert(QStringLiteral("everyWeeks"), value.everyWeeks);
            object.insert(QStringLiteral("weekday"), weekdayToString(value.weekday));
            if (value.time) object.insert(QStringLiteral("time"), value.time->toString(QStringLiteral("HH:mm")));
            if (value.referenceDate) object.insert(QStringLiteral("referenceDate"), value.referenceDate->toString(QStringLiteral("yyyy-MM-dd")));
            if (value.endDate) object.insert(QStringLiteral("endDate"), value.endDate->toString(QStringLiteral("yyyy-MM-dd")));
        } else {
            object.insert(QStringLiteral("type"), QStringLiteral("interval"));
            object.insert(QStringLiteral("everyMinutes"), value.everyMinutes);
            if (value.from) object.insert(QStringLiteral("from"), value.from->toString(QStringLiteral("HH:mm")));
            if (value.to) object.insert(QStringLiteral("to"), value.to->toString(QStringLiteral("HH:mm")));
            object.insert(QStringLiteral("countFrom"), countFromToString(value.countFrom));
            object.insert(QStringLiteral("snoozeMinutes"), value.snoozeMinutes);
            if (value.schedule) {
                QJsonObject limit;
                limit.insert(QStringLiteral("days"), weekdaysToJson(value.schedule->days));
                if (value.schedule->dateRange.start || value.schedule->dateRange.end) {
                    limit.insert(QStringLiteral("dateRange"), dateRangeToJson(value.schedule->dateRange));
                }
                object.insert(QStringLiteral("schedule"), limit);
            }
        }
        return object;
    }, schedule);
}

std::optional<Schedule> scheduleFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    const auto type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("once")) {
        const auto date = readDate(object, QStringLiteral("date"));
        const auto time = readTime(object, QStringLiteral("time"));
        if (!date || !time) return std::nullopt;
        return Schedule(OnceSchedule { date, time });
    }
    if (type == QStringLiteral("weekly")) {
        const auto days = weekdaysFromJson(object.value(QStringLiteral("days")));
        const auto time = readTime(object, QStringLiteral("time"));
        if (!days || days->isEmpty() || !time) return std::nullopt;
        WeeklySchedule schedule;
        schedule.days = *days;
        schedule.time = *time;
        if (object.contains(QStringLiteral("dateRange"))) {
            const auto range = dateRangeFromJson(object.value(QStringLiteral("dateRange")));
            if (!range) return std::nullopt;
            schedule.dateRange = *range;
        }
        return Schedule(schedule);
    }
    if (type == QStringLiteral("nth_week")) {
        const auto weekday = weekdayFromString(object.value(QStringLiteral("weekday")).toString());
        const auto time = readTime(object, QStringLiteral("time"));
        const auto referenceDate = readDate(object, QStringLiteral("referenceDate"));
        const int everyWeeks = object.value(QStringLiteral("everyWeeks")).toInt(-1);
        if (!weekday || !time || !referenceDate || everyWeeks < 1 || everyWeeks > 999) return std::nullopt;
        NthWeekSchedule schedule;
        schedule.everyWeeks = everyWeeks;
        schedule.weekday = *weekday;
        schedule.time = *time;
        schedule.referenceDate = *referenceDate;
        if (object.contains(QStringLiteral("endDate"))) {
            const auto endDate = readDate(object, QStringLiteral("endDate"));
            if (!endDate) return std::nullopt;
            schedule.endDate = *endDate;
        }
        return Schedule(schedule);
    }
    if (type == QStringLiteral("interval")) {
        const int everyMinutes = object.value(QStringLiteral("everyMinutes")).toInt(-1);
        const auto from = readTime(object, QStringLiteral("from"));
        const auto to = readTime(object, QStringLiteral("to"));
        const auto countFrom = countFromFromString(object.value(QStringLiteral("countFrom")).toString());
        const int snoozeMinutes = object.value(QStringLiteral("snoozeMinutes")).toInt(-1);
        if (everyMinutes < 1 || everyMinutes > 1440 || !from || !to || !countFrom || snoozeMinutes < 0 || snoozeMinutes > 1440) {
            return std::nullopt;
        }
        IntervalSchedule schedule;
        schedule.everyMinutes = everyMinutes;
        schedule.from = *from;
        schedule.to = *to;
        schedule.countFrom = *countFrom;
        schedule.snoozeMinutes = snoozeMinutes;
        if (object.contains(QStringLiteral("schedule"))) {
            const auto limitObject = object.value(QStringLiteral("schedule")).toObject();
            const auto days = weekdaysFromJson(limitObject.value(QStringLiteral("days")));
            if (!days || days->isEmpty()) return std::nullopt;
            IntervalScheduleLimit limit;
            limit.days = *days;
            if (limitObject.contains(QStringLiteral("dateRange"))) {
                const auto range = dateRangeFromJson(limitObject.value(QStringLiteral("dateRange")));
                if (!range) return std::nullopt;
                limit.dateRange = *range;
            }
            schedule.schedule = limit;
        }
        return Schedule(schedule);
    }
    return std::nullopt;
}

std::optional<Notification> notificationFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    const auto id = QUuid::fromString(object.value(QStringLiteral("id")).toString());
    const auto sound = soundFromJson(object.value(QStringLiteral("sound")));
    const auto schedule = scheduleFromJson(object.value(QStringLiteral("schedule")));
    if (id.isNull() || !object.value(QStringLiteral("enabled")).isBool() || !sound || !schedule) {
        return std::nullopt;
    }
    const auto message = object.value(QStringLiteral("message")).toString();
    const auto color = canonicalHexColor(object.value(QStringLiteral("color")).toString());
    const int volume = object.value(QStringLiteral("volume")).toInt(-1);
    const int playCount = object.value(QStringLiteral("playCount")).toInt(-1);
    if (message.trimmed().isEmpty() || !isCanonicalHexColor(color) || volume < 0 || volume > 100 || playCount < 0 || playCount > 999) {
        return std::nullopt;
    }
    Notification notification;
    notification.id = id;
    notification.enabled = object.value(QStringLiteral("enabled")).toBool();
    notification.message = message;
    notification.color = color;
    notification.volume = volume;
    notification.playCount = playCount;
    notification.sound = *sound;
    notification.schedule = *schedule;
    return notification;
}

} // namespace

ConfigStore::ConfigStore(QString filePath)
    : m_filePath(std::move(filePath))
{
    if (m_filePath.isEmpty()) {
        m_filePath = defaultFilePath();
    }
}

QString ConfigStore::filePath() const
{
    return m_filePath;
}

QString ConfigStore::defaultFilePath() const
{
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(dir).filePath(QString::fromLatin1(ConfigFileName));
}

QString ConfigStore::makeBackupPath() const
{
    const QFileInfo info(m_filePath);
    const auto stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return info.dir().filePath(QStringLiteral("settings.corrupt-%1.json").arg(stamp));
}

bool ConfigStore::backupExisting(QString *backupPath) const
{
    const auto path = makeBackupPath();
    QFile::remove(path);
    const bool ok = QFile::rename(m_filePath, path);
    if (ok && backupPath) {
        *backupPath = path;
    }
    return ok;
}

LoadResult ConfigStore::load()
{
    LoadResult result;
    QFile file(m_filePath);
    if (!file.exists()) {
        qInfo("Config file is missing. Starting with defaults.");
        result.status = LoadStatus::MissingDefaults;
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning().noquote() << "Failed to open config:" << file.errorString();
        result.status = LoadStatus::MissingDefaults;
        result.warnings << file.errorString();
        return result;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        backupExisting(&result.backupPath);
        qWarning().noquote() << "Corrupted config was backed up:" << result.backupPath;
        result.status = LoadStatus::CorruptedBackedUp;
        result.warnings << parseError.errorString();
        return result;
    }
    if (!document.isObject()) {
        backupExisting(&result.backupPath);
        qWarning().noquote() << "Wrong config root was backed up:" << result.backupPath;
        result.status = LoadStatus::WrongRootBackedUp;
        return result;
    }
    bool repaired = false;
    auto config = fromJson(document.object(), &repaired, &result.warnings);
    if (!config) {
        backupExisting(&result.backupPath);
        result.status = LoadStatus::WrongRootBackedUp;
        return result;
    }
    result.config = *config;
    result.status = repaired ? LoadStatus::LoadedWithRepairs : LoadStatus::Loaded;
    if (repaired) {
        qInfo("Config loaded with repairs. Saving canonical config.");
        save(result.config);
    }
    return result;
}

SaveResult ConfigStore::save(const AppConfig &config) const
{
    const QFileInfo info(m_filePath);
    if (!QDir().mkpath(info.dir().absolutePath())) {
        return { false, QStringLiteral("Failed to create config directory") };
    }
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return { false, file.errorString() };
    }
    const QJsonDocument document(toJson(config));
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        return { false, file.errorString() };
    }
    return { true, {} };
}

QJsonObject ConfigStore::toJson(const AppConfig &config)
{
    QJsonObject root;
    root.insert(QStringLiteral("_ver"), QString::fromLatin1(ConfigVersion));

    QJsonObject settings;
    settings.insert(QStringLiteral("defaultSnoozeMinutes"), config.settings.defaultSnoozeMinutes);
    settings.insert(QStringLiteral("notificationPosition"), notificationPositionToString(config.settings.notificationPosition));
    root.insert(QStringLiteral("settings"), settings);

    QJsonArray notifications;
    for (const auto &notification : config.notifications) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), notification.id.toString(QUuid::WithoutBraces));
        object.insert(QStringLiteral("enabled"), notification.enabled);
        object.insert(QStringLiteral("message"), notification.message);
        object.insert(QStringLiteral("color"), canonicalHexColor(notification.color));
        object.insert(QStringLiteral("volume"), notification.volume);
        object.insert(QStringLiteral("playCount"), notification.playCount);
        object.insert(QStringLiteral("sound"), soundToJson(notification.sound));
        object.insert(QStringLiteral("schedule"), scheduleToJson(notification.schedule));
        notifications.append(object);
    }
    root.insert(QStringLiteral("notifications"), notifications);
    return root;
}

std::optional<AppConfig> ConfigStore::fromJson(const QJsonObject &root, bool *repaired, QStringList *warnings)
{
    AppConfig config;
    auto markRepaired = [&] {
        if (repaired) *repaired = true;
    };
    if (!root.value(QStringLiteral("_ver")).isString()) {
        markRepaired();
        if (warnings) *warnings << QStringLiteral("Missing or invalid _ver");
    }
    if (root.contains(QStringLiteral("settings")) && root.value(QStringLiteral("settings")).isObject()) {
        const auto settings = root.value(QStringLiteral("settings")).toObject();
        const int snooze = settings.value(QStringLiteral("defaultSnoozeMinutes")).toInt(-1);
        if (snooze >= 1 && snooze <= 1440) {
            config.settings.defaultSnoozeMinutes = snooze;
        } else {
            markRepaired();
            if (warnings) *warnings << QStringLiteral("Invalid defaultSnoozeMinutes");
        }
        const auto position = notificationPositionFromString(settings.value(QStringLiteral("notificationPosition")).toString());
        if (position) {
            config.settings.notificationPosition = *position;
        } else {
            markRepaired();
            if (warnings) *warnings << QStringLiteral("Invalid notificationPosition");
        }
    } else {
        markRepaired();
        if (warnings) *warnings << QStringLiteral("Missing settings");
    }

    if (!root.contains(QStringLiteral("notifications")) || !root.value(QStringLiteral("notifications")).isArray()) {
        markRepaired();
        if (warnings) *warnings << QStringLiteral("Missing notifications");
        return config;
    }

    for (const auto item : root.value(QStringLiteral("notifications")).toArray()) {
        const auto notification = notificationFromJson(item);
        if (notification) {
            config.notifications.push_back(*notification);
        } else {
            markRepaired();
            if (warnings) *warnings << QStringLiteral("Broken notification removed");
        }
    }
    return config;
}

} // namespace smartalarm
