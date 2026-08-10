#include "config.h"

#include <QDir>
#include <QFileInfo>
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
    rejectUnknown(root,
                  {"version", "users", "sessions", "keyboard", "appearance"});
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
      rejectUnknown(*keyboard, {"label"});
      result.value.keyboardLabel =
          string(*keyboard, "label", result.value.keyboardLabel);
      if (result.value.keyboardLabel.trimmed().isEmpty())
        throw std::runtime_error("keyboard.label must not be empty");
    } else if (root.contains("keyboard"))
      throw std::runtime_error("keyboard must be a table");
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
