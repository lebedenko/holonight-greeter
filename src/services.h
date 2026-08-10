#pragma once

#include "desktopentry.h"
#include <QDateTime>
#include <QJsonObject>
#include <QObject>

namespace Greeter {
struct User {
  QString username;
  QString displayName;
  QString avatar;
  uint uid = 0;
};

class IGreetdTransport : public QObject {
  Q_OBJECT
public:
  using QObject::QObject;
  virtual void connectTo(const QString &path) = 0;
  virtual void send(const QJsonObject &message) = 0;
  virtual void cancel() = 0;
  virtual void disconnectFromServer() = 0;
signals:
  void connected();
  void message(const QJsonObject &message);
  void failed(const QString &reason);
  void disconnected();
};

class IAccountSource {
public:
  virtual ~IAccountSource() = default;
  virtual QList<User> users(const QStringList &include, int minUid, int maxUid,
                            const QStringList &exclude) = 0;
};

class IPowerService : public QObject {
  Q_OBJECT
public:
  using QObject::QObject;
  virtual void queryCapabilities() = 0;
  virtual void requestPowerOff() = 0;
  virtual void requestReboot() = 0;
signals:
  void capabilities(bool canPowerOff, bool canReboot, const QString &reason);
  void completed(const QString &error);
};

class IClock {
public:
  virtual ~IClock() = default;
  virtual QDateTime now() const = 0;
};

class IFileSystem {
public:
  virtual ~IFileSystem() = default;
  virtual QList<Session> sessions(const QStringList &directories,
                                  const QStringList &include,
                                  const QStringList &exclude) = 0;
  virtual bool save(const QString &path, const QString &user,
                    const QString &session, bool manual, QString *error) = 0;
};

class SystemAccountSource final : public IAccountSource {
public:
  QList<User> users(const QStringList &include, int minUid, int maxUid,
                    const QStringList &exclude) override;
};

class LogindPowerService final : public IPowerService {
  Q_OBJECT
public:
  using IPowerService::IPowerService;
  void queryCapabilities() override;
  void requestPowerOff() override;
  void requestReboot() override;

private:
  void request(const QString &method);
};

class SystemClock final : public IClock {
public:
  QDateTime now() const override { return QDateTime::currentDateTime(); }
};

class SystemFileSystem final : public IFileSystem {
public:
  QList<Session> sessions(const QStringList &directories,
                          const QStringList &include,
                          const QStringList &exclude) override;
  bool save(const QString &path, const QString &user, const QString &session,
            bool manual, QString *error) override;
};
} // namespace Greeter
