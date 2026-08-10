#pragma once
#include "services.h"
#include <QLocalSocket>
#include <QTimer>

namespace Greeter {
class GreetdClient final : public IGreetdTransport {
  Q_OBJECT
public:
  explicit GreetdClient(QObject *parent = nullptr);
  void connectTo(const QString &path) override;
  void send(const QJsonObject &message) override;
  void cancel() override;
  void disconnectFromServer() override;

private:
  void consume();
  void fail(const QString &reason);
  QLocalSocket socket_;
  QTimer timer_;
  QByteArray input_;
  std::optional<quint32> expected_;
  bool failing_ = false;
  static constexpr quint32 MaxFrame = 1024 * 1024;
};
} // namespace Greeter
