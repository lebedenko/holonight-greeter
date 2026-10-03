#pragma once

#include <QString>
#include <QStringList>

#include <cstdint>

namespace Greeter {
struct KeyboardLayout {
  QString id;
  QString layout;
  QString variant;
  QString label;
};

struct Config {
  enum class UserMode : std::uint8_t { List, Manual };
  UserMode user_mode = UserMode::List;
  int min_uid = 1000;
  int max_uid = 59999;
  QStringList include_users;
  QStringList exclude_users{"greeter", "nobody"};
  bool show_avatars = true;
  QStringList session_directories{
      "/usr/local/share/wayland-sessions",
      "/usr/share/wayland-sessions",
  };
  QStringList include_sessions;
  QStringList exclude_sessions;
  QString default_session = "holonight-hyprland.desktop";
  QString keyboard_label = "EN";
  QString keyboard_default;
  QString keyboard_options;
  QList<KeyboardLayout> keyboard_layouts;
  QString compositor_backend = "hyprland";
  QString primary_output;
  QString background = "/usr/share/holonight-greeter/backgrounds/wallpaper.png";
};

struct ConfigResult {
  Config value;
  QString warning;
  QString error;
  [[nodiscard]] bool valid() const { return error.isEmpty(); }
};

[[nodiscard]] ConfigResult loadConfig(const QString& path);
}  // namespace Greeter
