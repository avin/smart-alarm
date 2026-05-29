#include "app/command_client.h"

#include <QCoreApplication>

using namespace smartalarm;

namespace {

void applyApplicationMetadata()
{
    QCoreApplication::setOrganizationName(QStringLiteral("SmartAlarm"));
    QCoreApplication::setApplicationName(QStringLiteral("SmartAlarmCli"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    applyApplicationMetadata();

    auto arguments = app.arguments().mid(1);
    if (!arguments.isEmpty() && arguments.first() == QStringLiteral("cli")) {
        arguments.removeFirst();
    }
    return CommandClient::run(arguments);
}
