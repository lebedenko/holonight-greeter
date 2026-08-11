#pragma once

#include "config.h"
#include <QStringList>

namespace Greeter {
struct BackendCommand {
  QString program;
  QStringList arguments;
};

[[nodiscard]] BackendCommand cageCommand(const QString &greeter,
                                         const QString &config);
[[nodiscard]] BackendCommand hyprlandCommand(const QString &config);
[[nodiscard]] QString hyprlandConfig(const Config &config,
                                     const QString &greeter,
                                     const QString &configPath);
} // namespace Greeter
