#include "desktopentry.h"

#include <QDir>
#include <QFile>
#include <QMap>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>
#include <optional>

namespace Greeter {
namespace {
using DesktopEntry = QMap<QString, QStringList>;

std::optional<QStringList> decodeDesktopValue(const QString& value, bool list) {
  QStringList result;
  QString word;
  for (qsizetype i = 0; i < value.size(); ++i) {
    const QChar character = value[i];
    if (character == '\\') {
      if (++i == value.size()) {
        return std::nullopt;
      }
      switch (value[i].unicode()) {
        case 's':
          word += ' ';
          break;
        case 'n':
          word += '\n';
          break;
        case 't':
          word += '\t';
          break;
        case 'r':
          word += '\r';
          break;
        case '\\':
          word += '\\';
          break;
        case ';':
          if (!list) {
            return std::nullopt;
          }
          word += ';';
          break;
        default:
          return std::nullopt;
      }
    } else if (character == ';' && list) {
      result += word;
      word.clear();
    } else {
      word += character;
    }
  }
  if (!list || !word.isEmpty()) {
    result += word;
  }
  return result;
}

bool appendDesktopValue(const QString& line, const QSet<QString>& stringKeys, const QSet<QString>& listKeys,
                        DesktopEntry& entry) {
  const qsizetype equals = line.indexOf('=');
  if (equals < 0) {
    return false;
  }
  const QString key = line.left(equals).trimmed();
  const bool list = listKeys.contains(key);
  if (!list && !stringKeys.contains(key)) {
    return true;
  }
  if (entry.contains(key)) {
    return false;
  }
  qsizetype start = equals + 1;
  while (start < line.size() && line[start].isSpace()) {
    ++start;
  }
  // Desktop strings have their own escaping rules. Quotes belong to Exec's
  // command parser; an INI reader such as QSettings consumes them too early.
  const auto decoded = decodeDesktopValue(line.mid(start), list);
  if (!decoded) {
    return false;
  }
  entry.insert(key, *decoded);
  return true;
}

DesktopEntry readDesktopEntry(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  DesktopEntry entry;
  bool inDesktopEntry = false;
  bool foundDesktopEntry = false;
  const QSet<QString> stringKeys{
      "Type", "Name", "Exec", "TryExec", "Icon", "Hidden", "NoDisplay",
  };
  const QSet<QString> listKeys{"OnlyShowIn", "NotShowIn"};
  while (!file.atEnd()) {
    QString line = QString::fromUtf8(file.readLine());
    if (line.endsWith('\n')) {
      line.chop(1);
    }
    if (line.endsWith('\r')) {
      line.chop(1);
    }
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#')) {
      continue;
    }
    if (trimmed.startsWith('[')) {
      inDesktopEntry = trimmed == "[Desktop Entry]";
      if (inDesktopEntry && foundDesktopEntry) {
        return {};
      }
      foundDesktopEntry |= inDesktopEntry;
      continue;
    }
    if (!inDesktopEntry) {
      continue;
    }
    if (!appendDesktopValue(line, stringKeys, listKeys, entry)) {
      return {};
    }
  }
  return file.error() == QFileDevice::NoError ? entry : DesktopEntry{};
}

QString entryString(const DesktopEntry& entry, const QString& key) { return entry.value(key).value(0); }

bool visibleForDesktop(const DesktopEntry& entry) {
  const QStringList desktops = QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")).split(':', Qt::SkipEmptyParts);
  const QStringList only = entry.value("OnlyShowIn");
  const QStringList excluded = entry.value("NotShowIn");
  if (!only.isEmpty()) {
    bool matched = false;
    for (const auto& desktop : desktops) {
      matched |= only.contains(desktop);
    }
    if (!matched) {
      return false;
    }
  }
  return std::ranges::none_of(desktops, [&excluded](const auto& desktop) { return excluded.contains(desktop); });
}
QStringList failExec(const QString& message, QString* error) {
  if (error != nullptr) {
    *error = message;
  }
  return {};
}
QStringList expandExecWords(const QStringList& words, const QString& name, const QString& file, const QString& icon,
                            QString* error) {
  QStringList expanded;
  for (QString token : words) {
    QString output;
    for (qsizetype i = 0; i < token.size(); ++i) {
      if (token[i] != '%') {
        output += token[i];
        continue;
      }
      if (++i == token.size()) {
        return failExec(QStringLiteral("trailing field code"), error);
      }
      switch (token[i].unicode()) {
        case '%':
          output += '%';
          break;
        case 'c':
          output += name;
          break;
        case 'k':
          output += file;
          break;
        case 'i':
          if (token != "%i") {
            return failExec(QStringLiteral("%i must be a standalone argument"), error);
          }
          if (!icon.isEmpty()) {
            expanded << "--icon" << icon;
          }
          break;
        case 'f':
        case 'F':
        case 'u':
        case 'U':
          break;
        default:
          return failExec(QStringLiteral("unsupported field code"), error);
      }
    }
    if (!output.isEmpty()) {
      expanded += output;
    }
  }
  if (expanded.isEmpty() || expanded.first().isEmpty()) {
    return failExec(QStringLiteral("empty command"), error);
  }
  return expanded;
}
}  // namespace

QStringList parseDesktopExec(const QString& exec, const QString& name, const QString& file, const QString& icon,
                             QString* error) {
  QStringList words;
  QString word;
  bool quoted = false;
  bool escaped = false;
  for (const QChar character : exec) {
    if (escaped) {
      word += character;
      escaped = false;
      continue;
    }
    if (character == '\\') {
      escaped = true;
      continue;
    }
    if (character == '"') {
      quoted = !quoted;
      continue;
    }
    if (character.isSpace() && !quoted) {
      if (!word.isEmpty()) {
        words += word;
        word.clear();
      }
      continue;
    }
    word += character;
  }
  if (quoted || escaped) {
    return failExec(QStringLiteral("malformed quoting"), error);
  }
  if (!word.isEmpty()) {
    words += word;
  }
  return expandExecWords(words, name, file, icon, error);
}

QList<Session> discoverSessions(const QStringList& directories, const QStringList& include,
                                const QStringList& exclude) {
  QList<Session> result;
  QSet<QString> seen;
  for (const QString& directory : directories) {
    QDir dir(directory);
    const auto files = dir.entryList({"*.desktop"}, QDir::Files, QDir::Name);
    for (const QString& identifier : files) {
      if (seen.contains(identifier)) {
        continue;
      }
      seen += identifier;
      if ((!include.isEmpty() && !include.contains(identifier)) || exclude.contains(identifier)) {
        continue;
      }
      const QString path = dir.filePath(identifier);
      const auto entry = readDesktopEntry(path);
      if (entryString(entry, "Type") != "Application" || entryString(entry, "Hidden") == "true" ||
          entryString(entry, "NoDisplay") == "true" || !visibleForDesktop(entry)) {
        continue;
      }
      const QString name = entryString(entry, "Name");
      const QString tryExec = entryString(entry, "TryExec");
      if (name.isEmpty() || (!tryExec.isEmpty() && QStandardPaths::findExecutable(tryExec).isEmpty())) {
        continue;
      }
      QString error;
      const auto command = parseDesktopExec(entryString(entry, "Exec"), name, path, entryString(entry, "Icon"), &error);
      if (!command.isEmpty()) {
        result.push_back({.id = identifier, .name = name, .command = command});
      }
    }
  }
  std::ranges::stable_sort(result, [](const auto& left, const auto& right) {
    const int byName = QString::localeAwareCompare(left.name, right.name);
    return byName == 0 ? left.id < right.id : byName < 0;
  });
  return result;
}
}  // namespace Greeter
