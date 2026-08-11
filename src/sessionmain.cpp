#include "config.h"
#include "sessionlauncher.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <ranges>

Q_LOGGING_CATEGORY(sessionLog, "holonight.greeter.session")

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("holonight-greeter-session");
  QCoreApplication::setApplicationVersion("0.1.0");
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption({"config", "Greeter configuration", "path",
                    "/etc/holonight/greeter.toml"});
  parser.addOption({"backend", "Built-in backend override", "name"});
  parser.process(app);

  const QString configPath = parser.value("config");
  if (!QRegularExpression("^/[A-Za-z0-9_./-]+$").match(configPath).hasMatch()) {
    qCCritical(sessionLog) << "configuration-path-error";
    return 2;
  }
  const auto loaded = Greeter::loadConfig(configPath);
  if (!loaded.valid()) {
    qCCritical(sessionLog) << "configuration-error";
    return 2;
  }
  const QString backend = parser.isSet("backend")
                              ? parser.value("backend")
                              : loaded.value.compositorBackend;
  if (!QStringList{"hyprland", "cage"}.contains(backend)) {
    qCCritical(sessionLog) << "backend-unsupported";
    return 2;
  }
  const QString greeter = QStandardPaths::findExecutable("holonight-greeter");
  if (greeter.isEmpty()) {
    qCCritical(sessionLog) << "greeter-not-found";
    return 3;
  }

  QProcess process;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("HOLONIGHT_GREETER_BACKEND", backend);
  process.setProcessEnvironment(environment);
  process.setProcessChannelMode(QProcess::ForwardedChannels);
  QTemporaryDir runtime("/run/holonight-greeter/session-XXXXXX");
  if (!runtime.isValid()) {
    qCCritical(sessionLog) << "runtime-directory-error";
    return 3;
  }
  QString program;
  QStringList arguments;
  if (backend == "hyprland") {
    const QString path = runtime.filePath("hyprland.lua");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly) ||
        file.write(Greeter::hyprlandConfig(loaded.value, greeter, configPath)
                       .toUtf8()) < 0) {
      qCCritical(sessionLog) << "backend-config-error";
      return 3;
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    file.close();
    const auto command = Greeter::hyprlandCommand(path);
    program = command.program;
    arguments = command.arguments;
  } else {
    const auto command = Greeter::cageCommand(greeter, configPath);
    program = command.program;
    arguments = command.arguments;
    if (!loaded.value.keyboardLayouts.isEmpty()) {
      auto layout = loaded.value.keyboardLayouts.cbegin();
      const auto configured = std::ranges::find(loaded.value.keyboardLayouts,
                                                loaded.value.keyboardDefault,
                                                &Greeter::KeyboardLayout::id);
      if (configured != loaded.value.keyboardLayouts.cend())
        layout = configured;
      environment.insert("XKB_DEFAULT_LAYOUT", layout->layout);
      environment.insert("XKB_DEFAULT_VARIANT", layout->variant);
      environment.insert("XKB_DEFAULT_OPTIONS", loaded.value.keyboardOptions);
      process.setProcessEnvironment(environment);
    }
  }
  qCInfo(sessionLog) << "launcher-start"
                     << QCoreApplication::applicationVersion() << "backend"
                     << backend;
  process.start(program, arguments);
  if (!process.waitForStarted(5000)) {
    qCCritical(sessionLog) << "backend-start-error" << backend;
    return 4;
  }
  qCInfo(sessionLog) << "backend-ready" << backend;
  process.waitForFinished(-1);
  qCInfo(sessionLog) << "backend-exit" << backend << process.exitCode();
  return process.exitStatus() == QProcess::NormalExit ? process.exitCode()
                                                      : 128;
}
