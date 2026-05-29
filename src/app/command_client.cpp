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

void printJson(const QJsonObject &response)
{
    writeStdout(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)) + QLatin1Char('\n'));
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
