#include "config.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <limits>
#include <ranges>
#include <toml++/toml.h>

namespace {
QStringList strings(const toml::table &table, std::string_view key) {
  QStringList result;
  const auto *array = table[key].as_array();
  if (!array)
    throw std::runtime_error(std::string(key) + " must be an array");
  for (const auto &item : *array) {
    const auto value = item.value<std::string>();
    if (!value)
      throw std::runtime_error(std::string(key) + " entries must be strings");
    result.push_back(QString::fromStdString(*value));
  }
  return result;
}

void rejectUnknown(const toml::table &table,
                   std::initializer_list<std::string_view> allowed) {
  for (const auto &[key, value] : table) {
    Q_UNUSED(value)
    if (std::ranges::find(allowed, key.str()) == allowed.end())
      throw std::runtime_error("unknown key: " + std::string(key.str()));
  }
}

QString pathValue(const toml::table &table, std::string_view key,
                  const QString &fallback) {
  if (!table.contains(key))
    return fallback;
  const auto value = table[key].value<std::string>();
  if (!value)
    throw std::runtime_error(std::string(key) + " must be a string");
  const QString path = QString::fromStdString(*value);
  if (path.isEmpty() || !QFileInfo(path).isAbsolute() ||
      QDir::cleanPath(path) != path)
    throw std::runtime_error(std::string(key) +
                             " must be a safe absolute path");
  return path;
}

int integer(const toml::table &table, std::string_view key, int fallback) {
  if (!table.contains(key))
    return fallback;
  const auto value = table[key].value<int64_t>();
  if (!value || *value < std::numeric_limits<int>::min() ||
      *value > std::numeric_limits<int>::max())
    throw std::runtime_error(std::string(key) + " must be an integer in range");
  return static_cast<int>(*value);
}

QString string(const toml::table &table, std::string_view key,
               const QString &fallback) {
  if (!table.contains(key))
    return fallback;
  const auto value = table[key].value<std::string>();
  if (!value)
    throw std::runtime_error(std::string(key) + " must be a string");
  return QString::fromStdString(*value);
}

QList<Greeter::KeyboardLayout> layouts(const toml::table &table) {
  QList<Greeter::KeyboardLayout> result;
  const auto *array = table["layouts"].as_array();
  if (!array)
    throw std::runtime_error("keyboard.layouts must be an array");
  QSet<QString> ids;
  for (const auto &item : *array) {
    const auto *entry = item.as_table();
    if (!entry)
      throw std::runtime_error("keyboard.layouts entries must be tables");
    rejectUnknown(*entry, {"id", "layout", "variant", "label"});
    Greeter::KeyboardLayout value{
        string(*entry, "id", {}), string(*entry, "layout", {}),
        string(*entry, "variant", {}), string(*entry, "label", {})};
    if (value.id.isEmpty() || value.layout.isEmpty() || value.label.isEmpty())
      throw std::runtime_error(
          "keyboard layout id, layout, and label are required");
    if (ids.contains(value.id))
      throw std::runtime_error("keyboard layout IDs must be unique");
    ids.insert(value.id);
    result += value;
  }
  if (result.isEmpty())
    throw std::runtime_error("keyboard.layouts must not be empty");
  return result;
}
} // namespace

namespace Greeter {
ConfigResult loadConfig(const QString &path) {
  ConfigResult result;
  if (!QFileInfo::exists(path)) {
    result.warning =
        QStringLiteral("Configuration missing; compiled defaults are active");
    return result;
  }
  try {
    const auto root = toml::parse_file(path.toStdString());
    rejectUnknown(root, {"version", "users", "sessions", "keyboard",
                         "appearance", "compositor"});
    if (!root.contains("version") || !root["version"].is_integer() ||
        root["version"].value<int64_t>() != 1)
      throw std::runtime_error("version must equal 1");
    if (const auto *users = root["users"].as_table()) {
      rejectUnknown(*users, {"mode", "min_uid", "max_uid", "include", "exclude",
                             "show_avatars"});
      if (users->contains("mode")) {
        const auto mode = (*users)["mode"].value<std::string>();
        if (!mode)
          throw std::runtime_error("users.mode must be a string");
        if (*mode == "list")
          result.value.userMode = Config::UserMode::List;
        else if (*mode == "manual")
          result.value.userMode = Config::UserMode::Manual;
        else
          throw std::runtime_error("users.mode must be list or manual");
      }
      result.value.minUid = integer(*users, "min_uid", result.value.minUid);
      result.value.maxUid = integer(*users, "max_uid", result.value.maxUid);
      if (users->contains("include"))
        result.value.includeUsers = strings(*users, "include");
      if (users->contains("exclude"))
        result.value.excludeUsers = strings(*users, "exclude");
      if (users->contains("show_avatars")) {
        auto value = (*users)["show_avatars"].value<bool>();
        if (!value)
          throw std::runtime_error("show_avatars must be boolean");
        result.value.showAvatars = *value;
      }
      if (result.value.minUid < 0 || result.value.maxUid < result.value.minUid)
        throw std::runtime_error("invalid UID range");
    } else if (root.contains("users"))
      throw std::runtime_error("users must be a table");
    if (const auto *sessions = root["sessions"].as_table()) {
      rejectUnknown(*sessions,
                    {"directories", "include", "exclude", "default"});
      if (sessions->contains("directories"))
        result.value.sessionDirectories = strings(*sessions, "directories");
      for (const auto &dir : result.value.sessionDirectories)
        if (dir.isEmpty() || !QFileInfo(dir).isAbsolute() ||
            QDir::cleanPath(dir) != dir)
          throw std::runtime_error("unsafe session directory");
      if (sessions->contains("include"))
        result.value.includeSessions = strings(*sessions, "include");
      if (sessions->contains("exclude"))
        result.value.excludeSessions = strings(*sessions, "exclude");
      result.value.defaultSession =
          string(*sessions, "default", result.value.defaultSession);
      if (result.value.defaultSession.isEmpty() ||
          result.value.defaultSession.contains('/'))
        throw std::runtime_error("sessions.default must be a filename");
    } else if (root.contains("sessions"))
      throw std::runtime_error("sessions must be a table");
    if (const auto *keyboard = root["keyboard"].as_table()) {
      rejectUnknown(*keyboard, {"label", "default", "options", "layouts"});
      result.value.keyboardLabel =
          string(*keyboard, "label", result.value.keyboardLabel);
      result.value.keyboardDefault = string(*keyboard, "default", {});
      result.value.keyboardOptions = string(*keyboard, "options", {});
      if (keyboard->contains("layouts"))
        result.value.keyboardLayouts = layouts(*keyboard);
      if (result.value.keyboardLayouts.isEmpty() &&
          result.value.keyboardLabel.trimmed().isEmpty())
        throw std::runtime_error("keyboard.label must not be empty");
      if (!result.value.keyboardLayouts.isEmpty()) {
        if (result.value.keyboardDefault.isEmpty())
          throw std::runtime_error("keyboard.default is required with layouts");
        const auto found = std::ranges::find(result.value.keyboardLayouts,
                                             result.value.keyboardDefault,
                                             &KeyboardLayout::id);
        if (found == result.value.keyboardLayouts.end())
          throw std::runtime_error("keyboard.default must name a layout ID");
        result.value.keyboardLabel = found->label;
      }
    } else if (root.contains("keyboard"))
      throw std::runtime_error("keyboard must be a table");
    if (const auto *compositor = root["compositor"].as_table()) {
      rejectUnknown(*compositor, {"backend", "primary_output"});
      result.value.compositorBackend =
          string(*compositor, "backend", result.value.compositorBackend);
      result.value.primaryOutput = string(*compositor, "primary_output", {});
      if (!QStringList{"hyprland", "cage"}.contains(
              result.value.compositorBackend))
        throw std::runtime_error("compositor.backend must be hyprland or cage");
    } else if (root.contains("compositor"))
      throw std::runtime_error("compositor must be a table");
    if (const auto *appearance = root["appearance"].as_table()) {
      rejectUnknown(*appearance, {"background"});
      result.value.background =
          pathValue(*appearance, "background", result.value.background);
    } else if (root.contains("appearance"))
      throw std::runtime_error("appearance must be a table");
  } catch (const std::exception &error) {
    result.error = QString::fromUtf8(error.what());
  }
  return result;
}
} // namespace Greeter
