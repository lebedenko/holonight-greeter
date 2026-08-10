#include "config.h"
#include "controller.h"
#include "greetdclient.h"
#include "services.h"
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSysInfo>

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setApplicationName("holonight-greeter");
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOption(
      {"config", "Configuration file", "path", "/etc/holonight/greeter.toml"});
  parser.addOption(
      {"state", "State file", "path", "/var/lib/holonight-greeter/state.json"});
  parser.addOption({"demo", "Run windowed without system services"});
  parser.addOption({"demo-scenario", "Demo scenario", "name", "default"});
  parser.process(app);
  const QString scenario = parser.value("demo-scenario");
  if (!QStringList{"default", "wrong-password", "otp", "fingerprint"}.contains(
          scenario))
    parser.showHelp(2);

  const bool demo = parser.isSet("demo") || parser.isSet("demo-scenario");
  const auto config = demo ? Greeter::ConfigResult{}
                           : Greeter::loadConfig(parser.value("config"));
  Greeter::GreetdClient transport;
  Greeter::SystemAccountSource accounts;
  Greeter::LogindPowerService power;
  Greeter::SystemFileSystem files;
  Greeter::Controller controller(demo, scenario, config.value,
                                 parser.value("state"), &transport, &accounts,
                                 &power, &files);
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("greeterController", &controller);
  engine.rootContext()->setContextProperty("greeterConfigError", config.error);
  engine.rootContext()->setContextProperty("greeterConfigWarning",
                                           config.warning);
  engine.rootContext()->setContextProperty("greeterDemo", demo);
  engine.rootContext()->setContextProperty("greeterKeyboardLabel",
                                           config.value.keyboardLabel);
  engine.rootContext()->setContextProperty("greeterBackground",
                                           config.value.background);
  engine.rootContext()->setContextProperty("greeterMachineName",
                                           QSysInfo::machineHostName());
  engine.loadFromModule("Holonight.Greeter", "Main");
  return engine.rootObjects().isEmpty() ? 1 : app.exec();
}
