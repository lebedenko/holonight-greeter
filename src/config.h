#pragma once

#include <QString>
#include <QStringList>

namespace Greeter {
struct Config {
  enum class UserMode { List, Manual };
  UserMode userMode = UserMode::List;
  int minUid = 1000;
  int maxUid = 59999;
  QStringList includeUsers;
  QStringList excludeUsers{"greeter", "nobody"};
  bool showAvatars = true;
  QStringList sessionDirectories{"/usr/local/share/wayland-sessions",
                                 "/usr/share/wayland-sessions"};
  QStringList includeSessions;
  QStringList excludeSessions;
  QString defaultSession = "holonight-hyprland.desktop";
  QString keyboardLabel = "EN";
  QString background =
      "/usr/share/holonight-greeter/backgrounds/wallpaper1.png";
};

struct ConfigResult {
  Config value;
  QString warning;
  QString error;
  [[nodiscard]] bool valid() const { return error.isEmpty(); }
};

[[nodiscard]] ConfigResult loadConfig(const QString &path);
} // namespace Greeter
