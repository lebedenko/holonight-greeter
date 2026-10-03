#include "controller.h"

#include "state.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QTimer>
#include <QVariantMap>

#include <pwd.h>
#include <unistd.h>
#include <utility>

namespace {
Q_LOGGING_CATEGORY(greeterController, "holonight.greeter.controller")

Greeter::User demoUser() {
  const passwd* entry = getpwuid(getuid());
  const QString username = (entry != nullptr) ? QString::fromLocal8Bit(entry->pw_name) : QStringLiteral("demo");
  QString displayName = (entry != nullptr) ? QString::fromLocal8Bit(entry->pw_gecos).section(',', 0, 0) : QString{};
  if (displayName.isEmpty()) {
    displayName = username;
    if (!displayName.isEmpty()) {
      displayName[0] = displayName[0].toUpper();
    }
  }
  const QString home = (entry != nullptr) ? QString::fromLocal8Bit(entry->pw_dir) : QString{};
  const QString accountIcon = QStringLiteral("/var/lib/AccountsService/icons/") + username;
  const QString face = home + QStringLiteral("/.face");
  QString avatar;
  if (QFileInfo(accountIcon).isReadable()) {
    avatar = accountIcon;
  } else if (QFileInfo(face).isReadable()) {
    avatar = face;
  }
  return {
      .username = username,
      .display_name = displayName,
      .avatar = avatar,
      .uid = entry != nullptr ? static_cast<uint>(entry->pw_uid) : 1000U,
  };
}
}  // namespace

namespace Greeter {
const char* Controller::stageName(Stage stage) {
  switch (stage) {
    case Stage::Idle:
      return "idle";
    case Stage::Connecting:
      return "connecting";
    case Stage::Authenticating:
      return "authenticating";
    case Stage::Cancelling:
      return "cancelling";
    case Stage::Starting:
      return "starting";
    case Stage::Complete:
      return "complete";
    case Stage::Failed:
      return "failed";
  }
  return "unknown";
}
Controller::Controller(bool demo, QString scenario, Config config, QString statePath, IGreetdTransport* transport,
                       IAccountSource* accounts, IPowerService* power, IFileSystem* files, QObject* parent)
    : QObject(parent),
      demo_(demo),
      scenario_(std::move(scenario)),
      config_(std::move(config)),
      statePath_(std::move(statePath)),
      transport_(transport),
      power_(power),
      files_(files) {
  initializeRecords(accounts);
  connect(transport_, &IGreetdTransport::connected, this, [this] {
    if (stage_ != Stage::Connecting) {
      {
        fail(QStringLiteral("Unexpected greetd connection"));
      }
      return;
    }
    stage_ = Stage::Authenticating;
    setState("waiting");
    transport_->send({{"type", "create_session"}, {"username", activeUser_}});
  });
  connect(transport_, &IGreetdTransport::message, this, &Controller::handle);
  connect(transport_, &IGreetdTransport::failed, this, [this](const QString& reason) { fail(reason); });
  connect(transport_, &IGreetdTransport::disconnected, this, [this] {
    if (stage_ != Stage::Idle && stage_ != Stage::Complete && stage_ != Stage::Failed) {
      fail(QStringLiteral("greetd disconnected"));
    }
  });
  connect(power_, &IPowerService::capabilities, this,
          [this](bool off, bool reboot, bool confirmationRequired, const QString& reason) {
            canPowerOff_ = off;
            canReboot_ = reboot;
            powerConfirmationRequired_ = confirmationRequired;
            if (!reason.isEmpty()) {
              status_ = reason;
            }
            emit changed();
          });
  connect(power_, &IPowerService::completed, this, [this](const QString& error) {
    if (!error.isEmpty()) {
      setState(state_, error);
    }
  });
  if (!demo_) {
    power_->queryCapabilities();
  }
}

void Controller::initializeRecords(IAccountSource* accounts) {
  if (demo_) {
    config_.user_mode = Config::UserMode::List;
    userRecords_ = accounts->users(config_.include_users, config_.min_uid, config_.max_uid, config_.exclude_users);
    if (userRecords_.isEmpty()) {
      userRecords_ = {demoUser()};
    }
    sessionRecords_ = files_->sessions(config_.session_directories, config_.include_sessions, config_.exclude_sessions);
    if (sessionRecords_.isEmpty()) {
      sessionRecords_ = {
          {
              .id = QStringLiteral("demo.desktop"),
              .name = QStringLiteral("HoloNight (Demo fallback)"),
              .command = {QStringLiteral("/bin/true")},
          },
      };
    }
  } else if (!manualMode()) {
    userRecords_ = accounts->users(config_.include_users, config_.min_uid, config_.max_uid, config_.exclude_users);
  }
  if (!demo_) {
    sessionRecords_ = files_->sessions(config_.session_directories, config_.include_sessions, config_.exclude_sessions);
  }
  const State savedState = demo_ ? State{} : loadState(statePath_);
  if (!manualMode() && !userRecords_.isEmpty()) {
    initialUser_ = userRecords_.first().username;
    if (!demo_ && !savedState.last_user.isEmpty()) {
      for (const auto& user : userRecords_) {
        if (user.username == savedState.last_user) {
          initialUser_ = savedState.last_user;
          break;
        }
      }
    }
  }
  QStringList ids;
  for (const auto& session : sessionRecords_) {
    ids += session.id;
  }
  selectedSession_ = selectSession(savedState, config_.default_session, ids);
}

QVariantList Controller::users() const {
  QVariantList values;
  for (const auto& user : userRecords_) {
    values += QVariantMap{
        {"username", user.username},
        {"displayName", user.display_name},
        {"avatar", user.avatar},
    };
  }
  return values;
}
QVariantList Controller::sessions() const {
  QVariantList values;
  for (const auto& session : sessionRecords_) {
    values += QVariantMap{{"id", session.id}, {"name", session.name}};
  }
  return values;
}
QString Controller::selectedSessionName() const {
  const Session* session = selected();
  return (session != nullptr) ? session->name : QString{};
}
void Controller::setSelectedSession(const QString& identifier) {
  for (const auto& session : sessionRecords_) {
    if (session.id == identifier) {
      selectedSession_ = identifier;
      emit changed();
      return;
    }
  }
}
const Session* Controller::selected() const {
  for (const auto& session : sessionRecords_) {
    if (session.id == selectedSession_) {
      return &session;
    }
  }
  return nullptr;
}
void Controller::begin(const QString& user) {
  const QString candidate = user.trimmed();
  if (demo_ && !activeUser_.isEmpty() && activeUser_ != candidate) {
    cancel();
  }
  if (!demo_ && stage_ == Stage::Cancelling) {
    pendingUser_ = candidate;
    return;
  }
  if (!demo_ && stage_ == Stage::Connecting && !candidate.isEmpty() && candidate != activeUser_) {
    activeUser_.clear();
    stage_ = Stage::Idle;
    setState("user-selection");
    transport_->disconnectFromServer();
    begin(candidate);
    return;
  }
  if (!demo_ && stage_ != Stage::Idle && stage_ != Stage::Failed) {
    if (!candidate.isEmpty() && candidate != activeUser_) {
      pendingUser_ = candidate;
      beginCancellation();
    }
    return;
  }
  if (stage_ != Stage::Idle && stage_ != Stage::Failed) {
    return;
  }
  if (candidate.isEmpty() || (selected() == nullptr)) {
    {
      setState("user-selection",
               (selected() != nullptr) ? QStringLiteral("Choose a user") : QStringLiteral("No sessions available"));
    }
    return;
  }
  if (!manualMode()) {
    bool known = false;
    for (const auto& record : userRecords_) {
      known |= record.username == candidate;
    }
    if (!known) {
      {
        setState("user-selection", QStringLiteral("Choose an available user"));
      }
      return;
    }
  }
  activeUser_ = candidate;
  authenticationError_.clear();
  demoStep_ = 0;
  prompt_.clear();
  secret_ = false;
  if (demo_) {
    beginDemoAuthentication();
    return;
  }
  stage_ = Stage::Connecting;
  setState("connecting");
  transport_->connectTo(QString::fromLocal8Bit(qgetenv("GREETD_SOCK")));
}
void Controller::beginDemoAuthentication() {
  const quint64 attempt = ++demoAttempt_;
  prompt_ = scenario_ == "fingerprint" ? "Touch the fingerprint sensor" : "Password";
  secret_ = scenario_ != "fingerprint";
  stage_ = Stage::Authenticating;
  setState(secret_ ? "input-prompt" : "informational-prompt");
  if (scenario_ == "fingerprint") {
    QTimer::singleShot(750, this, [this, attempt] {
      if (demo_ && stage_ == Stage::Authenticating && scenario_ == "fingerprint" && demoAttempt_ == attempt) {
        stage_ = Stage::Complete;
        prompt_.clear();
        setState("authenticated", "Authenticated — demo does not start a session");
      }
    });
  }
}

void Controller::respond(const QString& response) {
  if (stage_ != Stage::Authenticating || state_ != "input-prompt") {
    return;
  }
  QByteArray bytes = response.toUtf8();
  if (demo_) {
    if (scenario_ == "wrong-password") {
      bytes.fill('\0');
      prompt_ = QStringLiteral("Password");
      secret_ = true;
      setState("input-prompt", QStringLiteral("Authentication failed"));
      return;
    }
    if (scenario_ == "otp" && demoStep_++ == 0) {
      bytes.fill('\0');
      prompt_ = QStringLiteral("One-time code");
      secret_ = false;
      setState("input-prompt");
      return;
    }
    stage_ = Stage::Complete;
    setState("authenticated", "Authenticated — demo does not start a session");
  } else {
    transport_->send({
        {"type", "post_auth_message_response"},
        {"response", QString::fromUtf8(bytes)},
    });
    setState("waiting");
  }
  bytes.fill('\0');
}
void Controller::cancel() {
  if (stage_ == Stage::Idle) {
    return;
  }
  pendingUser_.clear();
  if (!demo_ && stage_ == Stage::Connecting) {
    transport_->disconnectFromServer();
  } else if (!demo_ && stage_ != Stage::Failed) {
    beginCancellation();
    return;
  }
  if (demo_) {
    ++demoAttempt_;
  }
  activeUser_.clear();
  prompt_.clear();
  secret_ = false;
  stage_ = Stage::Idle;
  setState("user-selection");
}
void Controller::restartAuthentication() {
  const QString user = activeUser_;
  if (user.isEmpty()) {
    return;
  }
  if (demo_) {
    cancel();
    begin(user);
    return;
  }
  if (stage_ == Stage::Connecting) {
    transport_->disconnectFromServer();
    activeUser_.clear();
    stage_ = Stage::Idle;
    setState("user-selection");
    begin(user);
    return;
  }
  if (stage_ == Stage::Authenticating) {
    pendingUser_ = user;
    beginCancellation();
  }
}
void Controller::beginCancellation(QString failure) {
  if (stage_ == Stage::Cancelling) {
    return;
  }
  cancellationFailure_ = std::move(failure);
  prompt_.clear();
  secret_ = false;
  stage_ = Stage::Cancelling;
  setState("waiting", QStringLiteral("Cancelling authentication"));
  transport_->cancel();
}
void Controller::finishCancellation() {
  activeUser_.clear();
  prompt_.clear();
  secret_ = false;
  const QString failure = std::exchange(cancellationFailure_, QString{});
  const QString nextUser = std::exchange(pendingUser_, QString{});
  stage_ = failure.isEmpty() ? Stage::Idle : Stage::Failed;
  setState(failure.isEmpty() ? QStringLiteral("user-selection") : QStringLiteral("failed"), failure);
  transport_->disconnectFromServer();
  if (!nextUser.isEmpty()) {
    stage_ = Stage::Idle;
    begin(nextUser);
    authenticationError_ = failure;
  }
}
void Controller::handle(const QJsonObject& message) {
  const QString type = message.value("type").toString();
  qCInfo(greeterController).noquote() << "reply" << type << "stage" << stageName(stage_);
  if (stage_ == Stage::Cancelling) {
    if (type == "success" || type == "error") {
      finishCancellation();
      return;
    }
    fail(QStringLiteral("Could not cancel greetd authentication"));
    return;
  }
  if (stage_ == Stage::Authenticating && type == "auth_message") {
    handleAuthenticationPrompt(message);
    return;
  }
  if (type == "success" && stage_ == Stage::Authenticating) {
    const Session* session = selected();
    if (session == nullptr) {
      {
        fail(QStringLiteral("Selected session is unavailable"));
      }
      return;
    }
    stage_ = Stage::Starting;
    QJsonArray command;
    for (const auto& argument : session->command) {
      command += argument;
    }
    transport_->send({
        {"type", "start_session"},
        {"cmd", command},
        {"env", QJsonArray{"XDG_SESSION_TYPE=wayland"}},
    });
    setState("starting");
    return;
  }
  if (type == "success" && stage_ == Stage::Starting) {
    QString error;
    if (!files_->save(statePath_, activeUser_, selectedSession_, manualMode(), &error)) {
      qCWarning(greeterController) << "Could not save greeter state:" << error;
    }
    stage_ = Stage::Complete;
    transport_->disconnectFromServer();
    setState("authenticated");
    emit sessionStarted();
    return;
  }
  if (type == "error" && stage_ != Stage::Idle) {
    const auto description = message.value("description");
    const QString reason = description.isString() && !description.toString().isEmpty()
                               ? description.toString()
                               : QStringLiteral("greetd rejected the request");
    if (message.value("error_type").toString() == "auth_error" && stage_ == Stage::Authenticating) {
      const QString authenticationFailure = QStringLiteral("Authentication failed");
      authenticationError_.clear();
      pendingUser_ = activeUser_;
      beginCancellation(authenticationFailure);
      return;
    }
    fail(reason);
    return;
  }
  fail(QStringLiteral("Unexpected greetd reply"));
}
void Controller::handleAuthenticationPrompt(const QJsonObject& message) {
  const QString kind = message.value("auth_message_type").toString();
  const auto promptValue = message.value("auth_message");
  if (!promptValue.isString() || !QStringList{"secret", "visible", "info", "error"}.contains(kind)) {
    {
      fail(QStringLiteral("Malformed greetd authentication prompt"));
    }
    return;
  }
  prompt_ = promptValue.toString();
  secret_ = kind == "secret";
  if (kind == "error") {
    authenticationError_ = QStringLiteral("Authentication failed");
    prompt_.clear();
    setState("waiting", authenticationError_);
  } else {
    const QString status =
        kind == "visible" || kind == "secret" ? std::exchange(authenticationError_, QString{}) : QString{};
    setState(kind == "info" ? "informational-prompt" : "input-prompt", status);
  }
  if (kind == "info" || kind == "error") {
    transport_->send({
        {"type", "post_auth_message_response"},
        {"response", QJsonValue::Null},
    });
  }
}

void Controller::requestPowerOff(bool confirmed) {
  if (powerConfirmationRequired() && !confirmed) {
    return;
  }
  if (demo_) {
    {
      setState(state_, QStringLiteral("Shutdown simulated in demo mode"));
    }
    return;
  }
  if (canPowerOff_) {
    power_->requestPowerOff();
  }
}
void Controller::requestReboot(bool confirmed) {
  if (powerConfirmationRequired() && !confirmed) {
    return;
  }
  if (demo_) {
    {
      setState(state_, QStringLiteral("Reboot simulated in demo mode"));
    }
    return;
  }
  if (canReboot_) {
    power_->requestReboot();
  }
}
void Controller::fail(const QString& reason) {
  if (stage_ == Stage::Failed) {
    return;
  }
  qCWarning(greeterController) << "controller-failure"
                               << "stage" << stageName(stage_);
  stage_ = Stage::Failed;
  prompt_.clear();
  secret_ = false;
  pendingUser_.clear();
  cancellationFailure_.clear();
  authenticationError_.clear();
  if (!demo_) {
    transport_->disconnectFromServer();
  }
  setState("failed", reason);
}
void Controller::setState(QString state, QString status) {
  qCInfo(greeterController).noquote() << "state-transition" << stageName(stage_) << state
                                      << (status.isEmpty() ? "no-status" : "status-present");
  state_ = std::move(state);
  status_ = std::move(status);
  emit changed();
}
}  // namespace Greeter