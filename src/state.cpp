#include "state.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <fcntl.h>
#include <sys/stat.h>

namespace Greeter {
State loadState(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  const auto document = QJsonDocument::fromJson(file.readAll());
  const auto object = document.object();
  if (!document.isObject() || object.value("version").toInt() != 1)
    return {};
  return {object.value("last_user").toString(),
          object.value("last_session").toString()};
}
bool saveState(const QString &path, const State &state, bool manualMode,
               QString *error) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QSaveFile file(path);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly)) {
    if (error)
      *error = file.errorString();
    return false;
  }
  if (::fchmod(file.handle(), S_IRUSR | S_IWUSR) != 0) {
    if (error)
      *error = QStringLiteral("could not set owner-only state permissions");
    file.cancelWriting();
    return false;
  }
  QJsonObject object{{"version", 1}, {"last_session", state.lastSession}};
  if (!manualMode && !state.lastUser.isEmpty())
    object.insert("last_user", state.lastUser);
  if (file.write(QJsonDocument(object).toJson(QJsonDocument::Compact)) < 0 ||
      !file.commit()) {
    if (error)
      *error = file.errorString();
    return false;
  }
  return QFile::setPermissions(path, QFileDevice::ReadOwner |
                                         QFileDevice::WriteOwner);
}

QString selectSession(const State &state, const QString &configuredDefault,
                      const QStringList &validSessions) {
  if (validSessions.contains(state.lastSession))
    return state.lastSession;
  if (validSessions.contains(configuredDefault))
    return configuredDefault;
  return validSessions.value(0);
}
} // namespace Greeter
