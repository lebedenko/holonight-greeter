#pragma once
#include "services.h"

class FakeTransport final : public Greeter::IGreetdTransport {
  Q_OBJECT
 public:
  QList<QJsonObject> sent;
  QString path;
  int cancellations = 0;
  int disconnections = 0;
  bool disconnectSynchronously = false;
  void connectTo(const QString& value) override { path = value; }
  void send(const QJsonObject& message) override { sent += message; }
  void cancel() override { ++cancellations; }
  void disconnectFromServer() override {
    ++disconnections;
    if (disconnectSynchronously) {
      emit disconnected();
    }
  }
  void connectNow() { emit connected(); }
  void reply(const QJsonObject& value) { emit message(value); }
  void disconnectNow() { emit disconnected(); }
};
class FakeAccounts final : public Greeter::IAccountSource {
 public:
  int calls = 0;
  QList<Greeter::User> records{{.username = "alice", .display_name = "Alice", .avatar = {}, .uid = 1000}};
  QStringList include;
  QStringList exclude;
  int minUid = 0;
  int maxUid = 0;
  QList<Greeter::User> users(const QStringList& included, int minimumUid, int maximumUid,
                             const QStringList& excluded) override {
    ++calls;
    include = included;
    exclude = excluded;
    minUid = minimumUid;
    maxUid = maximumUid;
    return records;
  }
};
class FakePower final : public Greeter::IPowerService {
  Q_OBJECT
 public:
  int queries = 0;
  int offs = 0;
  int reboots = 0;
  void queryCapabilities() override { ++queries; }
  void requestPowerOff() override { ++offs; }
  void requestReboot() override { ++reboots; }
  void capabilitiesNow(bool off, bool reboot, bool confirmationRequired = true, const QString& reason = {}) {
    emit capabilities(off, reboot, confirmationRequired, reason);
  }
};
class FakeFiles final : public Greeter::IFileSystem {
 public:
  QList<Greeter::Session> records{
      {.id = "holo.desktop", .name = "Holo", .command = {"holo", "--start"}},
  };
  QString savedUser;
  QString savedSession;
  bool savedManual = false;
  int discoveryCalls = 0;
  int saveCalls = 0;
  QList<Greeter::Session> sessions([[maybe_unused]] const QStringList& directories,
                                   [[maybe_unused]] const QStringList& include,
                                   [[maybe_unused]] const QStringList& exclude) override {
    ++discoveryCalls;
    return records;
  }
  bool save([[maybe_unused]] const QString& path, const QString& user, const QString& session, bool manual,
            [[maybe_unused]] QString* error) override {
    ++saveCalls;
    savedUser = user;
    savedSession = session;
    savedManual = manual;
    return true;
  }
};
