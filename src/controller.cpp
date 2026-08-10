#include "controller.h"
#include "state.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QTimer>
#include <QVariantMap>
#include <pwd.h>
#include <unistd.h>

namespace {
Greeter::User demoUser() {
  const passwd *entry = getpwuid(getuid());
  const QString username =
      entry ? QString::fromLocal8Bit(entry->pw_name) : QStringLiteral("demo");
  QString displayName =
      entry ? QString::fromLocal8Bit(entry->pw_gecos).section(',', 0, 0)
            : QString{};
  if (displayName.isEmpty()) {
    displayName = username;
    if (!displayName.isEmpty())
      displayName[0] = displayName[0].toUpper();
  }
  const QString home =
      entry ? QString::fromLocal8Bit(entry->pw_dir) : QString{};
  const QString accountIcon =
      QStringLiteral("/var/lib/AccountsService/icons/") + username;
  const QString face = home + QStringLiteral("/.face");
  const QString avatar = QFileInfo(accountIcon).isReadable() ? accountIcon
                         : QFileInfo(face).isReadable()      ? face
                                                             : QString{};
  return {username, displayName, avatar,
          entry ? static_cast<uint>(entry->pw_uid) : 1000U};
}
} // namespace

namespace Greeter {
Controller::Controller(bool demo, QString scenario, Config config,
                       QString statePath, IGreetdTransport *transport,
                       IAccountSource *accounts, IPowerService *power,
                       IFileSystem *files, QObject *parent)
    : QObject(parent), demo_(demo), scenario_(std::move(scenario)),
      config_(std::move(config)), statePath_(std::move(statePath)),
      transport_(transport), power_(power), files_(files) {
  if (demo_) {
    config_.userMode = Config::UserMode::List;
    userRecords_ = accounts->users(config_.includeUsers, config_.minUid,
                                   config_.maxUid, config_.excludeUsers);
    if (userRecords_.isEmpty())
      userRecords_ = {demoUser()};
    sessionRecords_ =
        files_->sessions(config_.sessionDirectories, config_.includeSessions,
                         config_.excludeSessions);
    if (sessionRecords_.isEmpty())
      sessionRecords_ = {{QStringLiteral("demo.desktop"),
                          QStringLiteral("HoloNight (Demo fallback)"),
                          {QStringLiteral("/bin/true")}}};
  } else if (!manualMode()) {
    userRecords_ = accounts->users(config_.includeUsers, config_.minUid,
                                   config_.maxUid, config_.excludeUsers);
  }
  if (!demo_)
    sessionRecords_ =
        files_->sessions(config_.sessionDirectories, config_.includeSessions,
                         config_.excludeSessions);
  QStringList ids;
  for (const auto &session : sessionRecords_)
    ids += session.id;
  selectedSession_ = selectSession(demo_ ? State{} : loadState(statePath_),
                                   config_.defaultSession, ids);

  connect(transport_, &IGreetdTransport::connected, this, [this] {
    if (stage_ != Stage::Connecting)
      return fail(QStringLiteral("Unexpected greetd connection"));
    stage_ = Stage::Authenticating;
    setState("waiting");
    transport_->send({{"type", "create_session"}, {"username", activeUser_}});
  });
  connect(transport_, &IGreetdTransport::message, this, &Controller::handle);
  connect(transport_, &IGreetdTransport::failed, this,
          [this](const QString &reason) { fail(reason); });
  connect(transport_, &IGreetdTransport::disconnected, this, [this] {
    if (stage_ != Stage::Idle && stage_ != Stage::Complete &&
        stage_ != Stage::Failed)
      fail(QStringLiteral("greetd disconnected"));
  });
  connect(power_, &IPowerService::capabilities, this,
          [this](bool off, bool reboot, const QString &reason) {
            canPowerOff_ = off;
            canReboot_ = reboot;
            if (!reason.isEmpty())
              status_ = reason;
            emit changed();
          });
  connect(power_, &IPowerService::completed, this,
          [this](const QString &error) {
            if (!error.isEmpty())
              setState(state_, error);
          });
  if (!demo_)
    power_->queryCapabilities();
}

QVariantList Controller::users() const {
  QVariantList values;
  for (const auto &user : userRecords_)
    values += QVariantMap{{"username", user.username},
                          {"displayName", user.displayName},
                          {"avatar", user.avatar}};
  return values;
}
QVariantList Controller::sessions() const {
  QVariantList values;
  for (const auto &session : sessionRecords_)
    values += QVariantMap{{"id", session.id}, {"name", session.name}};
  return values;
}
QString Controller::selectedSessionName() const {
  const Session *session = selected();
  return session ? session->name : QString{};
}
void Controller::setSelectedSession(const QString &id) {
  for (const auto &session : sessionRecords_)
    if (session.id == id) {
      selectedSession_ = id;
      emit changed();
      return;
    }
}
const Session *Controller::selected() const {
  for (const auto &session : sessionRecords_)
    if (session.id == selectedSession_)
      return &session;
  return nullptr;
}
void Controller::begin(const QString &user) {
  const QString candidate = user.trimmed();
  if (demo_ && !activeUser_.isEmpty() && activeUser_ != candidate)
    cancel();
  if (stage_ != Stage::Idle && stage_ != Stage::Failed)
    return;
  if (candidate.isEmpty() || !selected())
    return setState("user-selection",
                    selected() ? QStringLiteral("Choose a user")
                               : QStringLiteral("No sessions available"));
  if (!manualMode()) {
    bool known = false;
    for (const auto &record : userRecords_)
      known |= record.username == candidate;
    if (!known)
      return setState("user-selection",
                      QStringLiteral("Choose an available user"));
  }
  activeUser_ = candidate;
  demoStep_ = 0;
  prompt_.clear();
  secret_ = false;
  if (demo_) {
    const quint64 attempt = ++demoAttempt_;
    prompt_ = scenario_ == "fingerprint" ? "Touch the fingerprint sensor"
                                         : "Password";
    secret_ = scenario_ != "fingerprint";
    stage_ = Stage::Authenticating;
    setState(secret_ ? "input-prompt" : "informational-prompt");
    if (scenario_ == "fingerprint")
      QTimer::singleShot(750, this, [this, attempt] {
        if (demo_ && stage_ == Stage::Authenticating &&
            scenario_ == "fingerprint" && demoAttempt_ == attempt) {
          stage_ = Stage::Complete;
          prompt_.clear();
          setState("authenticated",
                   "Authenticated — demo does not start a session");
        }
      });
    return;
  }
  stage_ = Stage::Connecting;
  setState("connecting");
  transport_->connectTo(QString::fromLocal8Bit(qgetenv("GREETD_SOCK")));
}
void Controller::respond(const QString &response) {
  if (stage_ != Stage::Authenticating || state_ != "input-prompt")
    return;
  QByteArray bytes = response.toUtf8();
  if (demo_) {
    if (scenario_ == "wrong-password")
      fail(QStringLiteral("Authentication failed"));
    else if (scenario_ == "otp" && demoStep_++ == 0) {
      bytes.fill('\0');
      prompt_ = QStringLiteral("One-time code");
      secret_ = false;
      setState("input-prompt");
      return;
    } else {
      stage_ = Stage::Complete;
      setState("authenticated",
               "Authenticated — demo does not start a session");
    }
  } else {
    transport_->send({{"type", "post_auth_message_response"},
                      {"response", QString::fromUtf8(bytes)}});
    setState("waiting");
  }
  bytes.fill('\0');
}
void Controller::cancel() {
  if (stage_ == Stage::Idle)
    return;
  if (!demo_ && stage_ != Stage::Connecting)
    transport_->cancel();
  if (!demo_)
    transport_->disconnectFromServer();
  if (demo_)
    ++demoAttempt_;
  activeUser_.clear();
  prompt_.clear();
  secret_ = false;
  stage_ = Stage::Idle;
  setState("user-selection");
}
void Controller::handle(const QJsonObject &message) {
  const QString type = message.value("type").toString();
  if (stage_ == Stage::Authenticating && type == "auth_message") {
    const QString kind = message.value("auth_message_type").toString();
    const auto promptValue = message.value("auth_message");
    if (!promptValue.isString() ||
        !QStringList{"secret", "visible", "info", "error"}.contains(kind))
      return fail(QStringLiteral("Malformed greetd authentication prompt"));
    prompt_ = promptValue.toString();
    secret_ = kind == "secret";
    setState(kind == "info" || kind == "error" ? "informational-prompt"
                                               : "input-prompt",
             kind == "error" ? prompt_ : QString{});
    if (kind == "info" || kind == "error")
      transport_->send({{"type", "post_auth_message_response"},
                        {"response", QJsonValue::Null}});
    return;
  }
  if (type == "success" && stage_ == Stage::Authenticating) {
    const Session *session = selected();
    if (!session)
      return fail(QStringLiteral("Selected session is unavailable"));
    stage_ = Stage::Starting;
    QJsonArray command;
    for (const auto &argument : session->command)
      command += argument;
    transport_->send({{"type", "start_session"},
                      {"cmd", command},
                      {"env", QJsonArray{"XDG_SESSION_TYPE=wayland"}}});
    return setState("starting");
  }
  if (type == "success" && stage_ == Stage::Starting) {
    QString error;
    if (!files_->save(statePath_, activeUser_, selectedSession_, manualMode(),
                      &error))
      return fail(
          QStringLiteral("Session started but state could not be saved: %1")
              .arg(error));
    stage_ = Stage::Complete;
    transport_->disconnectFromServer();
    return setState("authenticated");
  }
  if (type == "error" && stage_ != Stage::Idle) {
    const auto description = message.value("description");
    return fail(description.isString() && !description.toString().isEmpty()
                    ? description.toString()
                    : QStringLiteral("greetd rejected the request"));
  }
  fail(QStringLiteral("Unexpected greetd reply"));
}
void Controller::requestPowerOff() {
  if (demo_)
    return setState(state_, QStringLiteral("Shutdown simulated in demo mode"));
  if (canPowerOff_ && stage_ == Stage::Idle)
    power_->requestPowerOff();
}
void Controller::requestReboot() {
  if (demo_)
    return setState(state_, QStringLiteral("Reboot simulated in demo mode"));
  if (canReboot_ && stage_ == Stage::Idle)
    power_->requestReboot();
}
void Controller::fail(const QString &reason) {
  if (stage_ == Stage::Failed)
    return;
  stage_ = Stage::Failed;
  prompt_.clear();
  secret_ = false;
  if (!demo_)
    transport_->disconnectFromServer();
  setState("failed", reason);
}
void Controller::setState(QString state, QString status) {
  state_ = std::move(state);
  status_ = std::move(status);
  emit changed();
}
} // namespace Greeter
