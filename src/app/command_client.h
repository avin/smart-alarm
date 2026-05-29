#pragma once

#include <QStringList>

namespace smartalarm {

class CommandClient {
public:
    static int run(const QStringList &arguments);
};

} // namespace smartalarm
