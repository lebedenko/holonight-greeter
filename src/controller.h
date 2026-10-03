#pragma once

#include "config.h"
#include "services.h"

#include <QVariantList>

#include <cstdint>

namespace Greeter {
class Controller final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(QString prompt READ prompt NOTIFY changed)
  Q_PROPERTY(bool secret READ secret NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(bool demo READ demo CONSTANT)
  Q_PROPERTY(bool manualMode READ manualMode CONSTANT)
  Q_PROPERTY(QString initialUser READ initialUser CONSTANT)
  Q_PROPERTY(QVariantList users READ users CONSTANT)
  Q_PROPERTY(QVariantList sessions READ sessions CONSTANT)
  Q_PROPERTY(QString selectedSession READ selectedSession WRITE setSelectedSession NOTIFY changed)
  Q_PROPERTY(QString selectedSessionName READ selectedSessionName NOTIFY changed)
  Q_PROPERTY(bool canPowerOff READ canPowerOff NOTIFY changed)
  Q_PROPERTY(bool canReboot READ canReboot NOTIFY changed)
  Q_PROPERTY(bool powerConfirmationRequired READ powerConfirmationRequired NOTIFY changed)
 public:
  Controller(bool demo, QString scenario, Config config, QString statePath, IGreetdTransport* transport,
             IAccountSource* accounts, IPowerService* power, IFileSystem* files, QObject* parent = nullptr);
  [[nodiscard]] QString state() const { return state_; }
  [[nodiscard]] QString prompt() const { return prompt_; }
  [[nodiscard]] bool secret() const { return secret_; }
  [[nodiscard]] QString status() const { return status_; }
  [[nodiscard]] bool demo() const { return demo_; }
  [[nodiscard]] bool manualMode() const { return config_.user_mode == Config::UserMode::Manual; }
  [[nodiscard]] QString initialUser() const { return initialUser_; }
  [[nodiscard]] QVariantList users() const;
  [[nodiscard]] QVariantList sessions() const;
  [[nodiscard]] QString selectedSession() const { return selectedSession_; }
  [[nodiscard]] QString selectedSessionName() const;
  [[nodiscard]] bool canPowerOff() const { return demo_ || canPowerOff_; }
  [[nodiscard]] bool canReboot() const { return demo_ || canReboot_; }
  [[nodiscard]] bool powerConfirmationRequired() const { return demo_ || powerConfirmationRequired_; }
  void setSelectedSession(const QString& identifier);
  Q_INVOKABLE void begin(const QString& user);
  Q_INVOKABLE void respond(const QString& response);
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void restartAuthentication();
  Q_INVOKABLE void requestPowerOff(bool confirmed = false);
  Q_INVOKABLE void requestReboot(bool confirmed = false);
 signals:
  void changed();
  void sessionStarted();

 private:
  enum class Stage : std::uint8_t {
    Idle,
    Connecting,
    Authenticating,
    Cancelling,
    Starting,
    Complete,
    Failed,
  };
  static const char* stageName(Stage stage);
  void initializeRecords(IAccountSource* accounts);
  void beginDemoAuthentication();
  void handleAuthenticationPrompt(const QJsonObject& message);
  void handle(const QJsonObject& message);
  void beginCancellation(QString failure = {});
  void finishCancellation();
  void setState(QString state, QString status = {});
  void fail(const QString& reason);
  [[nodiscard]] const Session* selected() const;
  bool demo_;
  QString scenario_;
  Config config_;
  QString statePath_;
  IGreetdTransport* transport_;
  IPowerService* power_;
  IFileSystem* files_;
  QList<User> userRecords_;
  QList<Session> sessionRecords_;
  QString selectedSession_;
  QString initialUser_;
  QString activeUser_;
  QString pendingUser_;
  QString cancellationFailure_;
  QString authenticationError_;
  QString state_ = "user-selection";
  QString prompt_;
  QString status_;
  bool secret_ = false;
  bool canPowerOff_ = false;
  bool canReboot_ = false;
  bool powerConfirmationRequired_ = true;
  int demoStep_ = 0;
  quint64 demoAttempt_ = 0;
  Stage stage_ = Stage::Idle;
};
}  // namespace Greeter
