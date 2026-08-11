#include "outputmanager.h"
#include "compositoradapter.h"
#include <LayerShellQt/Window>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QWindow>
#include <ranges>
Q_LOGGING_CATEGORY(outputLog, "holonight.greeter.outputs")
namespace Greeter {
namespace {
void configureLayerSurface(
    QWindow *window, QScreen *screen, LayerShellQt::Window::Layer layer,
    LayerShellQt::Window::KeyboardInteractivity keyboard) {
  window->setScreen(screen);
  auto *surface = LayerShellQt::Window::get(window);
  surface->setScreen(screen);
  surface->setWantsToBeOnActiveScreen(false);
  LayerShellQt::Window::Anchors anchors;
  anchors.setFlag(LayerShellQt::Window::AnchorTop);
  anchors.setFlag(LayerShellQt::Window::AnchorBottom);
  anchors.setFlag(LayerShellQt::Window::AnchorLeft);
  anchors.setFlag(LayerShellQt::Window::AnchorRight);
  surface->setAnchors(anchors);
  surface->setExclusiveZone(-1);
  surface->setLayer(layer);
  surface->setKeyboardInteractivity(keyboard);
  surface->setScope("holonight-greeter");
}
} // namespace
OutputManager::OutputManager(QWindow *window, QString output,
                             QString background, QObject *parent)
    : QObject(parent), interactiveWindow_(window),
      configuredOutput_(std::move(output)), background_(std::move(background)) {
  if (interactiveWindow_) {
    connect(interactiveWindow_, SIGNAL(backgroundReady()), this,
            SLOT(backgroundReady()));
    connect(interactiveWindow_, SIGNAL(backgroundFailed()), this,
            SLOT(backgroundFailed()));
  }
  connect(qGuiApp, &QGuiApplication::screenAdded, this,
          &OutputManager::refresh);
  connect(qGuiApp, &QGuiApplication::screenRemoved, this,
          &OutputManager::refresh);
  connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this,
          &OutputManager::refresh);
  refresh();
}
OutputManager::~OutputManager() {
  for (const auto *background : backgrounds_)
    qCInfo(outputLog) << "surface-teardown" << "wallpaper"
                      << (background->screen() ? background->screen()->name()
                                               : QString{});
  qDeleteAll(backgrounds_);
  if (interactiveWindow_)
    qCInfo(outputLog) << "surface-teardown" << "interactive"
                      << interactiveOutput();
}
QString OutputManager::interactiveOutput() const {
  return interactiveWindow_ && interactiveWindow_->screen()
             ? interactiveWindow_->screen()->name()
             : QString{};
}
void OutputManager::refresh() {
  const QString previous = interactiveOutput();
  if (interactiveWindow_)
    interactiveWindow_->hide();
  for (const auto *background : backgrounds_)
    qCInfo(outputLog) << "surface-teardown" << "wallpaper"
                      << (background->screen() ? background->screen()->name()
                                               : QString{});
  qDeleteAll(backgrounds_);
  backgrounds_.clear();
  if (!interactiveWindow_)
    return;
  QStringList names;
  for (const auto *screen : qGuiApp->screens())
    names += screen->name();
  const QString primary =
      qGuiApp->primaryScreen() ? qGuiApp->primaryScreen()->name() : QString{};
  const auto plan = planOutputs(names, configuredOutput_, primary);
  const QString selected =
      selectInteractiveOutput(names, configuredOutput_, primary);
  const auto screens = qGuiApp->screens();
  qCInfo(outputLog) << "outputs-discovered" << names;
  if (!previous.isEmpty() && previous != selected)
    qCInfo(outputLog) << "hotplug-reassignment" << previous << selected;
  for (const auto &assignment : plan) {
    const auto screenIt =
        std::ranges::find_if(screens, [&](const auto *candidate) {
          return candidate->name() == assignment.name;
        });
    if (screenIt == screens.end())
      continue;
    auto *screen = *screenIt;
    qCInfo(outputLog) << "output-role" << assignment.name
                      << (assignment.role == OutputRole::Interactive
                              ? "interactive"
                              : "wallpaper");
    if (assignment.role == OutputRole::Interactive) {
      configureLayerSurface(
          interactiveWindow_, screen, LayerShellQt::Window::LayerTop,
          LayerShellQt::Window::KeyboardInteractivityExclusive);
    } else {
      auto *background = new QQuickView;
      configureLayerSurface(background, screen,
                            LayerShellQt::Window::LayerBackground,
                            LayerShellQt::Window::KeyboardInteractivityNone);
      background->setResizeMode(QQuickView::SizeRootObjectToView);
      background->setColor(QColor("#050b18"));
      background->setInitialProperties(
          {{"demo", false}, {"backgroundPath", background_}});
      background->setSource(
          QUrl("qrc:/qt/qml/Holonight/Greeter/qml/Background.qml"));
      if (background->status() == QQuickView::Error) {
        qCCritical(outputLog) << "wallpaper-component-error" << assignment.name;
        QCoreApplication::exit(1);
      } else if (background->rootObject()) {
        connect(background->rootObject(), SIGNAL(backgroundReady()), this,
                SLOT(backgroundReady()));
        connect(background->rootObject(), SIGNAL(backgroundFailed()), this,
                SLOT(backgroundFailed()));
        if (background->rootObject()
                ->property("backgroundLoadFailed")
                .toBool()) {
          qCCritical(outputLog) << "wallpaper-image-error" << assignment.name;
          QCoreApplication::exit(1);
        }
      }
      backgrounds_ += background;
    }
  }
  showSurfacesIfReady();
}
void OutputManager::backgroundReady() { showSurfacesIfReady(); }
void OutputManager::backgroundFailed() {
  const auto *root = sender();
  const auto found =
      std::ranges::find_if(backgrounds_, [root](const auto *view) {
        return view->rootObject() == root;
      });
  QString output = found != backgrounds_.end() && (*found)->screen()
                       ? (*found)->screen()->name()
                       : QString{};
  if (output.isEmpty())
    output = interactiveOutput();
  qCCritical(outputLog) << "wallpaper-image-error" << output;
  QCoreApplication::exit(1);
}
void OutputManager::showSurfacesIfReady() {
  if (!interactiveWindow_ || !interactiveWindow_->screen())
    return;
  if (interactiveWindow_->property("backgroundLoadFailed").toBool()) {
    qCCritical(outputLog) << "wallpaper-image-error" << interactiveOutput();
    QCoreApplication::exit(1);
    return;
  }
  if (!interactiveWindow_->property("backgroundLoaded").toBool())
    return;
  for (const auto *background : backgrounds_) {
    if (!background->rootObject() ||
        !background->rootObject()->property("backgroundLoaded").toBool())
      return;
  }
  for (auto *background : backgrounds_) {
    qCInfo(outputLog) << "component-ready" << "wallpaper"
                      << background->screen()->name();
    background->showFullScreen();
  }
  qCInfo(outputLog) << "component-ready" << "interactive"
                    << interactiveOutput();
  interactiveWindow_->showFullScreen();
}
} // namespace Greeter
