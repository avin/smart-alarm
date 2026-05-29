#include "app/command_client.h"

#include "app/command_server.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTextStream>

namespace smartalarm {

namespace {

struct ParsedArguments {
    bool ok = false;
    bool jsonOutput = false;
    QString errorMessage;
    QString command;
    QJsonObject options;
};

struct HelpEntry {
    QString command;
    QString summary;
    QString usage;
    QString details;
};

void writeStdout(const QString &text)
{
    QTextStream stream(stdout);
    stream << text;
    stream.flush();
}

void writeStderr(const QString &text)
{
    QTextStream stream(stderr);
    stream << text;
    stream.flush();
}

void printJson(const QJsonObject &response)
{
    writeStdout(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)) + QLatin1Char('\n'));
}

QVector<HelpEntry> helpEntries()
{
    return {
        {
            QStringLiteral("status"),
            QStringLiteral("Show runtime state."),
            QStringLiteral("SmartAlarmCli.exe status [--json]"),
            QStringLiteral("Shows runtimeNotificationsEnabled, notificationCount, activePopupCount, and audioPlaying.")
        },
        {
            QStringLiteral("list"),
            QStringLiteral("List saved notifications."),
            QStringLiteral("SmartAlarmCli.exe list [--json]"),
            QStringLiteral("Lists persistent notifications only. Runtime-only trigger popups are not included.")
        },
        {
            QStringLiteral("get"),
            QStringLiteral("Show one saved notification."),
            QStringLiteral("SmartAlarmCli.exe get --uuid UUID [--json]"),
            QStringLiteral("UUID may be passed with or without braces. Output always uses UUID without braces.")
        },
        {
            QStringLiteral("add"),
            QStringLiteral("Create a saved notification."),
            QStringLiteral("SmartAlarmCli.exe add --message TEXT --schedule-type TYPE [schedule options] [common options] [--json]"),
            QStringLiteral(
                "Schedule types:\n"
                "  once: --date yyyy-MM-dd --time HH:mm\n"
                "  weekly: --days mon,wed --time HH:mm [--start-date yyyy-MM-dd] [--end-date yyyy-MM-dd]\n"
                "  nth-week: --every-weeks N --weekday mon --time HH:mm --reference-date yyyy-MM-dd [--end-date yyyy-MM-dd]\n"
                "  interval: --every-minutes N --from HH:mm --to HH:mm --count-from trigger|confirmation [--days mon,tue] [--start-date yyyy-MM-dd] [--end-date yyyy-MM-dd] [--snooze-minutes N]\n"
                "Common options: --enabled true|false --color #RRGGBB --sound PRESET|custom --pattern PATTERN --volume 0..100 --play-count 0..999")
        },
        {
            QStringLiteral("update"),
            QStringLiteral("Update a saved notification."),
            QStringLiteral("SmartAlarmCli.exe update --uuid UUID [common options] [--schedule-type TYPE schedule options] [--json]"),
            QStringLiteral("Common fields can be patched. Schedule changes require --schedule-type and all required fields for the new schedule.")
        },
        {
            QStringLiteral("delete"),
            QStringLiteral("Delete a saved notification immediately."),
            QStringLiteral("SmartAlarmCli.exe delete --uuid UUID [--json]"),
            QStringLiteral("Deletes without confirmation. Runtime cleanup is handled by the running app after a successful save.")
        },
        {
            QStringLiteral("trigger"),
            QStringLiteral("Show a runtime-only popup now."),
            QStringLiteral("SmartAlarmCli.exe trigger --message TEXT [--color #RRGGBB] [--sound PRESET|custom] [--pattern PATTERN] [--volume 0..100] [--play-count 0..999] [--snooze-minutes 0..1440] [--json]"),
            QStringLiteral("Does not save JSON and ignores the global runtime toggle. Returns a runtime-only uuid.")
        },
        {
            QStringLiteral("dismiss"),
            QStringLiteral("Dismiss an active popup."),
            QStringLiteral("SmartAlarmCli.exe dismiss --uuid UUID [--json]"),
            QStringLiteral("Returns not_active if the popup is not open.")
        },
        {
            QStringLiteral("snooze"),
            QStringLiteral("Snooze an active popup."),
            QStringLiteral("SmartAlarmCli.exe snooze --uuid UUID [--json]"),
            QStringLiteral("Returns not_active if the popup is not open.")
        },
        {
            QStringLiteral("enable-runtime"),
            QStringLiteral("Enable future scheduled notifications."),
            QStringLiteral("SmartAlarmCli.exe enable-runtime [--json]"),
            QStringLiteral("Does not change saved notification enabled flags.")
        },
        {
            QStringLiteral("disable-runtime"),
            QStringLiteral("Disable future scheduled notifications."),
            QStringLiteral("SmartAlarmCli.exe disable-runtime [--json]"),
            QStringLiteral("Does not close already open popups and does not stop currently playing sound.")
        },
        {
            QStringLiteral("reset-interval"),
            QStringLiteral("Reset one interval notification timer."),
            QStringLiteral("SmartAlarmCli.exe reset-interval --uuid UUID [--json]"),
            QStringLiteral("Works only for interval notifications.")
        },
    };
}

QString commandNamesText()
{
    QStringList names;
    for (const auto &entry : helpEntries()) {
        names << entry.command;
    }
    return names.join(QStringLiteral(", "));
}

std::optional<HelpEntry> helpEntryFor(const QString &command)
{
    for (const auto &entry : helpEntries()) {
        if (entry.command == command) {
            return entry;
        }
    }
    return std::nullopt;
}

void printGeneralHelp()
{
    writeStdout(QStringLiteral(
        "Smart Alarm CLI\n\n"
        "Usage:\n"
        "  SmartAlarmCli.exe <command> [options]\n"
        "  SmartAlarmCli.exe help [command]\n\n"
        "Commands:\n"));
    for (const auto &entry : helpEntries()) {
        writeStdout(QStringLiteral("  %1%2%3\n")
            .arg(entry.command,
                QString(entry.command.size() < 16 ? 16 - entry.command.size() : 1, QLatin1Char(' ')),
                entry.summary));
    }
    writeStdout(QStringLiteral(
        "\nGlobal options:\n"
        "  --json             Print machine-readable JSON output.\n\n"
        "Run 'SmartAlarmCli.exe help <command>' for command details.\n"));
}

void printCommandHelp(const HelpEntry &entry)
{
    writeStdout(QStringLiteral("%1\n\nUsage:\n  %2\n\n%3\n")
        .arg(entry.command, entry.usage, entry.details));
}

void printHelpJson(const std::optional<HelpEntry> &entry = std::nullopt)
{
    QJsonObject data;
    if (entry) {
        data.insert(QStringLiteral("command"), entry->command);
        data.insert(QStringLiteral("summary"), entry->summary);
        data.insert(QStringLiteral("usage"), entry->usage);
        data.insert(QStringLiteral("details"), entry->details);
    } else {
        QJsonArray commands;
        for (const auto &item : helpEntries()) {
            QJsonObject object;
            object.insert(QStringLiteral("command"), item.command);
            object.insert(QStringLiteral("summary"), item.summary);
            object.insert(QStringLiteral("usage"), item.usage);
            commands.append(object);
        }
        data.insert(QStringLiteral("usage"), QStringLiteral("SmartAlarmCli.exe <command> [options]"));
        data.insert(QStringLiteral("commands"), commands);
    }

    QJsonObject response;
    response.insert(QStringLiteral("ok"), true);
    response.insert(QStringLiteral("data"), data);
    printJson(response);
}

ParsedArguments parseArguments(const QStringList &arguments)
{
    ParsedArguments result;
    if (arguments.isEmpty()) {
        result.errorMessage = QStringLiteral("CLI command is required");
        return result;
    }
    result.command = arguments.first();
    for (int i = 1; i < arguments.size(); ++i) {
        const auto token = arguments.at(i);
        if (token == QStringLiteral("--json")) {
            result.jsonOutput = true;
            continue;
        }
        if (!token.startsWith(QStringLiteral("--")) || token.size() <= 2) {
            result.errorMessage = QStringLiteral("Unexpected argument: %1").arg(token);
            return result;
        }
        if (i + 1 >= arguments.size()) {
            result.errorMessage = QStringLiteral("Missing value for %1").arg(token);
            return result;
        }
        result.options.insert(token.mid(2), arguments.at(++i));
    }
    result.ok = true;
    return result;
}

int exitCodeForError(const QString &code)
{
    if (code == QStringLiteral("validation_error")) {
        return 1;
    }
    if (code == QStringLiteral("app_not_running")) {
        return 2;
    }
    if (code == QStringLiteral("operation_failed") || code == QStringLiteral("not_found") || code == QStringLiteral("not_active")) {
        return 3;
    }
    return 4;
}

QJsonObject appNotRunningResponse()
{
    QJsonObject error;
    error.insert(QStringLiteral("code"), QStringLiteral("app_not_running"));
    error.insert(QStringLiteral("message"), QStringLiteral("Smart Alarm is not running"));

    QJsonObject response;
    response.insert(QStringLiteral("ok"), false);
    response.insert(QStringLiteral("error"), error);
    return response;
}

void printErrorHuman(const QJsonObject &response)
{
    const auto error = response.value(QStringLiteral("error")).toObject();
    const auto code = error.value(QStringLiteral("code")).toString(QStringLiteral("error"));
    const auto message = error.value(QStringLiteral("message")).toString();
    writeStderr(QStringLiteral("error: %1: %2\n").arg(code, message));
}

QString valueText(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("yes") : QStringLiteral("no");
    }
    return value.toString();
}

void printHuman(const QString &command, const QJsonObject &response)
{
    if (!response.value(QStringLiteral("ok")).toBool()) {
        printErrorHuman(response);
        return;
    }

    const auto data = response.value(QStringLiteral("data")).toObject();
    if (command == QStringLiteral("list")) {
        const auto notifications = data.value(QStringLiteral("notifications")).toArray();
        writeStdout(QStringLiteral("UUID                                  Enabled  Message  Schedule  Next\n"));
        for (const auto value : notifications) {
            const auto item = value.toObject();
            writeStdout(QStringLiteral("%1  %2      %3  %4  %5\n")
                .arg(item.value(QStringLiteral("uuid")).toString(),
                    valueText(item.value(QStringLiteral("enabled"))),
                    item.value(QStringLiteral("message")).toString(),
                    item.value(QStringLiteral("scheduleText")).toString(),
                    item.value(QStringLiteral("nextTriggerAt")).toString(QStringLiteral("-"))));
        }
        return;
    }
    if (command == QStringLiteral("get")) {
        const auto item = data.value(QStringLiteral("notification")).toObject();
        writeStdout(QStringLiteral("uuid: %1\n").arg(item.value(QStringLiteral("uuid")).toString()));
        writeStdout(QStringLiteral("enabled: %1\n").arg(valueText(item.value(QStringLiteral("enabled")))));
        writeStdout(QStringLiteral("message: %1\n").arg(item.value(QStringLiteral("message")).toString()));
        writeStdout(QStringLiteral("schedule: %1\n").arg(item.value(QStringLiteral("scheduleText")).toString()));
        writeStdout(QStringLiteral("next: %1\n").arg(item.value(QStringLiteral("nextTriggerAt")).toString(QStringLiteral("-"))));
        return;
    }
    if (command == QStringLiteral("status")) {
        writeStdout(QStringLiteral("runtimeNotificationsEnabled: %1\n").arg(valueText(data.value(QStringLiteral("runtimeNotificationsEnabled")))));
        writeStdout(QStringLiteral("notificationCount: %1\n").arg(data.value(QStringLiteral("notificationCount")).toInt()));
        writeStdout(QStringLiteral("activePopupCount: %1\n").arg(data.value(QStringLiteral("activePopupCount")).toInt()));
        writeStdout(QStringLiteral("audioPlaying: %1\n").arg(valueText(data.value(QStringLiteral("audioPlaying")))));
        return;
    }
    if (data.contains(QStringLiteral("uuid"))) {
        writeStdout(QStringLiteral("uuid: %1\n").arg(data.value(QStringLiteral("uuid")).toString()));
        return;
    }
    writeStdout(QStringLiteral("ok\n"));
}

} // namespace

int CommandClient::run(const QStringList &arguments)
{
    const bool jsonHelp = arguments.contains(QStringLiteral("--json"));
    if (arguments.isEmpty()
        || arguments.first() == QStringLiteral("help")
        || arguments.first() == QStringLiteral("--help")
        || arguments.first() == QStringLiteral("-h")
        || arguments.contains(QStringLiteral("--help"))
        || arguments.contains(QStringLiteral("-h"))) {
        QString command;
        if (arguments.size() >= 2 && arguments.first() == QStringLiteral("help")) {
            command = arguments.at(1);
        } else if (!arguments.isEmpty() && (arguments.first() == QStringLiteral("--help") || arguments.first() == QStringLiteral("-h"))) {
            if (arguments.size() >= 2) {
                command = arguments.at(1);
            }
        } else if (!arguments.isEmpty()
            && arguments.first() != QStringLiteral("help")
            && arguments.first() != QStringLiteral("--help")
            && arguments.first() != QStringLiteral("-h")) {
            command = arguments.first();
        }
        if (!command.isEmpty() && command != QStringLiteral("--json")) {
            const auto entry = helpEntryFor(command);
            if (!entry) {
                QJsonObject response;
                QJsonObject error;
                error.insert(QStringLiteral("code"), QStringLiteral("validation_error"));
                error.insert(QStringLiteral("message"), QStringLiteral("Unknown CLI command for help: %1").arg(command));
                error.insert(QStringLiteral("commands"), commandNamesText());
                response.insert(QStringLiteral("ok"), false);
                response.insert(QStringLiteral("error"), error);
                jsonHelp ? printJson(response) : printErrorHuman(response);
                return 1;
            }
            jsonHelp ? printHelpJson(entry) : printCommandHelp(*entry);
            return 0;
        }
        jsonHelp ? printHelpJson() : printGeneralHelp();
        return 0;
    }

    const auto parsed = parseArguments(arguments);
    if (!parsed.ok) {
        QJsonObject response;
        QJsonObject error;
        error.insert(QStringLiteral("code"), QStringLiteral("validation_error"));
        error.insert(QStringLiteral("message"), parsed.errorMessage);
        response.insert(QStringLiteral("ok"), false);
        response.insert(QStringLiteral("error"), error);
        printErrorHuman(response);
        return 1;
    }

    QJsonObject request;
    request.insert(QStringLiteral("command"), parsed.command);
    request.insert(QStringLiteral("options"), parsed.options);

    QLocalSocket socket;
    socket.connectToServer(CommandServer::serverName());
    if (!socket.waitForConnected(1000)) {
        const auto response = appNotRunningResponse();
        parsed.jsonOutput ? printJson(response) : printErrorHuman(response);
        return 2;
    }

    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    socket.flush();
    socket.waitForBytesWritten(1000);
    if (!socket.waitForReadyRead(5000)) {
        QJsonObject response;
        QJsonObject error;
        error.insert(QStringLiteral("code"), QStringLiteral("protocol_error"));
        error.insert(QStringLiteral("message"), QStringLiteral("No response from Smart Alarm"));
        response.insert(QStringLiteral("ok"), false);
        response.insert(QStringLiteral("error"), error);
        parsed.jsonOutput ? printJson(response) : printErrorHuman(response);
        return 4;
    }

    QByteArray payload = socket.readAll();
    while (socket.waitForReadyRead(50)) {
        payload += socket.readAll();
    }

    QJsonParseError parseError;
    const auto responseDocument = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDocument.isObject()) {
        QJsonObject response;
        QJsonObject error;
        error.insert(QStringLiteral("code"), QStringLiteral("protocol_error"));
        error.insert(QStringLiteral("message"), QStringLiteral("Invalid response from Smart Alarm"));
        response.insert(QStringLiteral("ok"), false);
        response.insert(QStringLiteral("error"), error);
        parsed.jsonOutput ? printJson(response) : printErrorHuman(response);
        return 4;
    }

    const auto response = responseDocument.object();
    parsed.jsonOutput ? printJson(response) : printHuman(parsed.command, response);
    if (response.value(QStringLiteral("ok")).toBool()) {
        return 0;
    }
    return exitCodeForError(response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString());
}

} // namespace smartalarm
