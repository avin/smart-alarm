#include "app/command_server.h"

#include "app/app_controller.h"
#include "audio/sound_pattern_parser.h"
#include "core/notification_validator.h"
#include "core/schedule_evaluator.h"
#include "core/schedule_formatter.h"
#include "storage/config_store.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>

namespace smartalarm {

namespace {

QJsonObject success(QJsonObject data = {})
{
    QJsonObject response;
    response.insert(QStringLiteral("ok"), true);
    response.insert(QStringLiteral("data"), data);
    return response;
}

QJsonObject failure(const QString &code, const QString &message, const QString &field = {})
{
    QJsonObject error;
    error.insert(QStringLiteral("code"), code);
    error.insert(QStringLiteral("message"), message);
    if (!field.isEmpty()) {
        error.insert(QStringLiteral("field"), field);
    }

    QJsonObject response;
    response.insert(QStringLiteral("ok"), false);
    response.insert(QStringLiteral("error"), error);
    return response;
}

std::optional<QString> optionString(const QJsonObject &options, const QString &key)
{
    if (!options.contains(key)) {
        return std::nullopt;
    }
    const auto value = options.value(key);
    if (!value.isString()) {
        return std::nullopt;
    }
    return value.toString();
}

bool hasAnyOption(const QJsonObject &options, const QStringList &keys)
{
    for (const auto &key : keys) {
        if (options.contains(key)) {
            return true;
        }
    }
    return false;
}

std::optional<int> parseInt(const QJsonObject &options, const QString &key, int min, int max, QString *error)
{
    const auto value = optionString(options, key);
    if (!value) {
        return std::nullopt;
    }
    bool ok = false;
    const int parsed = value->toInt(&ok);
    if (!ok || parsed < min || parsed > max) {
        if (error) {
            *error = QStringLiteral("%1 must be in range %2..%3").arg(key).arg(min).arg(max);
        }
        return std::nullopt;
    }
    return parsed;
}

std::optional<bool> parseBool(const QJsonObject &options, const QString &key, QString *error)
{
    const auto value = optionString(options, key);
    if (!value) {
        return std::nullopt;
    }
    const auto normalized = value->trimmed().toLower();
    if (normalized == QStringLiteral("true") || normalized == QStringLiteral("yes") || normalized == QStringLiteral("1")) {
        return true;
    }
    if (normalized == QStringLiteral("false") || normalized == QStringLiteral("no") || normalized == QStringLiteral("0")) {
        return false;
    }
    if (error) {
        *error = QStringLiteral("%1 must be true or false").arg(key);
    }
    return std::nullopt;
}

std::optional<QDate> parseDate(const QJsonObject &options, const QString &key, QString *error)
{
    const auto value = optionString(options, key);
    if (!value) {
        return std::nullopt;
    }
    const auto date = QDate::fromString(*value, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        if (error) {
            *error = QStringLiteral("%1 must use yyyy-MM-dd").arg(key);
        }
        return std::nullopt;
    }
    return date;
}

std::optional<QTime> parseTime(const QJsonObject &options, const QString &key, QString *error)
{
    const auto value = optionString(options, key);
    if (!value) {
        return std::nullopt;
    }
    const auto time = QTime::fromString(*value, QStringLiteral("HH:mm"));
    if (!time.isValid()) {
        if (error) {
            *error = QStringLiteral("%1 must use HH:mm").arg(key);
        }
        return std::nullopt;
    }
    return time;
}

std::optional<QUuid> parseUuid(const QJsonObject &options, QString *error)
{
    const auto value = optionString(options, QStringLiteral("uuid"));
    if (!value) {
        if (error) {
            *error = QStringLiteral("uuid is required");
        }
        return std::nullopt;
    }
    const auto uuid = QUuid::fromString(*value);
    if (uuid.isNull()) {
        if (error) {
            *error = QStringLiteral("uuid is invalid");
        }
        return std::nullopt;
    }
    return uuid;
}

std::optional<QVector<Weekday>> parseDays(const QJsonObject &options, QString *error)
{
    const auto value = optionString(options, QStringLiteral("days"));
    if (!value) {
        return std::nullopt;
    }
    QVector<Weekday> days;
    const auto parts = value->split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const auto &part : parts) {
        const auto weekday = weekdayFromString(part);
        if (!weekday) {
            if (error) {
                *error = QStringLiteral("days contains an invalid weekday");
            }
            return std::nullopt;
        }
        if (!days.contains(*weekday)) {
            days.push_back(*weekday);
        }
    }
    return days;
}

std::optional<SoundSpec> parseSound(const QJsonObject &options, const SoundSpec &fallback, bool requireSoundFlag, QString *error)
{
    const auto sound = optionString(options, QStringLiteral("sound"));
    const auto pattern = optionString(options, QStringLiteral("pattern"));
    if (!sound) {
        if (pattern) {
            if (error) {
                *error = QStringLiteral("sound must be custom when pattern is provided");
            }
            return std::nullopt;
        }
        return requireSoundFlag ? std::optional<SoundSpec> {} : std::optional<SoundSpec>(fallback);
    }

    const auto normalized = sound->trimmed().toLower();
    if (normalized == QStringLiteral("custom")) {
        if (!pattern || pattern->trimmed().isEmpty()) {
            if (error) {
                *error = QStringLiteral("pattern is required for custom sound");
            }
            return std::nullopt;
        }
        const auto parsed = audio::SoundPatternParser::parse(*pattern);
        if (!parsed.ok) {
            if (error) {
                *error = parsed.errorMessage;
            }
            return std::nullopt;
        }
        return SoundSpec(CustomSound { *pattern });
    }

    const auto preset = soundPresetFromString(normalized);
    if (!preset) {
        if (error) {
            *error = QStringLiteral("sound preset is invalid");
        }
        return std::nullopt;
    }
    if (pattern) {
        if (error) {
            *error = QStringLiteral("pattern is only valid with custom sound");
        }
        return std::nullopt;
    }
    return SoundSpec(PresetSound { *preset });
}

std::optional<DateRange> parseDateRange(const QJsonObject &options, QString *error)
{
    DateRange range;
    if (options.contains(QStringLiteral("start-date"))) {
        const auto date = parseDate(options, QStringLiteral("start-date"), error);
        if (!date) {
            return std::nullopt;
        }
        range.start = *date;
    }
    if (options.contains(QStringLiteral("end-date"))) {
        const auto date = parseDate(options, QStringLiteral("end-date"), error);
        if (!date) {
            return std::nullopt;
        }
        range.end = *date;
    }
    return range;
}

std::optional<Schedule> parseSchedule(const QJsonObject &options, QString *error)
{
    const auto typeValue = optionString(options, QStringLiteral("schedule-type"));
    if (!typeValue) {
        if (error) {
            *error = QStringLiteral("schedule-type is required");
        }
        return std::nullopt;
    }
    const auto type = typeValue->trimmed().toLower();
    if (type == QStringLiteral("once")) {
        const auto date = parseDate(options, QStringLiteral("date"), error);
        const auto time = parseTime(options, QStringLiteral("time"), error);
        if (!date || !time) {
            return std::nullopt;
        }
        return Schedule(OnceSchedule { *date, *time });
    }
    if (type == QStringLiteral("weekly")) {
        const auto days = parseDays(options, error);
        const auto time = parseTime(options, QStringLiteral("time"), error);
        if (!days || !time) {
            return std::nullopt;
        }
        WeeklySchedule schedule;
        schedule.days = *days;
        schedule.time = *time;
        const auto range = parseDateRange(options, error);
        if (!range) {
            return std::nullopt;
        }
        if (range->start || range->end) {
            schedule.dateRange = *range;
        }
        return Schedule(schedule);
    }
    if (type == QStringLiteral("nth-week")) {
        const auto everyWeeks = parseInt(options, QStringLiteral("every-weeks"), 1, 999, error);
        const auto weekdayValue = optionString(options, QStringLiteral("weekday"));
        const auto weekday = weekdayValue ? weekdayFromString(*weekdayValue) : std::optional<Weekday> {};
        const auto time = parseTime(options, QStringLiteral("time"), error);
        const auto referenceDate = parseDate(options, QStringLiteral("reference-date"), error);
        if (!everyWeeks || !weekday || !time || !referenceDate) {
            if (error && error->isEmpty()) {
                *error = QStringLiteral("every-weeks, weekday, time and reference-date are required");
            }
            return std::nullopt;
        }
        NthWeekSchedule schedule;
        schedule.everyWeeks = *everyWeeks;
        schedule.weekday = *weekday;
        schedule.time = *time;
        schedule.referenceDate = *referenceDate;
        if (options.contains(QStringLiteral("end-date"))) {
            const auto endDate = parseDate(options, QStringLiteral("end-date"), error);
            if (!endDate) {
                return std::nullopt;
            }
            schedule.endDate = *endDate;
        }
        return Schedule(schedule);
    }
    if (type == QStringLiteral("interval")) {
        const auto everyMinutes = parseInt(options, QStringLiteral("every-minutes"), 1, 1440, error);
        const auto from = parseTime(options, QStringLiteral("from"), error);
        const auto to = parseTime(options, QStringLiteral("to"), error);
        const auto countFromValue = optionString(options, QStringLiteral("count-from"));
        const auto countFrom = countFromValue ? countFromFromString(*countFromValue) : std::optional<CountFrom> {};
        if (!everyMinutes || !from || !to || !countFrom) {
            if (error && error->isEmpty()) {
                *error = QStringLiteral("every-minutes, from, to and count-from are required");
            }
            return std::nullopt;
        }
        IntervalSchedule schedule;
        schedule.everyMinutes = *everyMinutes;
        schedule.from = *from;
        schedule.to = *to;
        schedule.countFrom = *countFrom;
        if (options.contains(QStringLiteral("snooze-minutes"))) {
            const auto snooze = parseInt(options, QStringLiteral("snooze-minutes"), 0, 1440, error);
            if (!snooze) {
                return std::nullopt;
            }
            schedule.snoozeMinutes = *snooze;
        }
        const bool hasLimit = options.contains(QStringLiteral("days")) || options.contains(QStringLiteral("start-date")) || options.contains(QStringLiteral("end-date"));
        if (hasLimit) {
            const auto days = parseDays(options, error);
            if (!days || days->isEmpty()) {
                if (error) {
                    *error = QStringLiteral("days is required when interval schedule limit is used");
                }
                return std::nullopt;
            }
            const auto range = parseDateRange(options, error);
            if (!range) {
                return std::nullopt;
            }
            schedule.schedule = IntervalScheduleLimit { *days, *range };
        }
        return Schedule(schedule);
    }
    if (error) {
        *error = QStringLiteral("schedule-type is invalid");
    }
    return std::nullopt;
}

bool applyCommonNotificationOptions(Notification &notification, const QJsonObject &options, bool requireMessage, QString *error)
{
    if (const auto enabled = parseBool(options, QStringLiteral("enabled"), error)) {
        notification.enabled = *enabled;
    } else if (options.contains(QStringLiteral("enabled"))) {
        return false;
    }

    if (const auto message = optionString(options, QStringLiteral("message"))) {
        notification.message = *message;
    } else if (requireMessage) {
        if (error) {
            *error = QStringLiteral("message is required");
        }
        return false;
    }

    if (const auto color = optionString(options, QStringLiteral("color"))) {
        if (!isCanonicalHexColor(*color)) {
            if (error) {
                *error = QStringLiteral("color must use #RRGGBB");
            }
            return false;
        }
        notification.color = canonicalHexColor(*color);
    }
    if (options.contains(QStringLiteral("volume"))) {
        const auto volume = parseInt(options, QStringLiteral("volume"), 0, 100, error);
        if (!volume) {
            return false;
        }
        notification.volume = *volume;
    }
    if (options.contains(QStringLiteral("play-count"))) {
        const auto playCount = parseInt(options, QStringLiteral("play-count"), 0, 999, error);
        if (!playCount) {
            return false;
        }
        notification.playCount = *playCount;
    }
    if (options.contains(QStringLiteral("sound")) || options.contains(QStringLiteral("pattern"))) {
        const auto sound = parseSound(options, notification.sound, true, error);
        if (!sound) {
            return false;
        }
        notification.sound = *sound;
    }
    return true;
}

QJsonObject notificationToJson(const Notification &notification, AppController *controller)
{
    AppConfig config;
    config.notifications.push_back(notification);
    auto object = ConfigStore::toJson(config).value(QStringLiteral("notifications")).toArray().first().toObject();
    object.insert(QStringLiteral("uuid"), object.take(QStringLiteral("id")).toString());
    object.insert(QStringLiteral("scheduleText"), ScheduleFormatter::format(notification.schedule));
    object.insert(QStringLiteral("runtimeOnly"), controller->isRuntimeOnlyNotification(notification.id));
    const auto next = controller->nextNotificationTime(notification.id, QDateTime::currentDateTime());
    if (next) {
        object.insert(QStringLiteral("nextTriggerAt"), next->toString(Qt::ISODate));
    }
    object.insert(QStringLiteral("overdue"), ScheduleEvaluator::isOnceOverdue(notification, QDateTime::currentDateTime()));
    return object;
}

QJsonObject resultResponse(const OperationResult &result, QJsonObject data = {})
{
    if (!result.ok) {
        return failure(QStringLiteral("operation_failed"), result.errorMessage);
    }
    return success(data);
}

} // namespace

CommandServer::CommandServer(AppController *controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &CommandServer::handleConnection);
}

bool CommandServer::start()
{
    QLocalServer::removeServer(serverName());
    return m_server->listen(serverName());
}

QString CommandServer::serverName()
{
    return QStringLiteral("SmartAlarm.CommandServer");
}

void CommandServer::handleConnection()
{
    while (auto *socket = m_server->nextPendingConnection()) {
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            handleRequest(socket);
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void CommandServer::handleRequest(QLocalSocket *socket)
{
    QJsonParseError parseError;
    const auto requestDocument = QJsonDocument::fromJson(socket->readAll(), &parseError);
    QJsonObject response;
    if (parseError.error != QJsonParseError::NoError || !requestDocument.isObject()) {
        response = failure(QStringLiteral("protocol_error"), QStringLiteral("Invalid command request"));
    } else {
        response = dispatch(requestDocument.object());
    }
    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
    socket->flush();
    socket->disconnectFromServer();
}

QJsonObject CommandServer::dispatch(const QJsonObject &request) const
{
    const auto command = request.value(QStringLiteral("command")).toString();
    const auto options = request.value(QStringLiteral("options")).toObject();
    QString error;

    if (command == QStringLiteral("list")) {
        QJsonArray notifications;
        for (const auto &notification : m_controller->notifications()) {
            notifications.append(notificationToJson(notification, m_controller));
        }
        QJsonObject data;
        data.insert(QStringLiteral("notifications"), notifications);
        return success(data);
    }

    if (command == QStringLiteral("get")) {
        const auto uuid = parseUuid(options, &error);
        if (!uuid) {
            return failure(QStringLiteral("validation_error"), error, QStringLiteral("uuid"));
        }
        for (const auto &notification : m_controller->notifications()) {
            if (notification.id == *uuid) {
                QJsonObject data;
                data.insert(QStringLiteral("notification"), notificationToJson(notification, m_controller));
                return success(data);
            }
        }
        return failure(QStringLiteral("not_found"), QStringLiteral("Notification was not found"));
    }

    if (command == QStringLiteral("add")) {
        Notification notification;
        if (!applyCommonNotificationOptions(notification, options, true, &error)) {
            return failure(QStringLiteral("validation_error"), error);
        }
        const auto schedule = parseSchedule(options, &error);
        if (!schedule) {
            return failure(QStringLiteral("validation_error"), error);
        }
        notification.schedule = *schedule;
        const auto result = m_controller->addNotification(notification);
        QJsonObject data;
        data.insert(QStringLiteral("uuid"), notification.id.toString(QUuid::WithoutBraces));
        return resultResponse(result, data);
    }

    if (command == QStringLiteral("update")) {
        const auto uuid = parseUuid(options, &error);
        if (!uuid) {
            return failure(QStringLiteral("validation_error"), error, QStringLiteral("uuid"));
        }
        std::optional<Notification> existing;
        for (const auto &notification : m_controller->notifications()) {
            if (notification.id == *uuid) {
                existing = notification;
                break;
            }
        }
        if (!existing) {
            return failure(QStringLiteral("not_found"), QStringLiteral("Notification was not found"));
        }
        auto next = *existing;
        if (!applyCommonNotificationOptions(next, options, false, &error)) {
            return failure(QStringLiteral("validation_error"), error);
        }
        const QStringList scheduleKeys {
            QStringLiteral("date"), QStringLiteral("time"), QStringLiteral("days"), QStringLiteral("start-date"), QStringLiteral("end-date"),
            QStringLiteral("every-weeks"), QStringLiteral("weekday"), QStringLiteral("reference-date"), QStringLiteral("every-minutes"),
            QStringLiteral("from"), QStringLiteral("to"), QStringLiteral("count-from"), QStringLiteral("snooze-minutes")
        };
        if (options.contains(QStringLiteral("schedule-type"))) {
            const auto schedule = parseSchedule(options, &error);
            if (!schedule) {
                return failure(QStringLiteral("validation_error"), error);
            }
            next.schedule = *schedule;
        } else if (hasAnyOption(options, scheduleKeys)) {
            return failure(QStringLiteral("validation_error"), QStringLiteral("schedule-type is required to update schedule"));
        }
        const auto result = m_controller->updateNotification(*uuid, next);
        QJsonObject data;
        data.insert(QStringLiteral("uuid"), uuid->toString(QUuid::WithoutBraces));
        return resultResponse(result, data);
    }

    if (command == QStringLiteral("delete")) {
        const auto uuid = parseUuid(options, &error);
        if (!uuid) {
            return failure(QStringLiteral("validation_error"), error, QStringLiteral("uuid"));
        }
        return resultResponse(m_controller->deleteNotification(*uuid));
    }

    if (command == QStringLiteral("trigger")) {
        RuntimeNotificationOptions trigger;
        if (const auto message = optionString(options, QStringLiteral("message"))) {
            trigger.message = *message;
        } else {
            return failure(QStringLiteral("validation_error"), QStringLiteral("message is required"), QStringLiteral("message"));
        }
        if (const auto color = optionString(options, QStringLiteral("color"))) {
            if (!isCanonicalHexColor(*color)) {
                return failure(QStringLiteral("validation_error"), QStringLiteral("color must use #RRGGBB"), QStringLiteral("color"));
            }
            trigger.color = canonicalHexColor(*color);
        }
        if (options.contains(QStringLiteral("volume"))) {
            const auto volume = parseInt(options, QStringLiteral("volume"), 0, 100, &error);
            if (!volume) return failure(QStringLiteral("validation_error"), error);
            trigger.volume = *volume;
        }
        if (options.contains(QStringLiteral("play-count"))) {
            const auto playCount = parseInt(options, QStringLiteral("play-count"), 0, 999, &error);
            if (!playCount) return failure(QStringLiteral("validation_error"), error);
            trigger.playCount = *playCount;
        }
        if (options.contains(QStringLiteral("snooze-minutes"))) {
            const auto snooze = parseInt(options, QStringLiteral("snooze-minutes"), 0, 1440, &error);
            if (!snooze) return failure(QStringLiteral("validation_error"), error);
            trigger.snoozeMinutes = *snooze;
        }
        if (options.contains(QStringLiteral("sound")) || options.contains(QStringLiteral("pattern"))) {
            const auto sound = parseSound(options, trigger.sound, true, &error);
            if (!sound) return failure(QStringLiteral("validation_error"), error);
            trigger.sound = *sound;
        }
        const auto result = m_controller->triggerRuntimeNotification(trigger);
        if (!result.operation.ok) {
            return failure(QStringLiteral("operation_failed"), result.operation.errorMessage);
        }
        QJsonObject data;
        data.insert(QStringLiteral("uuid"), result.uuid.toString(QUuid::WithoutBraces));
        data.insert(QStringLiteral("runtimeOnly"), true);
        return success(data);
    }

    if (command == QStringLiteral("status")) {
        QJsonArray active;
        for (const auto &uuid : m_controller->activeNotificationIds()) {
            QJsonObject item;
            item.insert(QStringLiteral("uuid"), uuid.toString(QUuid::WithoutBraces));
            item.insert(QStringLiteral("runtimeOnly"), m_controller->isRuntimeOnlyNotification(uuid));
            if (const auto notification = m_controller->notificationByUuid(uuid)) {
                item.insert(QStringLiteral("message"), notification->message);
            }
            active.append(item);
        }
        QJsonObject data;
        data.insert(QStringLiteral("runtimeNotificationsEnabled"), m_controller->runtimeNotificationsEnabled());
        data.insert(QStringLiteral("notificationCount"), m_controller->notifications().size());
        data.insert(QStringLiteral("activePopupCount"), m_controller->activeNotificationCount());
        data.insert(QStringLiteral("audioPlaying"), m_controller->isAudioPlaying());
        data.insert(QStringLiteral("activeNotifications"), active);
        return success(data);
    }

    if (command == QStringLiteral("enable-runtime") || command == QStringLiteral("disable-runtime")) {
        m_controller->setRuntimeNotificationsEnabled(command == QStringLiteral("enable-runtime"));
        return success();
    }

    if (command == QStringLiteral("reset-interval")) {
        const auto uuid = parseUuid(options, &error);
        if (!uuid) {
            return failure(QStringLiteral("validation_error"), error, QStringLiteral("uuid"));
        }
        return resultResponse(m_controller->resetIntervalTimer(*uuid, QDateTime::currentDateTime()));
    }

    if (command == QStringLiteral("dismiss") || command == QStringLiteral("snooze")) {
        const auto uuid = parseUuid(options, &error);
        if (!uuid) {
            return failure(QStringLiteral("validation_error"), error, QStringLiteral("uuid"));
        }
        if (!m_controller->hasActiveNotification(*uuid)) {
            return failure(QStringLiteral("not_active"), QStringLiteral("Notification popup is not active"));
        }
        if (command == QStringLiteral("dismiss")) {
            m_controller->dismissNotification(*uuid);
        } else {
            m_controller->snoozeNotification(*uuid);
        }
        return success();
    }

    return failure(QStringLiteral("validation_error"), QStringLiteral("Unknown CLI command"));
}

} // namespace smartalarm
