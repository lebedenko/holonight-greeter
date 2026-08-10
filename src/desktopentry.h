#pragma once
#include <QString>
#include <QStringList>

namespace Greeter {
struct Session {
  QString id;
  QString name;
  QStringList command;
};
[[nodiscard]] QStringList
parseDesktopExec(const QString &exec, const QString &name, const QString &file,
                 const QString &icon, QString *error = nullptr);
[[nodiscard]] QList<Session> discoverSessions(const QStringList &directories,
                                              const QStringList &include,
                                              const QStringList &exclude);
} // namespace Greeter
