#include "greetdclient.h"
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(greetdProtocol, "holonight.greeter.protocol")

namespace Greeter {
GreetdClient::GreetdClient(QObject *parent) : IGreetdTransport(parent) {
  timer_.setSingleShot(true);
  timer_.setInterval(15000);
  connect(&timer_, &QTimer::timeout, this,
          [this] { fail(QStringLiteral("greetd timed out"), "timeout"); });
  connect(&socket_, &QLocalSocket::connected, this, [this] {
    timer_.stop();
    qCInfo(greetdProtocol) << "transport-connected";
    emit connected();
  });
  connect(&socket_, &QLocalSocket::readyRead, this, [this] {
    input_ += socket_.readAll();
    consume();
  });
  connect(&socket_, &QLocalSocket::disconnected, this, [this] {
    timer_.stop();
    expected_.reset();
    input_.fill('\0');
    input_.clear();
    qCInfo(greetdProtocol) << "transport-disconnected"
                           << (failing_ ? "after-failure" : "normal");
    if (!failing_)
      emit disconnected();
    failing_ = false;
  });
  connect(&socket_, &QLocalSocket::errorOccurred, this, [this](auto) {
    if (socket_.error() != QLocalSocket::PeerClosedError)
      fail(socket_.errorString(), "socket");
  });
}
void GreetdClient::connectTo(const QString &path) {
  timer_.stop();
  expected_.reset();
  input_.fill('\0');
  input_.clear();
  failing_ = false;
  socket_.abort();
  if (path.isEmpty()) {
    fail(QStringLiteral("GREETD_SOCK is not set"), "configuration");
    return;
  }
  qCInfo(greetdProtocol) << "transport-connecting";
  timer_.start();
  socket_.connectToServer(path);
}
void GreetdClient::send(const QJsonObject &message) {
  if (socket_.state() != QLocalSocket::ConnectedState) {
    fail(QStringLiteral("greetd is not connected"), "not-connected");
    return;
  }
  QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
  qCInfo(greetdProtocol).noquote()
      << "send" << message.value("type").toString();
  const quint32 size = quint32(body.size());
  QByteArray frame(reinterpret_cast<const char *>(&size), sizeof(size));
  frame += body;
  if (socket_.write(frame) != frame.size()) {
    fail(QStringLiteral("could not queue greetd request"), "write");
    return;
  }
  body.fill('\0');
  frame.fill('\0');
  timer_.start();
}
void GreetdClient::cancel() { send({{"type", "cancel_session"}}); }
void GreetdClient::disconnectFromServer() {
  timer_.stop();
  expected_.reset();
  input_.fill('\0');
  input_.clear();
  socket_.disconnectFromServer();
}
void GreetdClient::fail(const QString &reason, const char *category) {
  if (failing_)
    return;
  failing_ = true;
  qCWarning(greetdProtocol) << "transport-failure" << category;
  timer_.stop();
  input_.fill('\0');
  input_.clear();
  emit failed(reason);
  socket_.abort();
  if (socket_.state() == QLocalSocket::UnconnectedState)
    failing_ = false;
}
void GreetdClient::consume() {
  while (true) {
    if (!expected_) {
      if (input_.size() < qsizetype(sizeof(quint32)))
        return;
      quint32 size = 0;
      memcpy(&size, input_.constData(), sizeof(size));
      input_.remove(0, sizeof(size));
      if (size == 0 || size > MaxFrame) {
        fail(QStringLiteral("invalid greetd frame length"), "framing");
        return;
      }
      expected_ = size;
    }
    if (input_.size() < *expected_)
      return;
    const QByteArray body = input_.first(*expected_);
    input_.remove(0, *expected_);
    expected_.reset();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(body, &parseError);
    if (!document.isObject()) {
      fail(QStringLiteral("invalid greetd JSON: %1")
               .arg(parseError.errorString()),
           "json");
      return;
    }
    timer_.stop();
    const QJsonObject message = document.object();
    qCInfo(greetdProtocol).noquote()
        << "receive" << message.value("type").toString()
        << message.value("auth_message_type").toString();
    emit this->message(message);
  }
}
} // namespace Greeter
