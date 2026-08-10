#pragma once
#include "config.h"
#include "services.h"
#include <QVariantList>

namespace Greeter {
class Controller final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(QString prompt READ prompt NOTIFY changed)
  Q_PROPERTY(bool secret READ secret NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(bool demo READ demo CONSTANT)
  Q_PROPERTY(bool manualMode READ manualMode CONSTANT)
  Q_PROPERTY(QVariantList users READ users CONSTANT)
  Q_PROPERTY(QVariantList sessions READ sessions CONSTANT)
  Q_PROPERTY(QString selectedSession READ selectedSession WRITE
                 setSelectedSession NOTIFY changed)
  Q_PROPERTY(
      QString selectedSessionName READ selectedSessionName NOTIFY changed)
  Q_PROPERTY(bool canPowerOff READ canPowerOff NOTIFY changed)
  Q_PROPERTY(bool canReboot READ canReboot NOTIFY changed)
public:
  Controller(bool demo, QString scenario, Config config, QString statePath,
             IGreetdTransport *transport, IAccountSource *accounts,
             IPowerService *power, IFileSystem *files,
             QObject *parent = nullptr);
  QString state() const { return state_; }
  QString prompt() const { return prompt_; }
  bool secret() const { return secret_; }
  QString status() const { return status_; }
  bool demo() const { return demo_; }
  bool manualMode() const {
    return config_.userMode == Config::UserMode::Manual;
  }
  QVariantList users() const;
  QVariantList sessions() const;
  QString selectedSession() const { return selectedSession_; }
  QString selectedSessionName() const;
  bool canPowerOff() const { return demo_ || canPowerOff_; }
  bool canReboot() const { return demo_ || canReboot_; }
  void setSelectedSession(const QString &id);
  Q_INVOKABLE void begin(const QString &user);
  Q_INVOKABLE void respond(const QString &response);
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void requestPowerOff();
  Q_INVOKABLE void requestReboot();
signals:
  void changed();

private:
  enum class Stage {
    Idle,
    Connecting,
    Authenticating,
    Starting,
    Complete,
    Failed
  };
  void handle(const QJsonObject &message);
  void setState(QString state, QString status = {});
  void fail(const QString &reason);
  const Session *selected() const;
  bool demo_;
  QString scenario_;
  Config config_;
  QString statePath_;
  IGreetdTransport *transport_;
  IPowerService *power_;
  IFileSystem *files_;
  QList<User> userRecords_;
  QList<Session> sessionRecords_;
  QString selectedSession_;
  QString activeUser_;
  QString state_ = "user-selection";
  QString prompt_;
  QString status_;
  bool secret_ = false;
  bool canPowerOff_ = false;
  bool canReboot_ = false;
  int demoStep_ = 0;
  quint64 demoAttempt_ = 0;
  Stage stage_ = Stage::Idle;
};
} // namespace Greeter
