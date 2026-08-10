#include "config.h"
#include "controller.h"
#include "desktopentry.h"
#include "greetdclient.h"
#include "state.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <gtest/gtest.h>

namespace {
class FakeTransport final : public Greeter::IGreetdTransport {
  Q_OBJECT
public:
  QList<QJsonObject> sent;
  QString path;
  int cancellations = 0;
  void connectTo(const QString &value) override { path = value; }
  void send(const QJsonObject &message) override { sent += message; }
  void cancel() override { ++cancellations; }
  void disconnectFromServer() override {}
  void connectNow() { emit connected(); }
  void reply(const QJsonObject &value) { emit message(value); }
  void disconnectNow() { emit disconnected(); }
};
class FakeAccounts final : public Greeter::IAccountSource {
public:
  QList<Greeter::User> users(const QStringList &, int, int,
                             const QStringList &) override {
    return {{"alice", "Alice", {}, 1000}};
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
  void capabilitiesNow(bool off, bool reboot, const QString &reason = {}) {
    emit capabilities(off, reboot, reason);
  }
};
class FakeFiles final : public Greeter::IFileSystem {
public:
  QList<Greeter::Session> records{
      {"holo.desktop", "Holo", {"holo", "--start"}}};
  QString savedUser;
  QString savedSession;
  bool savedManual = false;
  QList<Greeter::Session> sessions(const QStringList &, const QStringList &,
                                   const QStringList &) override {
    return records;
  }
  bool save(const QString &, const QString &user, const QString &session,
            bool manual, QString *) override {
    savedUser = user;
    savedSession = session;
    savedManual = manual;
    return true;
  }
};
} // namespace

TEST(Config, MissingUsesDefaultsWithWarning) {
  const auto result = Greeter::loadConfig("/definitely/missing/greeter.toml");
  EXPECT_TRUE(result.valid());
  EXPECT_FALSE(result.warning.isEmpty());
  EXPECT_EQ(result.value.minUid, 1000);
}
TEST(Config, RejectsUnsafeValues) {
  QTemporaryDir temporary;
  QFile file(temporary.filePath("greeter.toml"));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("version=1\n[appearance]\nbackground='../secret'\n");
  file.close();
  EXPECT_FALSE(Greeter::loadConfig(file.fileName()).valid());
}
TEST(Config, RejectsWrongTypesOverflowAndUnknownKeys) {
  QTemporaryDir temporary;
  const auto check = [&](const QByteArray &contents) {
    QFile file(temporary.filePath("greeter.toml"));
    EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(contents);
    file.close();
    EXPECT_FALSE(Greeter::loadConfig(file.fileName()).valid());
  };
  check("version='1'\n");
  check("version=1\n[users]\nmin_uid=999999999999\n");
  check("version=1\n[users]\nshow_avatars='yes'\n");
  check("version=1\nsurprise=true\n");
  check("version=1\n[sessions]\ndirectories=['/usr//share']\n");
}
TEST(DesktopExec, ExpandsSafeCodesWithoutShell) {
  QString error;
  const auto command = Greeter::parseDesktopExec(
      "/usr/bin/session --name \"%c\" %% %f", "Holo Night",
      "/tmp/session.desktop", {}, &error);
  EXPECT_TRUE(error.isEmpty());
  EXPECT_EQ(command,
            QStringList({"/usr/bin/session", "--name", "Holo Night", "%"}));
}
TEST(DesktopExec, RejectsUnknownCodeAndMalformedQuote) {
  QString error;
  EXPECT_TRUE(
      Greeter::parseDesktopExec("session %d", {}, {}, {}, &error).isEmpty());
  EXPECT_FALSE(error.isEmpty());
  error.clear();
  EXPECT_TRUE(Greeter::parseDesktopExec("session \"oops", {}, {}, {}, &error)
                  .isEmpty());
}
TEST(State, RoundTripsAndOmitsManualUser) {
  QTemporaryDir temporary;
  const QString path = temporary.filePath("state.json");
  QString error;
  ASSERT_TRUE(Greeter::saveState(path, {"alice", "holo.desktop"}, true, &error))
      << error.toStdString();
  const auto state = Greeter::loadState(path);
  EXPECT_TRUE(state.lastUser.isEmpty());
  EXPECT_EQ(state.lastSession, "holo.desktop");
  EXPECT_EQ(QFileInfo(path).permissions() &
                (QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                 QFileDevice::ReadOther | QFileDevice::WriteOther),
            0);
}
TEST(State, CorruptionIsNonFatal) {
  QTemporaryDir temporary;
  QFile file(temporary.filePath("state.json"));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("not json");
  file.close();
  const auto state = Greeter::loadState(file.fileName());
  EXPECT_TRUE(state.lastUser.isEmpty());
  EXPECT_TRUE(state.lastSession.isEmpty());
}
TEST(State, SelectsSavedThenDefaultThenFirst) {
  const QStringList sessions{"first.desktop", "default.desktop",
                             "saved.desktop"};
  EXPECT_EQ(Greeter::selectSession({{}, "saved.desktop"}, "default.desktop",
                                   sessions),
            "saved.desktop");
  EXPECT_EQ(
      Greeter::selectSession({{}, "gone.desktop"}, "default.desktop", sessions),
      "default.desktop");
  EXPECT_EQ(Greeter::selectSession({}, "gone.desktop", sessions),
            "first.desktop");
}

TEST(Controller, RunsCompleteAuthenticationAndPersistsAfterStart) {
  FakeTransport transport;
  FakeAccounts accounts;
  FakePower power;
  FakeFiles files;
  Greeter::Config config;
  config.defaultSession = "holo.desktop";
  Greeter::Controller controller(false, {}, config, "/unused", &transport,
                                 &accounts, &power, &files);
  qputenv("GREETD_SOCK", "/tmp/greetd.sock");
  controller.begin("alice");
  EXPECT_EQ(controller.state(), "connecting");
  transport.connectNow();
  ASSERT_EQ(transport.sent.size(), 1);
  EXPECT_EQ(transport.sent[0].value("type"), "create_session");
  EXPECT_EQ(transport.sent[0].value("username"), "alice");
  transport.reply({{"type", "auth_message"},
                   {"auth_message_type", "secret"},
                   {"auth_message", "Password"}});
  EXPECT_TRUE(controller.secret());
  controller.respond("sensitive");
  ASSERT_EQ(transport.sent.size(), 2);
  EXPECT_EQ(transport.sent[1].value("response"), "sensitive");
  transport.reply({{"type", "success"}});
  ASSERT_EQ(transport.sent.size(), 3);
  EXPECT_EQ(transport.sent[2].value("type"), "start_session");
  EXPECT_EQ(transport.sent[2].value("cmd").toArray(),
            QJsonArray({"holo", "--start"}));
  EXPECT_EQ(transport.sent[2].value("env").toArray(),
            QJsonArray({"XDG_SESSION_TYPE=wayland"}));
  EXPECT_TRUE(files.savedUser.isEmpty());
  transport.reply({{"type", "success"}});
  EXPECT_EQ(controller.state(), "authenticated");
  EXPECT_EQ(files.savedUser, "alice");
  EXPECT_EQ(files.savedSession, "holo.desktop");
}

TEST(Controller, HandlesMixedPromptsCancellationAndDisconnectFailClosed) {
  FakeTransport transport;
  FakeAccounts accounts;
  FakePower power;
  FakeFiles files;
  Greeter::Config config;
  config.defaultSession = "holo.desktop";
  Greeter::Controller controller(false, {}, config, "/unused", &transport,
                                 &accounts, &power, &files);
  controller.begin("alice");
  transport.connectNow();
  transport.reply({{"type", "auth_message"},
                   {"auth_message_type", "info"},
                   {"auth_message", "Insert token"}});
  EXPECT_TRUE(transport.sent.last().value("response").isNull());
  transport.reply({{"type", "auth_message"},
                   {"auth_message_type", "visible"},
                   {"auth_message", "Code"}});
  EXPECT_EQ(controller.state(), "input-prompt");
  transport.disconnectNow();
  EXPECT_EQ(controller.state(), "failed");
  controller.cancel();
  EXPECT_EQ(controller.state(), "user-selection");
}

TEST(Controller, GatesPowerOnExactCapability) {
  FakeTransport transport;
  FakeAccounts accounts;
  FakePower power;
  FakeFiles files;
  Greeter::Config config;
  config.defaultSession = "holo.desktop";
  Greeter::Controller controller(false, {}, config, "/unused", &transport,
                                 &accounts, &power, &files);
  EXPECT_EQ(power.queries, 1);
  controller.requestReboot();
  EXPECT_EQ(power.reboots, 0);
  power.capabilitiesNow(false, true);
  controller.requestReboot();
  EXPECT_EQ(power.reboots, 1);
}

TEST(GreetdTransport, AcceptsFragmentedAndCoalescedNativeEndianFrames) {
  const QString socketPath =
      "/tmp/hgr-" + QUuid::createUuid().toString(QUuid::Id128);
  QLocalServer server;
  ASSERT_TRUE(server.listen(socketPath)) << server.errorString().toStdString();
  Greeter::GreetdClient client;
  QSignalSpy connected(&client, &Greeter::IGreetdTransport::connected);
  QSignalSpy messages(&client, &Greeter::IGreetdTransport::message);
  client.connectTo(socketPath);
  ASSERT_TRUE(server.waitForNewConnection(1000));
  ASSERT_TRUE(connected.wait(1000) || connected.count() == 1);
  auto *peer = server.nextPendingConnection();
  ASSERT_NE(peer, nullptr);
  const auto frame = [](const QJsonObject &object) {
    const QByteArray body =
        QJsonDocument(object).toJson(QJsonDocument::Compact);
    const quint32 size = static_cast<quint32>(body.size());
    return QByteArray(reinterpret_cast<const char *>(&size), sizeof(size)) +
           body;
  };
  const QByteArray first = frame({{"type", "success"}});
  const QByteArray second = frame({{"type", "error"}, {"description", "no"}});
  peer->write(first.first(2));
  peer->flush();
  EXPECT_FALSE(messages.wait(30));
  peer->write(first.sliced(2) + second);
  peer->flush();
  ASSERT_TRUE(messages.wait(1000));
  QTRY_COMPARE(messages.count(), 2);
  EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("type"), "success");
  EXPECT_EQ(messages.at(1).at(0).toJsonObject().value("type"), "error");
}

TEST(GreetdTransport, RejectsOversizedAndMalformedFrames) {
  const QString socketPath =
      "/tmp/hgr-" + QUuid::createUuid().toString(QUuid::Id128);
  QLocalServer server;
  ASSERT_TRUE(server.listen(socketPath)) << server.errorString().toStdString();
  Greeter::GreetdClient client;
  QSignalSpy connected(&client, &Greeter::IGreetdTransport::connected);
  QSignalSpy failed(&client, &Greeter::IGreetdTransport::failed);
  client.connectTo(socketPath);
  ASSERT_TRUE(server.waitForNewConnection(1000));
  ASSERT_TRUE(connected.wait(1000) || connected.count() == 1);
  auto *peer = server.nextPendingConnection();
  quint32 size = 1024 * 1024 + 1;
  peer->write(QByteArray(reinterpret_cast<const char *>(&size), sizeof(size)));
  peer->flush();
  ASSERT_TRUE(failed.wait(1000));
  EXPECT_TRUE(failed.at(0).at(0).toString().contains("frame length"));
}

TEST(QmlSmoke, StartsEveryDemoScenarioOffscreen) {
  for (const QString &scenario :
       {"default", "wrong-password", "otp", "fingerprint"}) {
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("QT_QPA_PLATFORM", "offscreen");
    process.setProcessEnvironment(environment);
    process.start(GREETER_EXECUTABLE_PATH,
                  {"--demo", "--demo-scenario", scenario});
    ASSERT_TRUE(process.waitForStarted(2000))
        << process.errorString().toStdString();
    QTest::qWait(150);
    EXPECT_EQ(process.state(), QProcess::Running)
        << process.readAllStandardError().toStdString();
    process.terminate();
    if (!process.waitForFinished(1000)) {
      process.kill();
      process.waitForFinished(1000);
    }
  }
}

#include "core_test.moc"
