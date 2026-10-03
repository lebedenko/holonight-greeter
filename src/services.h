#pragma once

#include "desktopentry.h"

#include <QDateTime>
#include <QJsonObject>
#include <QObject>

namespace Greeter {
struct User {
  QString username;
  QString display_name;
  QString avatar;
  uint uid = 0;
};

class IGreetdTransport : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;
  virtual void connectTo(const QString& path) = 0;
  virtual void send(const QJsonObject& message) = 0;
  virtual void cancel() = 0;
  virtual void disconnectFromServer() = 0;
 signals:
  void connected();
  void message(const QJsonObject& message);
  void failed(const QString& reason);
  void disconnected();
};

class IAccountSource {
 public:
  IAccountSource() = default;
  virtual ~IAccountSource() = default;
  IAccountSource(const IAccountSource&) = delete;
  IAccountSource& operator=(const IAccountSource&) = delete;
  IAccountSource(IAccountSource&&) = delete;
  IAccountSource& operator=(IAccountSource&&) = delete;
  virtual QList<User> users(const QStringList& include, int min_uid, int max_uid, const QStringList& exclude) = 0;
};

class IPowerService : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;
  virtual void queryCapabilities() = 0;
  virtual void requestPowerOff() = 0;
  virtual void requestReboot() = 0;
 signals:
  void capabilities(bool canPowerOff, bool canReboot, bool powerConfirmationRequired, const QString& reason);
  void completed(const QString& error);
};

class IClock {
 public:
  IClock() = default;
  virtual ~IClock() = default;
  IClock(const IClock&) = delete;
  IClock& operator=(const IClock&) = delete;
  IClock(IClock&&) = delete;
  IClock& operator=(IClock&&) = delete;
  [[nodiscard]] virtual QDateTime now() const = 0;
};

class IFileSystem {
 public:
  IFileSystem() = default;
  virtual ~IFileSystem() = default;
  IFileSystem(const IFileSystem&) = delete;
  IFileSystem& operator=(const IFileSystem&) = delete;
  IFileSystem(IFileSystem&&) = delete;
  IFileSystem& operator=(IFileSystem&&) = delete;
  virtual QList<Session> sessions(const QStringList& directories, const QStringList& include,
                                  const QStringList& exclude) = 0;
  virtual bool save(const QString& path, const QString& user, const QString& session, bool manual, QString* error) = 0;
};

class SystemAccountSource final : public IAccountSource {
 public:
  QList<User> users(const QStringList& include, int min_uid, int max_uid, const QStringList& exclude) override;
};

class LogindPowerService final : public IPowerService {
  Q_OBJECT
 public:
  explicit LogindPowerService(QObject* parent = nullptr);
 public slots:
  void queryCapabilities() override;

 public:
  void requestPowerOff() override;
  void requestReboot() override;

 private:
  void request(const QString& method);
  static bool queryPowerConfirmationRequired(QString* error);
};

class SystemClock final : public IClock {
 public:
  [[nodiscard]] QDateTime now() const override { return QDateTime::currentDateTime(); }
};

class SystemFileSystem final : public IFileSystem {
 public:
  QList<Session> sessions(const QStringList& directories, const QStringList& include,
                          const QStringList& exclude) override;
  bool save(const QString& path, const QString& user, const QString& session, bool manual, QString* error) override;
};
}  // namespace Greeter
