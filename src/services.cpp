#include "services.h"
#include "state.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <algorithm>
#include <pwd.h>
#include <shadow.h>

namespace {
bool locked(const QString &username) {
  const QByteArray name = username.toLocal8Bit();
  if (const spwd *shadow = getspnam(name.constData())) {
    const QByteArray password(shadow->sp_pwdp ? shadow->sp_pwdp : "");
    return password.isEmpty() || password.startsWith('!') ||
           password.startsWith('*');
  }
  return false;
}

std::optional<Greeter::User> resolve(const QString &username) {
  const QByteArray name = username.toLocal8Bit();
  passwd storage{};
  passwd *entry = nullptr;
  QByteArray buffer(16384, Qt::Uninitialized);
  if (getpwnam_r(name.constData(), &storage, buffer.data(), buffer.size(),
                 &entry) != 0 ||
      !entry)
    return std::nullopt;
  const QString gecos =
      QString::fromLocal8Bit(entry->pw_gecos).section(',', 0, 0);
  return Greeter::User{
      username, gecos.isEmpty() ? username : gecos, {}, entry->pw_uid};
}
} // namespace

namespace Greeter {
QList<User> SystemAccountSource::users(const QStringList &include, int minUid,
                                       int maxUid, const QStringList &exclude) {
  QList<User> result;
  QSet<QString> names;
  QHash<QString, User> cached;
  QHash<QString, bool> cachedLocked;
  QDBusInterface accounts(QStringLiteral("org.freedesktop.Accounts"),
                          QStringLiteral("/org/freedesktop/Accounts"),
                          QStringLiteral("org.freedesktop.Accounts"),
                          QDBusConnection::systemBus());
  const QDBusReply<QList<QDBusObjectPath>> paths =
      accounts.call(QStringLiteral("ListCachedUsers"));
  if (paths.isValid()) {
    for (const auto &path : paths.value()) {
      QDBusInterface properties(
          QStringLiteral("org.freedesktop.Accounts"), path.path(),
          QStringLiteral("org.freedesktop.DBus.Properties"),
          QDBusConnection::systemBus());
      const QDBusReply<QVariantMap> reply =
          properties.call(QStringLiteral("GetAll"),
                          QStringLiteral("org.freedesktop.Accounts.User"));
      if (!reply.isValid())
        continue;
      const auto values = reply.value();
      const QString name = values.value("UserName").toString();
      if (name.isEmpty())
        continue;
      cached.insert(name, {name,
                           values.value("RealName").toString().isEmpty()
                               ? name
                               : values.value("RealName").toString(),
                           values.value("IconFile").toString(),
                           values.value("Uid").toUInt()});
      cachedLocked.insert(name, values.value("Locked").toBool());
    }
  }
  if (include.isEmpty()) {
    for (auto it = cached.cbegin(); it != cached.cend(); ++it)
      names.insert(it.key());
    if (names.isEmpty()) {
      setpwent();
      while (const passwd *entry = getpwent())
        names.insert(QString::fromLocal8Bit(entry->pw_name));
      endpwent();
    }
  } else {
    for (const auto &name : include)
      names.insert(name);
  }
  for (const auto &name : names) {
    const auto user = cached.contains(name)
                          ? std::optional<User>(cached.value(name))
                          : resolve(name);
    if (!user || user->uid < static_cast<uint>(minUid) ||
        user->uid > static_cast<uint>(maxUid) || exclude.contains(name) ||
        cachedLocked.value(name, false) || locked(name))
      continue;
    result += *user;
  }
  std::stable_sort(
      result.begin(), result.end(), [](const auto &a, const auto &b) {
        const int display =
            QString::localeAwareCompare(a.displayName, b.displayName);
        return display == 0 ? a.username < b.username : display < 0;
      });
  return result;
}

void LogindPowerService::queryCapabilities() {
  auto *interface =
      new QDBusInterface(QStringLiteral("org.freedesktop.login1"),
                         QStringLiteral("/org/freedesktop/login1"),
                         QStringLiteral("org.freedesktop.login1.Manager"),
                         QDBusConnection::systemBus(), this);
  if (!interface->isValid()) {
    emit capabilities(false, false, QStringLiteral("logind unavailable"));
    interface->deleteLater();
    return;
  }
  const auto power = interface->call(QStringLiteral("CanPowerOff"));
  const auto reboot = interface->call(QStringLiteral("CanReboot"));
  if (power.type() == QDBusMessage::ErrorMessage ||
      reboot.type() == QDBusMessage::ErrorMessage) {
    emit capabilities(false, false,
                      power.type() == QDBusMessage::ErrorMessage
                          ? power.errorMessage()
                          : reboot.errorMessage());
    interface->deleteLater();
    return;
  }
  const QString powerValue = power.arguments().value(0).toString();
  const QString rebootValue = reboot.arguments().value(0).toString();
  emit capabilities(powerValue == "yes", rebootValue == "yes",
                    powerValue == "challenge" || rebootValue == "challenge"
                        ? QStringLiteral("Power action requires authorization")
                        : QString{});
  interface->deleteLater();
}

void LogindPowerService::requestPowerOff() { request("PowerOff"); }
void LogindPowerService::requestReboot() { request("Reboot"); }
void LogindPowerService::request(const QString &method) {
  QDBusInterface interface(QStringLiteral("org.freedesktop.login1"),
                           QStringLiteral("/org/freedesktop/login1"),
                           QStringLiteral("org.freedesktop.login1.Manager"),
                           QDBusConnection::systemBus());
  if (!interface.isValid()) {
    emit completed(QStringLiteral("logind unavailable"));
    return;
  }
  const QDBusMessage reply = interface.call(method, false);
  emit completed(reply.type() == QDBusMessage::ErrorMessage
                     ? (reply.errorMessage().isEmpty()
                            ? QStringLiteral("Power request failed")
                            : reply.errorMessage())
                     : QString{});
}

QList<Session> SystemFileSystem::sessions(const QStringList &directories,
                                          const QStringList &include,
                                          const QStringList &exclude) {
  return discoverSessions(directories, include, exclude);
}
bool SystemFileSystem::save(const QString &path, const QString &user,
                            const QString &session, bool manual,
                            QString *error) {
  return saveState(path, {user, session}, manual, error);
}
} // namespace Greeter
