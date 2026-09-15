#include "compositoradapter.h"
#include "controller.h"
#include "fakes.h"
#include <QDir>
#include <QFile>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/private/qqmlcontextdata_p.h>
#include <QtQml/private/qqmldata_p.h>
#include <gtest/gtest.h>

namespace {
bool hasOrigin(QObject *object, const QString &suffix) {
  auto *data = QQmlData::get(object);
  for (auto *context = data ? data->context : nullptr; context;
       context = context->parent().data()) {
    if (context->url().toString().endsWith(suffix)) {
      qInfo().noquote() << "CONTROL_IMPLEMENTATION" << context->url();
      return true;
    }
  }
  return false;
}

class RuntimeControls : public testing::Test {
protected:
  void load(bool manual = false, const QString &error = {},
            bool multiple = true) {
    config.userMode = manual ? Greeter::Config::UserMode::Manual
                             : Greeter::Config::UserMode::List;
    config.keyboardLayouts = {{"us", "us", {}, "English"},
                              {"de", "de", {}, "German"}};
    if (multiple)
      accounts.records += {"bob", "Bob", {}, 1001};
    for (int i = 0; i < 25; ++i)
      files.records += {QString("session-%1").arg(i),
                        QString("Session %1").arg(i),
                        {"fake-session"}};
    controller = std::make_unique<Greeter::Controller>(
        false, QString{}, config, temporary.filePath("state.json"), &transport,
        &accounts, &power, &files);
    compositor = std::make_unique<Greeter::CompositorAdapter>(config);
    engine = std::make_unique<QQmlApplicationEngine>();
    engine->addImportPath(HOLONIGHT_QML_IMPORT_PATH);
    QObject::connect(engine.get(), &QQmlEngine::warnings, engine.get(),
                     [this](const QList<QQmlError> &errors) {
                       for (const auto &entry : errors)
                         diagnostics += entry.toString();
                     });
    auto *context = engine->rootContext();
    context->setContextProperty("greeterController", controller.get());
    context->setContextProperty("greeterCompositor", compositor.get());
    context->setContextProperty("greeterConfigError", error);
    context->setContextProperty("greeterConfigWarning", QString{});
    context->setContextProperty("greeterDemo", true);
    context->setContextProperty("greeterBackground", QString{});
    context->setContextProperty("greeterMachineName", "Acceptance");
    engine->loadFromModule("Holonight.Greeter", "Main");
    ASSERT_EQ(engine->rootObjects().size(), 1)
        << qPrintable(diagnostics.join('\n'));
    window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    ASSERT_NE(window, nullptr);
    QCoreApplication::processEvents();
  }
  void TearDown() override {
    engine.reset();
    EXPECT_TRUE(diagnostics.isEmpty()) << qPrintable(diagnostics.join('\n'));
  }
  QObject *object(const char *name) {
    auto *result = window->findChild<QObject *>(QString::fromLatin1(name));
    EXPECT_NE(result, nullptr) << name;
    return result;
  }
  QQuickItem *focusItem(const char *name) {
    auto *item = qobject_cast<QQuickItem *>(object(name));
    if (QString::fromLatin1(name).endsWith("Button") &&
        (QString::fromLatin1(name) == "rebootButton" ||
         QString::fromLatin1(name) == "powerButton"))
      return evaluate(item, "focusTarget").value<QQuickItem *>();
    return item;
  }
  void checkCycle(const QStringList &names) {
    auto *first = focusItem(qPrintable(names.first()));
    ASSERT_NE(first, nullptr);
    first->forceActiveFocus(Qt::TabFocusReason);
    for (int direction : {1, -1}) {
      for (int step = 1; step <= names.size(); ++step) {
        QTest::keyClick(window, direction == 1 ? Qt::Key_Tab : Qt::Key_Backtab);
        const auto name =
            names[(direction * step + names.size()) % names.size()];
        EXPECT_EQ(window->activeFocusItem(), focusItem(qPrintable(name)))
            << qPrintable(name) << " direction " << direction;
      }
    }
  }
  QVariant evaluate(QObject *target, const QString &expression) {
    QQmlExpression expr(qmlContext(target), target, expression);
    auto result = expr.evaluate();
    EXPECT_FALSE(expr.hasError()) << qPrintable(expr.error().toString());
    return result;
  }
  void activate(const char *name, int index) {
    auto *combo = object(name);
    ASSERT_TRUE(combo->setProperty("currentIndex", index));
    ASSERT_TRUE(
        QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, index)));
    QCoreApplication::processEvents();
  }
  void prompt(const char *kind = "secret", const char *message = "Password") {
    transport.reply({{"type", "auth_message"},
                     {"auth_message_type", kind},
                     {"auth_message", message}});
    QCoreApplication::processEvents();
  }
  QTemporaryDir temporary;
  Greeter::Config config;
  FakeTransport transport;
  FakeAccounts accounts;
  FakePower power;
  FakeFiles files;
  std::unique_ptr<Greeter::Controller> controller;
  std::unique_ptr<Greeter::CompositorAdapter> compositor;
  QStringList diagnostics;
  std::unique_ptr<QQmlApplicationEngine> engine;
  QQuickWindow *window = nullptr;
};

TEST_F(RuntimeControls, KeyboardRevealHoldReleaseAndFocusLoss) {
  load();
  transport.connectNow();
  prompt();
  auto *response = object("responseField");
  auto *reveal = qobject_cast<QQuickItem *>(object("revealButton"));
  response->setProperty("text", "disposable reveal sample");
  const auto sent = transport.sent.size();
  reveal->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyPress(window, Qt::Key_Space);
  EXPECT_EQ(response->property("echoMode").toInt(), 0);
  QKeyEvent repeatRelease(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier,
                          QString{}, true);
  QCoreApplication::sendEvent(window, &repeatRelease);
  EXPECT_EQ(response->property("echoMode").toInt(), 0);
  QTest::keyRelease(window, Qt::Key_Space);
  EXPECT_EQ(response->property("echoMode").toInt(), 2);
  QTest::keyPress(window, Qt::Key_Space);
  EXPECT_EQ(response->property("echoMode").toInt(), 0);
  qobject_cast<QQuickItem *>(response)->forceActiveFocus(Qt::TabFocusReason);
  EXPECT_EQ(response->property("echoMode").toInt(), 2);
  QTest::keyRelease(window, Qt::Key_Space);
  EXPECT_EQ(transport.sent.size(), sent);
}

TEST_F(RuntimeControls, KeyboardCycleIncludesAccountBothDirections) {
  load();
  transport.connectNow();
  prompt();
  power.capabilitiesNow(true, true);
  QCoreApplication::processEvents();
  auto *response = qobject_cast<QQuickItem *>(object("responseField"));
  response->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(window, Qt::Key_Backtab);
  EXPECT_EQ(window->activeFocusItem(), object("userSelector"));
  QTest::keyClick(window, Qt::Key_Tab);
  EXPECT_EQ(window->activeFocusItem(), response);
}

TEST_F(RuntimeControls, CompleteCycleSkipsCompactAndDisabledActions) {
  load();
  transport.connectNow();
  prompt();
  power.capabilitiesNow(true, true);
  QCoreApplication::processEvents();
  const QStringList inputs{"userSelector",    "responseField",
                           "revealButton",    "primaryButton",
                           "sessionSelector", "keyboardSelector"};
  checkCycle(inputs + QStringList{"rebootButton", "powerButton"});
  window->resize(850, 941);
  QCoreApplication::processEvents();
  checkCycle(inputs);
  window->resize(1672, 941);
  power.capabilitiesNow(false, true);
  QCoreApplication::processEvents();
  checkCycle(inputs + QStringList{"rebootButton"});
  object("keyboardSelector")->setProperty("enabled", false);
  power.capabilitiesNow(false, false);
  QCoreApplication::processEvents();
  checkCycle(inputs.sliced(0, 5));
}

TEST_F(RuntimeControls, SingleAccountAndManualCycles) {
  load(false, {}, false);
  transport.connectNow();
  prompt();
  checkCycle({"responseField", "revealButton", "primaryButton",
              "sessionSelector", "keyboardSelector"});
}

TEST_F(RuntimeControls, ManualUsernameCycleAndPromptFocus) {
  load(true);
  checkCycle({"usernameField", "sessionSelector", "keyboardSelector"});
  object("usernameField")->setProperty("text", "alice");
  checkCycle({"usernameField", "primaryButton", "sessionSelector",
              "keyboardSelector"});
  QMetaObject::invokeMethod(object("usernameField"), "accepted");
  transport.connectNow();
  prompt();
  ASSERT_TRUE(QTest::qWaitFor(
      [&] { return window->activeFocusItem() == object("responseField"); }));
  checkCycle({"responseField", "revealButton", "primaryButton",
              "sessionSelector", "keyboardSelector"});
}

TEST_F(RuntimeControls, RevealCancellationHidingDisablingAndDeactivation) {
  load();
  transport.connectNow();
  prompt();
  auto *response = object("responseField");
  auto *reveal = focusItem("revealButton");
  const auto sent = transport.sent.size();
  for (const auto &cancel : {QString("canceled()"), QString("enabled = false"),
                             QString("visible = false")}) {
    reveal->setProperty("enabled", true);
    reveal->setProperty("visible", true);
    reveal->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyPress(window, Qt::Key_Space);
    ASSERT_EQ(response->property("echoMode").toInt(), 0);
    evaluate(reveal, cancel);
    EXPECT_EQ(response->property("echoMode").toInt(), 2);
    QTest::keyRelease(window, Qt::Key_Space);
  }
  reveal->setProperty("visible", true);
  reveal->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyPress(window, Qt::Key_Space);
  QWindow other;
  other.show();
  other.requestActivate();
  ASSERT_TRUE(QTest::qWaitFor([&] { return !window->isActive(); }));
  EXPECT_EQ(response->property("echoMode").toInt(), 2);
  QTest::keyRelease(window, Qt::Key_Space);
  other.hide();
  window->requestActivate();
  ASSERT_TRUE(QTest::qWaitFor([&] { return window->isActive(); }));
  reveal->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(window, Qt::Key_Return);
  EXPECT_EQ(transport.sent.size(), sent);
  // Mouse events are delivered directly to the isolated offscreen test window.
  const auto point =
      reveal->mapToScene(QPointF(reveal->width() / 2, reveal->height() / 2))
          .toPoint();
  QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
  EXPECT_EQ(response->property("echoMode").toInt(), 0);
  QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
  EXPECT_EQ(response->property("echoMode").toInt(), 2);
  EXPECT_EQ(transport.sent.size(), sent);
}

TEST_F(RuntimeControls, AccountRowsFallbackLongNamesAndSelectionFocus) {
  accounts.records[0].displayName = QString(160, 'A');
  load();
  transport.connectNow();
  prompt();
  auto *selector = object("userSelector");
  EXPECT_TRUE(hasOrigin(selector, "/Holonight/Controls/HnIconComboBox.qml"));
  EXPECT_EQ(object("userAvatar")->property("width").toDouble(), 132);
  EXPECT_GT(evaluate(selector, "indicator.width").toDouble(), 0);
  evaluate(selector, "popup.open()");
  ASSERT_TRUE(QTest::qWaitFor(
      [&] { return evaluate(selector, "popup.opened").toBool(); }));
  auto *row =
      evaluate(selector, "popup.contentItem.currentItem").value<QObject *>();
  ASSERT_NE(row, nullptr);
  EXPECT_TRUE(
      hasOrigin(row, qEnvironmentVariable("QT_QUICK_CONTROLS_STYLE") == "Fusion"
                         ? "/Fusion/ItemDelegate.qml"
                         : "/Holonight/ItemDelegate.qml"));
  EXPECT_TRUE(evaluate(row, "contentItem.children[1].truncated").toBool());
  EXPECT_TRUE(evaluate(row, "contentItem.children[0].fallbackSource.toString()."
                            "endsWith('no-avatar.png')")
                  .toBool());
  EXPECT_LE(row->property("width").toDouble(),
            selector->property("width").toDouble());
  evaluate(selector, "popup.close()");
  activate("userSelector", 1);
  transport.reply({{"type", "success"}});
  transport.connectNow();
  EXPECT_EQ(transport.sent.last().value("username"), "bob");
  prompt();
  ASSERT_TRUE(QTest::qWaitFor(
      [&] { return window->activeFocusItem() == object("responseField"); }));
  activate("sessionSelector", 1);
  EXPECT_EQ(window->activeFocusItem(), object("responseField"));
}

TEST_F(RuntimeControls, SemanticDisabledFooterAndPowerPresentation) {
  load();
  auto *footer = object("keyboardSelector");
  footer->setProperty("enabled", false);
  evaluate(footer, "down = true");
  EXPECT_EQ(evaluate(footer, "background.color").value<QColor>().alpha(), 0);
  EXPECT_EQ(evaluate(footer, "background.border.width").toInt(), 0);
  EXPECT_EQ(evaluate(footer, "contentItem.children[0].color"),
            evaluate(footer, "contentItem.children[1].color"));
  EXPECT_EQ(evaluate(footer, "indicator.color"),
            evaluate(footer, "contentItem.children[1].color"));
  power.capabilitiesNow(true, true);
  auto *reboot = object("rebootButton");
  auto *poweroff = object("powerButton");
  EXPECT_EQ(reboot->property("width"), poweroff->property("width"));
  EXPECT_NEAR(evaluate(poweroff, "focusTarget.font.pointSize").toDouble() /
                  evaluate(reboot, "focusTarget.font.pointSize").toDouble(),
              1.25, 0.01);
  for (auto *action : {reboot, poweroff}) {
    EXPECT_EQ(evaluate(action, "children[0].data[0].fillColor")
                  .value<QColor>()
                  .alpha(),
              0);
    EXPECT_GT(evaluate(action, "children[0].data[0].strokeWidth").toDouble(),
              0);
  }
  EXPECT_EQ(power.offs, 0);
  EXPECT_EQ(power.reboots, 0);
}

TEST_F(RuntimeControls, PasswordInheritsSelectedStyleAndRendersStates) {
  load();
  transport.connectNow();
  prompt();
  auto *response = focusItem("responseField");
  const bool fusion =
      qEnvironmentVariable("QT_QUICK_CONTROLS_STYLE") == "Fusion";
  EXPECT_TRUE(hasOrigin(response, fusion ? "/Fusion/TextField.qml"
                                         : "/Holonight/TextField.qml"));
  for (bool enabled : {true, false}) {
    response->setEnabled(enabled);
    for (bool focused : {false, true}) {
      if (focused && enabled)
        response->forceActiveFocus(Qt::TabFocusReason);
      else
        focusItem("sessionSelector")->forceActiveFocus(Qt::TabFocusReason);
      QCoreApplication::processEvents();
      EXPECT_EQ(evaluate(response, "color").value<QColor>(),
                evaluate(response,
                         fusion
                             ? "palette.text"
                             : (enabled ? "controlColors.colors.textPrimary"
                                        : "controlColors.colors.textDisabled"))
                    .value<QColor>());
      EXPECT_EQ(evaluate(response, "selectionColor").value<QColor>(),
                evaluate(response, fusion ? "palette.highlight"
                                          : "controlColors.colors.selection")
                    .value<QColor>());
      ASSERT_TRUE(QTest::qWaitFor(
          [&] { return response->width() > 0 && response->height() > 0; }));
      const auto grab = response->grabToImage();
      ASSERT_FALSE(grab.isNull());
      QSignalSpy ready(grab.data(), &QQuickItemGrabResult::ready);
      ASSERT_TRUE(ready.wait());
      const auto image = grab->image();
      ASSERT_FALSE(image.isNull());
      const QColor pixel =
          image.pixelColor(image.width() / 3, image.height() / 2);
      EXPECT_GT(pixel.alpha(), 0);
      qInfo() << "PASSWORD_RENDER" << (fusion ? "Fusion" : "Holonight") << "dpr"
              << window->devicePixelRatio() << "enabled" << enabled << "focus"
              << response->hasActiveFocus() << "pixel" << pixel << "text"
              << evaluate(response, "color").value<QColor>();
    }
  }
}

TEST_F(RuntimeControls, SelectedImplementationsAndCompositePainting) {
  load();
  ASSERT_NE(window, nullptr);
  const QString prefix =
      qEnvironmentVariable("QT_QUICK_CONTROLS_STYLE") == "Fusion"
          ? "/QtQuick/Controls/Fusion/"
          : "/Holonight/";
  EXPECT_TRUE(hasOrigin(window, prefix + "ApplicationWindow.qml"));
  EXPECT_TRUE(hasOrigin(object("userSelector"), prefix + "ComboBox.qml"));
  EXPECT_TRUE(hasOrigin(object("responseField"), prefix + "TextField.qml"));
  EXPECT_TRUE(hasOrigin(object("primaryButton"), prefix + "Button.qml"));
  EXPECT_TRUE(hasOrigin(object("sessionSelector"),
                        "/Holonight/Controls/HnIconComboBox.qml"));
  EXPECT_TRUE(hasOrigin(object("sessionSelector"), prefix + "ComboBox.qml"));
  EXPECT_TRUE(
      hasOrigin(object("userAvatar"), "/Holonight/Controls/HnAvatar.qml"));
  EXPECT_EQ(window->color(), QColor("#050b18"));
  auto *selector = object("sessionSelector");
  EXPECT_EQ(selector->property("iconRole").toString(), "");
  EXPECT_EQ(evaluate(selector, "contentItem.children[0].text").toString(),
            QString::fromUtf8("▱"));
  QFile maps("/proc/self/maps");
  ASSERT_TRUE(maps.open(QIODevice::ReadOnly));
  auto contents = maps.readAll();
  const QByteArray root = QByteArray(HOLONIGHT_QML_IMPORT_PATH) + "/Holonight/";
  for (auto module : {"Core/libholonight_core_qml.so",
                      "Controls/libholonight_controls_qml.so"})
    EXPECT_TRUE(contents.contains(root + module));
  EXPECT_EQ(contents.contains(root + "libholonight_qml.so"),
            prefix == "/Holonight/");
}

TEST_F(RuntimeControls, UserSessionPasswordOtpFingerprintAndCancellation) {
  load();
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(controller->state(), "connecting");
  transport.connectNow();
  prompt();
  auto *response = object("responseField");
  EXPECT_TRUE(response->property("visible").toBool());
  EXPECT_EQ(response->property("echoMode").toInt(), 2);
  response->setProperty("text", "test-password");
  EXPECT_NE(response->property("displayText").toString(), "test-password");
  QMetaObject::invokeMethod(response, "accepted");
  EXPECT_EQ(transport.sent.last().value("response"), "test-password");
  EXPECT_TRUE(response->property("text").toString().isEmpty());
  prompt("visible", "OTP");
  EXPECT_EQ(response->property("echoMode").toInt(), 0);
  response->setProperty("text", "123456");
  QMetaObject::invokeMethod(object("primaryButton"), "clicked");
  EXPECT_EQ(transport.sent.last().value("response"), "123456");
  prompt("info", "Touch fingerprint sensor");
  EXPECT_EQ(controller->state(), "informational-prompt");
  EXPECT_FALSE(response->property("visible").toBool());
  controller->restartAuthentication();
  EXPECT_EQ(transport.cancellations, 1);
  transport.reply({{"type", "success"}});
  transport.connectNow();
  prompt();
  activate("userSelector", 1);
  transport.reply({{"type", "success"}});
  transport.connectNow();
  EXPECT_EQ(transport.sent.last().value("username"), "bob");
  activate("sessionSelector", 3);
  EXPECT_EQ(controller->selectedSession(), "session-2");
  prompt();
  controller->respond("wrong");
  transport.reply({{"type", "error"},
                   {"error_type", "auth_error"},
                   {"description", "backend detail"}});
  EXPECT_FALSE(controller->status().contains("backend detail"));
  transport.reply({{"type", "success"}});
  QMetaObject::invokeMethod(object("primaryButton"), "clicked");
  transport.connectNow();
  prompt();
  EXPECT_EQ(controller->state(), "input-prompt");
  EXPECT_TRUE(response->property("visible").toBool());
}

TEST_F(RuntimeControls,
       SuccessfulAuthenticationDisablesSelectorsAndPersistsFakeSession) {
  load();
  ASSERT_NE(window, nullptr);
  transport.connectNow();
  prompt();
  auto *response = object("responseField");
  response->setProperty("text", "test-password");
  QMetaObject::invokeMethod(object("primaryButton"), "clicked");
  transport.reply({{"type", "success"}});
  EXPECT_EQ(controller->state(), "starting");
  EXPECT_FALSE(object("userSelector")->property("enabled").toBool());
  EXPECT_FALSE(object("sessionSelector")->property("enabled").toBool());
  EXPECT_EQ(transport.sent.last().value("type"), "start_session");
  EXPECT_EQ(files.saveCalls, 0);
  QSignalSpy started(controller.get(), &Greeter::Controller::sessionStarted);
  transport.reply({{"type", "success"}});
  EXPECT_EQ(started.count(), 1);
  EXPECT_EQ(files.saveCalls, 1);
  EXPECT_EQ(controller->state(), "authenticated");
}

TEST_F(RuntimeControls, ManualEntryAndConfigurationErrors) {
  load(true);
  ASSERT_NE(window, nullptr);
  auto *username = object("usernameField");
  EXPECT_TRUE(username->property("visible").toBool());
  EXPECT_FALSE(object("primaryButton")->property("enabled").toBool());
  username->setProperty("text", "manual-user");
  EXPECT_TRUE(object("primaryButton")->property("enabled").toBool());
  QMetaObject::invokeMethod(username, "accepted");
  transport.connectNow();
  EXPECT_EQ(transport.sent.last().value("username"), "manual-user");
}

TEST_F(RuntimeControls, InvalidConfigurationDisablesAuthenticationAndSessions) {
  load(true, "Invalid configuration");
  ASSERT_NE(window, nullptr);
  EXPECT_FALSE(object("usernameField")->property("enabled").toBool());
  EXPECT_FALSE(object("primaryButton")->property("enabled").toBool());
  EXPECT_FALSE(object("sessionSelector")->property("enabled").toBool());
  EXPECT_TRUE(transport.sent.isEmpty());
}

TEST_F(RuntimeControls, PowerConfirmationUsesOnlyFakeService) {
  load();
  ASSERT_NE(window, nullptr);
  EXPECT_FALSE(object("rebootButton")->property("enabled").toBool());
  power.capabilitiesNow(true, true);
  EXPECT_TRUE(object("rebootButton")->property("enabled").toBool());
  QMetaObject::invokeMethod(object("rebootButton"), "clicked");
  EXPECT_EQ(window->property("pendingPowerAction").toString(), "reboot");
  EXPECT_EQ(power.reboots, 0);
  QMetaObject::invokeMethod(window, "cancelPowerConfirmation");
  EXPECT_TRUE(window->property("pendingPowerAction").toString().isEmpty());
  QMetaObject::invokeMethod(object("powerButton"), "clicked");
  QMetaObject::invokeMethod(window, "confirmPowerAction");
  ASSERT_TRUE(QTest::qWaitFor([&] { return power.offs == 1; }));
  EXPECT_EQ(power.reboots, 0);
}

TEST_F(RuntimeControls,
       KeyboardSelectorCapturesIpcAndPreservesSelectionOnFailure) {
  load();
  ASSERT_NE(window, nullptr);
  const auto oldPath = qgetenv("PATH");
  QFile script(temporary.filePath("hyprctl"));
  ASSERT_TRUE(script.open(QIODevice::WriteOnly));
  script.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$0.args\"\nexit 0\n");
  script.close();
  ASSERT_TRUE(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner |
                                    QFile::ExeOwner));
  qputenv("PATH", temporary.path().toUtf8());
  activate("keyboardSelector", 1);
  EXPECT_EQ(compositor->keyboardLayoutId(), "de");
  QFile args(script.fileName() + ".args");
  ASSERT_TRUE(args.open(QIODevice::ReadOnly));
  EXPECT_EQ(args.readAll(), "switchxkblayout\nall\n1\n");
  ASSERT_TRUE(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
  script.write("#!/bin/sh\nexit 1\n");
  script.close();
  activate("keyboardSelector", 0);
  EXPECT_EQ(compositor->keyboardLayoutId(), "de");
  EXPECT_EQ(object("keyboardSelector")->property("currentIndex").toInt(), 1);
  qputenv("PATH", oldPath);
}

TEST_F(RuntimeControls, ResponsiveLayoutAndScaledPopupOverflowReachability) {
  load();
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->size(), QSize(1672, 941));
  auto *selector = object("sessionSelector");
  for (const QSize size : {QSize(1672, 941), QSize(899, 941), QSize(900, 941),
                           QSize(2200, 1200)}) {
    window->resize(size);
    QCoreApplication::processEvents();
    EXPECT_EQ(window->property("compact").toBool(), size.width() < 900);
    for (double scale : {0.78, 1.0, 1.25}) {
      object("loginPanel")->setProperty("scale", scale);
      for (bool above : {false, true}) {
        QCoreApplication::processEvents();
        const double targetY = above ? size.height() - 56 * scale - 10 : 10;
        auto *panel = object("loginPanel");
        panel->setProperty("y", panel->property("y").toDouble() + targetY -
                                    qobject_cast<QQuickItem *>(selector)
                                        ->mapToScene(QPointF{})
                                        .y());
        auto *popup = evaluate(selector, "popup").value<QObject *>();
        ASSERT_NE(popup, nullptr);
        evaluate(selector, "currentIndex = 24; popup.open()");
        ASSERT_TRUE(QTest::qWaitFor(
            [&] { return popup->property("opened").toBool(); }));
        EXPECT_NEAR(selector->property("effectiveScale").toDouble(), scale,
                    0.01);
        EXPECT_EQ(selector->property("delegateHeight"),
                  selector->property("height"));
        EXPECT_NEAR(popup->property("scale").toDouble(), scale, 0.01);
        EXPECT_EQ(popup->property("opensAbove").toBool(), above);
        const double renderedTop =
            popup->property("y").toDouble() +
            (above ? popup->property("height").toDouble() * (1 - scale) : 0);
        EXPECT_GE(renderedTop, 0);
        EXPECT_LE(renderedTop + popup->property("height").toDouble() * scale,
                  size.height());
        auto *list = evaluate(selector, "popup.contentItem").value<QObject *>();
        ASSERT_NE(list, nullptr);
        ASSERT_TRUE(QTest::qWaitFor([&] {
          return list->property("contentHeight").toDouble() >
                 list->property("height").toDouble();
        }));
        EXPECT_TRUE(list->property("interactive").toBool());
        EXPECT_EQ(list->property("currentIndex").toInt(), 24);
        EXPECT_TRUE(evaluate(list,
                             "currentItem.y >= contentY - 1 && currentItem.y + "
                             "currentItem.height <= contentY + height + 1")
                        .toBool());
        EXPECT_EQ(evaluate(list, "currentItem.height").toDouble(),
                  selector->property("height").toDouble());
        EXPECT_TRUE(
            evaluate(list, "currentItem.iconSource.toString().length === 0")
                .toBool());
        evaluate(list, "positionViewAtIndex(0, ListView.Beginning)");
        ASSERT_TRUE(QTest::qWaitFor(
            [&] { return list->property("contentY").toDouble() <= 0.1; }));
        evaluate(list, "positionViewAtIndex(count - 1, ListView.End)");
        ASSERT_TRUE(QTest::qWaitFor([&] {
          return list->property("contentY").toDouble() +
                     list->property("height").toDouble() >=
                 list->property("contentHeight").toDouble() - 1;
        }));
        evaluate(selector, "popup.close()");
        ASSERT_TRUE(QTest::qWaitFor(
            [&] { return !popup->property("visible").toBool(); }));
      }
    }
  }
}

TEST(Wallpaper, SecondaryEngineLoadsTemporaryImageWithoutProviderDiscovery) {
  QTemporaryDir temporary;
  QImage image(32, 32, QImage::Format_RGB32);
  image.fill(Qt::blue);
  ASSERT_TRUE(image.save(temporary.filePath("wallpaper.png")));
  QQmlEngine engine;
  auto paths = engine.importPathList();
  paths.removeAll(QStringLiteral(HOLONIGHT_QML_IMPORT_PATH));
  engine.setImportPathList(paths);
  QQmlComponent component(
      &engine, QUrl("qrc:/qt/qml/Holonight/Greeter/qml/Background.qml"));
  std::unique_ptr<QObject> root(component.createWithInitialProperties(
      {{"demo", false},
       {"backgroundPath", temporary.filePath("wallpaper.png")}}));
  ASSERT_NE(root, nullptr) << qPrintable(component.errorString());
  ASSERT_TRUE(QTest::qWaitFor(
      [&] { return root->property("backgroundLoaded").toBool(); }));
  EXPECT_FALSE(root->property("backgroundLoadFailed").toBool());
}
} // namespace
