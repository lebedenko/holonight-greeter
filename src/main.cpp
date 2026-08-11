#include "compositoradapter.h"
#include "config.h"
#include "controller.h"
#include "greetdclient.h"
#include "outputmanager.h"
#include "services.h"
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSysInfo>
#include <QWindow>
#include <memory>

Q_LOGGING_CATEGORY(greeterLifecycle, "holonight.greeter.lifecycle")

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setApplicationName("holonight-greeter");
  qCInfo(greeterLifecycle) << "application-starting";
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOption(
      {"config", "Configuration file", "path", "/etc/holonight/greeter.toml"});
  parser.addOption(
      {"state", "State file", "path", "/var/lib/holonight-greeter/state.json"});
  parser.addOption(
      {"demo", "Run windowed with simulated authentication and actions"});
  parser.addOption({"demo-scenario", "Demo scenario", "name", "default"});
  parser.process(app);
  const QString scenario = parser.value("demo-scenario");
  if (!QStringList{"default", "wrong-password", "otp", "fingerprint"}.contains(
          scenario))
    parser.showHelp(2);

  const bool demo = parser.isSet("demo") || parser.isSet("demo-scenario");
  qCInfo(greeterLifecycle) << "configuration-loading"
                           << (demo ? "demo" : "production");
  const auto config = Greeter::loadConfig(parser.value("config"));
  qCInfo(greeterLifecycle) << "configuration-loaded"
                           << (config.valid() ? "valid" : "invalid")
                           << (config.warning.isEmpty() ? "no-warning"
                                                        : "warning");
  Greeter::GreetdClient transport;
  Greeter::SystemAccountSource accounts;
  Greeter::LogindPowerService power;
  Greeter::SystemFileSystem files;
  Greeter::CompositorAdapter compositor(config.value);
  Greeter::Controller controller(demo, scenario, config.value,
                                 parser.value("state"), &transport, &accounts,
                                 &power, &files);
  QObject::connect(&controller, &Greeter::Controller::sessionStarted, &app,
                   [&app] {
                     qCInfo(greeterLifecycle) << "session-started";
                     app.quit();
                   });
  QObject::connect(&app, &QCoreApplication::aboutToQuit,
                   [] { qCInfo(greeterLifecycle) << "application-exiting"; });
  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { qCCritical(greeterLifecycle) << "qml-load-failed"; },
      Qt::QueuedConnection);
  engine.rootContext()->setContextProperty("greeterController", &controller);
  engine.rootContext()->setContextProperty("greeterConfigError", config.error);
  engine.rootContext()->setContextProperty("greeterConfigWarning",
                                           config.warning);
  engine.rootContext()->setContextProperty("greeterDemo", demo);
  engine.rootContext()->setContextProperty("greeterCompositor", &compositor);
  engine.rootContext()->setContextProperty("greeterBackground",
                                           config.value.background);
  engine.rootContext()->setContextProperty("greeterMachineName",
                                           QSysInfo::machineHostName());
  engine.loadFromModule("Holonight.Greeter", "Main");
  if (engine.rootObjects().isEmpty()) {
    qCCritical(greeterLifecycle) << "qml-root-missing";
    return 1;
  }
  qCInfo(greeterLifecycle) << "qml-loaded";
  std::unique_ptr<Greeter::OutputManager> outputs;
  if (!demo)
    outputs = std::make_unique<Greeter::OutputManager>(
        qobject_cast<QWindow *>(engine.rootObjects().first()),
        config.value.primaryOutput, config.value.background);
  const int result = app.exec();
  qCInfo(greeterLifecycle) << "event-loop-finished" << result;
  return result;
}
