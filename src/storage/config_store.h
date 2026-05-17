#pragma once

#include "core/notification.h"

#include <QJsonObject>
#include <QStringList>

namespace smartalarm {

enum class LoadStatus {
    MissingDefaults,
    Loaded,
    CorruptedBackedUp,
    WrongRootBackedUp,
    LoadedWithRepairs
};

struct LoadResult {
    AppConfig config;
    LoadStatus status = LoadStatus::MissingDefaults;
    QString backupPath;
    QStringList warnings;
};

struct SaveResult {
    bool ok = false;
    QString errorMessage;
};

class ConfigStore {
public:
    explicit ConfigStore(QString filePath = {});

    [[nodiscard]] QString filePath() const;
    LoadResult load();
    SaveResult save(const AppConfig &config) const;

    static QJsonObject toJson(const AppConfig &config);
    static std::optional<AppConfig> fromJson(const QJsonObject &root, bool *repaired, QStringList *warnings);

private:
    QString m_filePath;

    QString defaultFilePath() const;
    QString makeBackupPath() const;
    bool backupExisting(QString *backupPath) const;
};

} // namespace smartalarm
