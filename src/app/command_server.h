#pragma once

#include <QObject>

class QLocalServer;
class QLocalSocket;
class QJsonObject;

namespace smartalarm {

class AppController;

class CommandServer : public QObject {
    Q_OBJECT
public:
    explicit CommandServer(AppController *controller, QObject *parent = nullptr);

    bool start();
    static QString serverName();

private:
    void handleConnection();
    void handleRequest(QLocalSocket *socket);
    QJsonObject dispatch(const QJsonObject &request) const;

    AppController *m_controller = nullptr;
    QLocalServer *m_server = nullptr;
};

} // namespace smartalarm
