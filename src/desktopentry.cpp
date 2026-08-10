#include "desktopentry.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>

namespace Greeter {
namespace {
bool visibleForDesktop(const QSettings &entry) {
  const QStringList desktops =
      QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP"))
          .split(':', Qt::SkipEmptyParts);
  const QStringList only =
      entry.value("OnlyShowIn").toString().split(';', Qt::SkipEmptyParts);
  const QStringList excluded =
      entry.value("NotShowIn").toString().split(';', Qt::SkipEmptyParts);
  if (!only.isEmpty()) {
    bool matched = false;
    for (const auto &desktop : desktops)
      matched |= only.contains(desktop);
    if (!matched)
      return false;
  }
  for (const auto &desktop : desktops)
    if (excluded.contains(desktop))
      return false;
  return true;
}
} // namespace

QStringList parseDesktopExec(const QString &exec, const QString &name,
                             const QString &file, const QString &icon,
                             QString *error) {
  QStringList words;
  QString word;
  bool quoted = false;
  bool escaped = false;
  auto fail = [&](const QString &message) {
    if (error)
      *error = message;
    return QStringList{};
  };
  for (const QChar ch : exec) {
    if (escaped) {
      word += ch;
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      quoted = !quoted;
      continue;
    }
    if (ch.isSpace() && !quoted) {
      if (!word.isEmpty()) {
        words += word;
        word.clear();
      }
      continue;
    }
    word += ch;
  }
  if (quoted || escaped)
    return fail(QStringLiteral("malformed quoting"));
  if (!word.isEmpty())
    words += word;
  QStringList expanded;
  for (QString token : words) {
    QString output;
    for (qsizetype i = 0; i < token.size(); ++i) {
      if (token[i] != '%') {
        output += token[i];
        continue;
      }
      if (++i == token.size())
        return fail(QStringLiteral("trailing field code"));
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
        if (token != "%i")
          return fail(QStringLiteral("%i must be a standalone argument"));
        if (!icon.isEmpty())
          expanded << "--icon" << icon;
        break;
      case 'f':
      case 'F':
      case 'u':
      case 'U':
        break;
      default:
        return fail(QStringLiteral("unsupported field code"));
      }
    }
    if (!output.isEmpty())
      expanded += output;
  }
  if (expanded.isEmpty() || expanded.first().isEmpty())
    return fail(QStringLiteral("empty command"));
  return expanded;
}

QList<Session> discoverSessions(const QStringList &directories,
                                const QStringList &include,
                                const QStringList &exclude) {
  QList<Session> result;
  QSet<QString> seen;
  for (const QString &directory : directories) {
    QDir dir(directory);
    const auto files = dir.entryList({"*.desktop"}, QDir::Files, QDir::Name);
    for (const QString &id : files) {
      if (seen.contains(id))
        continue;
      seen += id;
      if ((!include.isEmpty() && !include.contains(id)) || exclude.contains(id))
        continue;
      const QString path = dir.filePath(id);
      QSettings entry(path, QSettings::IniFormat);
      entry.beginGroup("Desktop Entry");
      if (entry.value("Type").toString() != "Application" ||
          entry.value("Hidden").toString() == "true" ||
          entry.value("NoDisplay").toString() == "true" ||
          !visibleForDesktop(entry))
        continue;
      const QString name = entry.value("Name").toString();
      const QString tryExec = entry.value("TryExec").toString();
      if (name.isEmpty() || (!tryExec.isEmpty() &&
                             QStandardPaths::findExecutable(tryExec).isEmpty()))
        continue;
      QString error;
      const auto command =
          parseDesktopExec(entry.value("Exec").toString(), name, path,
                           entry.value("Icon").toString(), &error);
      if (!command.isEmpty())
        result.push_back({id, name, command});
    }
  }
  std::stable_sort(
      result.begin(), result.end(), [](const auto &left, const auto &right) {
        const int byName = QString::localeAwareCompare(left.name, right.name);
        return byName == 0 ? left.id < right.id : byName < 0;
      });
  return result;
}
} // namespace Greeter
