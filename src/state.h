#pragma once
#include <QString>
#include <QStringList>

namespace Greeter {
struct State {
  QString last_user;
  QString last_session;
};
[[nodiscard]] State loadState(const QString& path);
[[nodiscard]] bool saveState(const QString& path, const State& state, bool manualMode, QString* error = nullptr);
[[nodiscard]] QString selectSession(const State& state, const QString& configuredDefault,
                                    const QStringList& validSessions);
}  // namespace Greeter
