#include "config.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

#include <limits>
#include <ranges>
#include <toml++/toml.h>

namespace {
QStringList strings(const toml::table& table, std::string_view key) {
  QStringList result;
  const auto* array = table[key].as_array();
  if (array == nullptr) {
    throw std::runtime_error(std::string(key) + " must be an array");
  }
  for (const auto& item : *array) {
    const auto value = item.value<std::string>();
    if (!value) {
      throw std::runtime_error(std::string(key) + " entries must be strings");
    }
    result.push_back(QString::fromStdString(*value));
  }
  return result;
}

void rejectUnknown(const toml::table& table, std::initializer_list<std::string_view> allowed) {
  for (const auto& [key, value] : table) {
    Q_UNUSED(value)
    if (std::ranges::find(allowed, key.str()) == allowed.end()) {
      throw std::runtime_error("unknown key: " + std::string(key.str()));
    }
  }
}

QString pathValue(const toml::table& table, std::string_view key, const QString& fallback) {
  if (!table.contains(key)) {
    return fallback;
  }
  const auto value = table[key].value<std::string>();
  if (!value) {
    throw std::runtime_error(std::string(key) + " must be a string");
  }
  const QString path = QString::fromStdString(*value);
  if (path.isEmpty() || !QFileInfo(path).isAbsolute() || QDir::cleanPath(path) != path) {
    throw std::runtime_error(std::string(key) + " must be a safe absolute path");
  }
  return path;
}

int integer(const toml::table& table, std::string_view key, int fallback) {
  if (!table.contains(key)) {
    return fallback;
  }
  const auto value = table[key].value<int64_t>();
  if (!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max()) {
    throw std::runtime_error(std::string(key) + " must be an integer in range");
  }
  return static_cast<int>(*value);
}

QString string(const toml::table& table, std::string_view key, const QString& fallback) {
  if (!table.contains(key)) {
    return fallback;
  }
  const auto value = table[key].value<std::string>();
  if (!value) {
    throw std::runtime_error(std::string(key) + " must be a string");
  }
  return QString::fromStdString(*value);
}

QList<Greeter::KeyboardLayout> layouts(const toml::table& table) {
  QList<Greeter::KeyboardLayout> result;
  const auto* array = table["layouts"].as_array();
  if (array == nullptr) {
    throw std::runtime_error("keyboard.layouts must be an array");
  }
  QSet<QString> ids;
  for (const auto& item : *array) {
    const auto* entry = item.as_table();
    if (entry == nullptr) {
      throw std::runtime_error("keyboard.layouts entries must be tables");
    }
    rejectUnknown(*entry, {"id", "layout", "variant", "label"});
    Greeter::KeyboardLayout value{
        .id = string(*entry, "id", {}),
        .layout = string(*entry, "layout", {}),
        .variant = string(*entry, "variant", {}),
        .label = string(*entry, "label", {}),
    };
    if (value.id.isEmpty() || value.layout.isEmpty() || value.label.isEmpty()) {
      throw std::runtime_error("keyboard layout id, layout, and label are required");
    }
    if (ids.contains(value.id)) {
      throw std::runtime_error("keyboard layout IDs must be unique");
    }
    ids.insert(value.id);
    result += value;
  }
  if (result.isEmpty()) {
    throw std::runtime_error("keyboard.layouts must not be empty");
  }
  return result;
}
void loadUsers(const toml::table& root, Greeter::Config& config) {
  if (const auto* users = root["users"].as_table()) {
    rejectUnknown(*users, {
                              "mode",
                              "min_uid",
                              "max_uid",
                              "include",
                              "exclude",
                              "show_avatars",
                          });
    if (users->contains("mode")) {
      const auto mode = (*users)["mode"].value<std::string>();
      if (!mode) {
        throw std::runtime_error("users.mode must be a string");
      }
      if (*mode == "list") {
        config.user_mode = Greeter::Config::UserMode::List;
      } else if (*mode == "manual") {
        config.user_mode = Greeter::Config::UserMode::Manual;
      } else {
        throw std::runtime_error("users.mode must be list or manual");
      }
    }
    config.min_uid = integer(*users, "min_uid", config.min_uid);
    config.max_uid = integer(*users, "max_uid", config.max_uid);
    if (users->contains("include")) {
      config.include_users = strings(*users, "include");
    }
    if (users->contains("exclude")) {
      config.exclude_users = strings(*users, "exclude");
    }
    if (users->contains("show_avatars")) {
      auto value = (*users)["show_avatars"].value<bool>();
      if (!value) {
        throw std::runtime_error("show_avatars must be boolean");
      }
      config.show_avatars = *value;
    }
    if (config.min_uid < 0 || config.max_uid < config.min_uid) {
      throw std::runtime_error("invalid UID range");
    }
  } else if (root.contains("users")) {
    {
      throw std::runtime_error("users must be a table");
    }
  }
}

void loadSessions(const toml::table& root, Greeter::Config& config) {
  if (const auto* sessions = root["sessions"].as_table()) {
    rejectUnknown(*sessions, {"directories", "include", "exclude", "default"});
    if (sessions->contains("directories")) {
      config.session_directories = strings(*sessions, "directories");
    }
    for (const auto& dir : config.session_directories) {
      if (dir.isEmpty() || !QFileInfo(dir).isAbsolute() || QDir::cleanPath(dir) != dir) {
        throw std::runtime_error("unsafe session directory");
      }
    }
    if (sessions->contains("include")) {
      config.include_sessions = strings(*sessions, "include");
    }
    if (sessions->contains("exclude")) {
      config.exclude_sessions = strings(*sessions, "exclude");
    }
    config.default_session = string(*sessions, "default", config.default_session);
    if (config.default_session.isEmpty() || config.default_session.contains('/')) {
      throw std::runtime_error("sessions.default must be a filename");
    }
  } else if (root.contains("sessions")) {
    {
      throw std::runtime_error("sessions must be a table");
    }
  }
}

void loadKeyboard(const toml::table& root, Greeter::Config& config) {
  if (const auto* keyboard = root["keyboard"].as_table()) {
    rejectUnknown(*keyboard, {"label", "default", "options", "layouts"});
    config.keyboard_label = string(*keyboard, "label", config.keyboard_label);
    config.keyboard_default = string(*keyboard, "default", {});
    config.keyboard_options = string(*keyboard, "options", {});
    if (keyboard->contains("layouts")) {
      config.keyboard_layouts = layouts(*keyboard);
    }
    if (config.keyboard_layouts.isEmpty() && config.keyboard_label.trimmed().isEmpty()) {
      throw std::runtime_error("keyboard.label must not be empty");
    }
    if (!config.keyboard_layouts.isEmpty()) {
      if (config.keyboard_default.isEmpty()) {
        throw std::runtime_error("keyboard.default is required with layouts");
      }
      const auto found =
          std::ranges::find(config.keyboard_layouts, config.keyboard_default, &Greeter::KeyboardLayout::id);
      if (found == config.keyboard_layouts.end()) {
        throw std::runtime_error("keyboard.default must name a layout ID");
      }
      config.keyboard_label = found->label;
    }
  } else if (root.contains("keyboard")) {
    {
      throw std::runtime_error("keyboard must be a table");
    }
  }
}

void loadCompositor(const toml::table& root, Greeter::Config& config) {
  if (const auto* compositor = root["compositor"].as_table()) {
    rejectUnknown(*compositor, {"backend", "primary_output"});
    config.compositor_backend = string(*compositor, "backend", config.compositor_backend);
    config.primary_output = string(*compositor, "primary_output", {});
    if (!QStringList{"hyprland", "cage"}.contains(config.compositor_backend)) {
      throw std::runtime_error("compositor.backend must be hyprland or cage");
    }
  } else if (root.contains("compositor")) {
    {
      throw std::runtime_error("compositor must be a table");
    }
  }
}

void loadAppearance(const toml::table& root, Greeter::Config& config) {
  if (const auto* appearance = root["appearance"].as_table()) {
    rejectUnknown(*appearance, {"background"});
    config.background = pathValue(*appearance, "background", config.background);
  } else if (root.contains("appearance")) {
    {
      throw std::runtime_error("appearance must be a table");
    }
  }
}

}  // namespace

namespace Greeter {
ConfigResult loadConfig(const QString& path) {
  ConfigResult result;
  if (!QFileInfo::exists(path)) {
    result.warning = QStringLiteral("Configuration missing; compiled defaults are active");
    return result;
  }
  try {
    const auto root = toml::parse_file(path.toStdString());
    rejectUnknown(root, {
                            "version",
                            "users",
                            "sessions",
                            "keyboard",
                            "appearance",
                            "compositor",
                        });
    if (!root.contains("version") || !root["version"].is_integer() || root["version"].value<int64_t>() != 1) {
      throw std::runtime_error("version must equal 1");
    }
    loadUsers(root, result.value);
    loadSessions(root, result.value);
    loadKeyboard(root, result.value);
    loadCompositor(root, result.value);
    loadAppearance(root, result.value);
  } catch (const std::exception& error) {
    result.error = QString::fromUtf8(error.what());
  }
  return result;
}
}  // namespace Greeter
