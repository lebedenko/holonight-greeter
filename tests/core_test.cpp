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
  int disconnections = 0;
  void connectTo(const QString &value) override { path = value; }
  void send(const QJsonObject &message) override { sent += message; }
  void cancel() override { ++cancellations; }
  void disconnectFromServer() override { ++disconnections; }
  void connectNow() { emit connected(); }
  void reply(const QJsonObject &value) { emit message(value); }
  void disconnectNow() { emit disconnected(); }
};
class FakeAccounts final : public Greeter::IAccountSource {
public:
  int calls = 0;
  QList<Greeter::User> records{{"alice", "Alice", {}, 1000}};
  QStringList include;
  QStringList exclude;
  int minUid = 0;
  int maxUid = 0;
  QList<Greeter::User> users(const QStringList &included, int minimumUid,
                             int maximumUid,
                             const QStringList &excluded) override {
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
  int discoveryCalls = 0;
  int saveCalls = 0;
  QList<Greeter::Session> sessions(const QStringList &, const QStringList &,
                                   const QStringList &) override {
    ++discoveryCalls;
    return records;
  }
  bool save(const QString &, const QString &user, const QString &session,
            bool manual, QString *) override {
    ++saveCalls;
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

TEST(Demo, DiscoversConfiguredUsersAndSessionsWithoutPrivilegedServices) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records = {{"alice", "Alice", "/avatars/alice.png", 1100},
                      {"bob", "Bob", "/avatars/bob.png", 1200}};
  FakePower power;
  FakeFiles files;
  files.records += {"plasma.desktop", "Plasma", {"startplasma-wayland"}};
  Greeter::Config config;
  config.userMode = Greeter::Config::UserMode::Manual;
  config.minUid = 1100;
  config.maxUid = 1200;
  config.includeUsers = {"alice", "bob"};
  config.excludeUsers = {"guest"};
  Greeter::Controller controller(true, "default", config,
                                 "/definitely/unreadable/state", &transport,
                                 &accounts, &power, &files);
  EXPECT_EQ(accounts.calls, 1);
  EXPECT_EQ(accounts.include, config.includeUsers);
  EXPECT_EQ(accounts.exclude, config.excludeUsers);
  EXPECT_EQ(accounts.minUid, 1100);
  EXPECT_EQ(accounts.maxUid, 1200);
  EXPECT_EQ(files.discoveryCalls, 1);
  EXPECT_EQ(power.queries, 0);
  ASSERT_EQ(controller.users().size(), 2);
  EXPECT_EQ(controller.users()[0].toMap().value("username"), "alice");
  EXPECT_EQ(controller.users()[0].toMap().value("avatar"),
            "/avatars/alice.png");
  EXPECT_EQ(controller.users()[1].toMap().value("username"), "bob");
  EXPECT_EQ(controller.users()[1].toMap().value("avatar"), "/avatars/bob.png");
  ASSERT_EQ(controller.sessions().size(), 2);
  EXPECT_EQ(controller.sessions().first().toMap().value("id"), "holo.desktop");
  EXPECT_EQ(controller.selectedSessionName(), "Holo");
  controller.setSelectedSession("plasma.desktop");
  EXPECT_EQ(controller.selectedSessionName(), "Plasma");
  EXPECT_FALSE(controller.manualMode());
  controller.requestPowerOff();
  controller.requestReboot();
  EXPECT_EQ(power.offs, 0);
  EXPECT_EQ(power.reboots, 0);
  EXPECT_EQ(files.saveCalls, 0);
}

TEST(Demo, FallsBackToProcessAccountWhenDiscoveryIsEmpty) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records.clear();
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "default", {}, "/unused", &transport,
                                 &accounts, &power, &files);
  EXPECT_EQ(accounts.calls, 1);
  ASSERT_EQ(controller.users().size(), 1);
  EXPECT_FALSE(
      controller.users()[0].toMap().value("username").toString().isEmpty());
}

TEST(Demo, SwitchingUsersRestartsTheDeterministicPrompt) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records = {{"alice", "Alice", "/avatars/alice.png", 1000},
                      {"bob", "Bob", "/avatars/bob.png", 1001}};
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "otp", {}, "/unused", &transport,
                                 &accounts, &power, &files);
  controller.begin("alice");
  controller.respond("demo-password");
  EXPECT_EQ(controller.prompt(), "One-time code");
  controller.begin("bob");
  EXPECT_EQ(controller.state(), "input-prompt");
  EXPECT_EQ(controller.prompt(), "Password");
  EXPECT_TRUE(controller.secret());
  controller.respond("demo-password");
  EXPECT_EQ(controller.prompt(), "One-time code");
  controller.respond("123456");
  EXPECT_EQ(controller.state(), "authenticated");
  EXPECT_TRUE(transport.path.isEmpty());
  EXPECT_TRUE(transport.sent.isEmpty());
  EXPECT_EQ(power.queries, 0);
  EXPECT_EQ(files.saveCalls, 0);
}

TEST(Demo, DefaultAuthenticatesWithoutExternalCalls) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records += {"bob", "Bob", "/avatars/bob.png", 1001};
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "default", {}, "/unused", &transport,
                                 &accounts, &power, &files);
  controller.begin(
      controller.users().first().toMap().value("username").toString());
  EXPECT_EQ(controller.state(), "input-prompt");
  EXPECT_EQ(controller.prompt(), "Password");
  EXPECT_TRUE(controller.secret());
  controller.respond("demo-password");
  EXPECT_EQ(controller.state(), "authenticated");
  controller.begin("bob");
  EXPECT_EQ(controller.prompt(), "Password");
  controller.respond("demo-password");
  EXPECT_EQ(controller.state(), "authenticated");
  EXPECT_TRUE(transport.path.isEmpty());
  EXPECT_TRUE(transport.sent.isEmpty());
  EXPECT_EQ(transport.disconnections, 0);
  EXPECT_EQ(files.saveCalls, 0);
}

TEST(Demo, WrongPasswordFailsAndCanRetry) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records += {"bob", "Bob", "/avatars/bob.png", 1001};
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "wrong-password", {}, "/unused",
                                 &transport, &accounts, &power, &files);
  const QString username =
      controller.users().first().toMap().value("username").toString();
  controller.begin(username);
  controller.respond("wrong");
  EXPECT_EQ(controller.state(), "failed");
  EXPECT_EQ(controller.status(), "Authentication failed");
  EXPECT_EQ(transport.disconnections, 0);
  controller.begin(username);
  EXPECT_EQ(controller.state(), "input-prompt");
  EXPECT_TRUE(controller.secret());
  controller.begin("bob");
  EXPECT_EQ(controller.prompt(), "Password");
  controller.respond("wrong");
  EXPECT_EQ(controller.state(), "failed");
}

TEST(Demo, OtpTransitionsFromPasswordToVisibleCode) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records += {"bob", "Bob", "/avatars/bob.png", 1001};
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "otp", {}, "/unused", &transport,
                                 &accounts, &power, &files);
  controller.begin(
      controller.users().first().toMap().value("username").toString());
  EXPECT_EQ(controller.prompt(), "Password");
  EXPECT_TRUE(controller.secret());
  controller.respond("demo-password");
  EXPECT_EQ(controller.state(), "input-prompt");
  EXPECT_EQ(controller.prompt(), "One-time code");
  EXPECT_FALSE(controller.secret());
  controller.respond("123456");
  EXPECT_EQ(controller.state(), "authenticated");
  controller.begin("bob");
  EXPECT_EQ(controller.prompt(), "Password");
  controller.respond("demo-password");
  EXPECT_EQ(controller.prompt(), "One-time code");
  controller.respond("123456");
  EXPECT_EQ(controller.state(), "authenticated");
}

TEST(Demo, FingerprintCompletesAfterInformationalPrompt) {
  FakeTransport transport;
  FakeAccounts accounts;
  accounts.records += {"bob", "Bob", "/avatars/bob.png", 1001};
  FakePower power;
  FakeFiles files;
  Greeter::Controller controller(true, "fingerprint", {}, "/unused", &transport,
                                 &accounts, &power, &files);
  QSignalSpy changed(&controller, &Greeter::Controller::changed);
  controller.begin(
      controller.users().first().toMap().value("username").toString());
  EXPECT_EQ(controller.state(), "informational-prompt");
  EXPECT_EQ(controller.prompt(), "Touch the fingerprint sensor");
  EXPECT_FALSE(controller.secret());
  QTRY_COMPARE_WITH_TIMEOUT(controller.state(), QString("authenticated"), 1500);
  controller.begin("bob");
  EXPECT_EQ(controller.state(), "informational-prompt");
  QTRY_COMPARE_WITH_TIMEOUT(controller.state(), QString("authenticated"), 1500);
  EXPECT_GT(changed.count(), 1);
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

TEST(DemoCli, StartsDocumentedCommandsOffscreen) {
  const QList<QStringList> arguments{{"--demo"},
                                     {"--demo-scenario", "wrong-password"},
                                     {"--demo-scenario", "otp"},
                                     {"--demo-scenario", "fingerprint"}};
  for (const auto &commandArguments : arguments) {
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("QT_QPA_PLATFORM", "offscreen");
    process.setProcessEnvironment(environment);
    process.start(GREETER_EXECUTABLE_PATH, commandArguments);
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
