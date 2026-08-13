#include "compositoradapter.h"
#include <QProcess>
#include <algorithm>

namespace Greeter {
CompositorAdapter::CompositorAdapter(Config config, QObject *parent)
    : QObject(parent),
      backend_(qEnvironmentVariable("HOLONIGHT_GREETER_BACKEND",
                                    config.compositorBackend)),
      legacyLabel_(std::move(config.keyboardLabel)),
      layouts_(std::move(config.keyboardLayouts)) {
  if (!layouts_.isEmpty()) {
    const auto found = std::ranges::find(layouts_, config.keyboardDefault,
                                         &KeyboardLayout::id);
    if (found != layouts_.end())
      current_ = static_cast<int>(std::distance(layouts_.begin(), found));
  }
}

bool CompositorAdapter::canCycleLayout() const {
  return backend_ == "hyprland" && layouts_.size() > 1;
}
QString CompositorAdapter::keyboardLabel() const {
  return layouts_.isEmpty() ? legacyLabel_ : layouts_[current_].label;
}
QString CompositorAdapter::keyboardLayoutId() const {
  return layouts_.isEmpty() ? QString{} : layouts_[current_].id;
}
QVariantList CompositorAdapter::layouts() const {
  QVariantList result;
  if (layouts_.isEmpty() && !legacyLabel_.isEmpty())
    result += QVariantMap{{"id", QString{}}, {"label", legacyLabel_}};
  for (const auto &layout : layouts_)
    result += QVariantMap{{"id", layout.id}, {"label", layout.label}};
  return result;
}
bool CompositorAdapter::cycleLayout() {
  if (!canCycleLayout())
    return false;
  return selectLayout(layouts_[(current_ + 1) % layouts_.size()].id);
}
bool CompositorAdapter::selectLayout(const QString &id) {
  if (backend_ != "hyprland")
    return false;
  const auto found = std::ranges::find(layouts_, id, &KeyboardLayout::id);
  if (found == layouts_.end())
    return false;
  const int index = static_cast<int>(std::distance(layouts_.begin(), found));
  if (index == current_)
    return true;
  QProcess ipc;
  ipc.start("hyprctl", {"switchxkblayout", "all", QString::number(index)});
  if (!ipc.waitForStarted(1000) || !ipc.waitForFinished(2000) ||
      ipc.exitStatus() != QProcess::NormalExit || ipc.exitCode() != 0)
    return false;
  current_ = index;
  emit layoutChanged();
  return true;
}

QString selectInteractiveOutput(const QStringList &connected,
                                const QString &configured,
                                const QString &primary) {
  if (connected.contains(configured) && !configured.isEmpty())
    return configured;
  if (connected.contains(primary) && !primary.isEmpty())
    return primary;
  return connected.isEmpty() ? QString{} : connected.first();
}

QList<OutputAssignment> planOutputs(const QStringList &connected,
                                    const QString &configured,
                                    const QString &primary) {
  const QString interactive =
      selectInteractiveOutput(connected, configured, primary);
  QList<OutputAssignment> result;
  result.reserve(connected.size());
  for (const auto &name : connected)
    result += {name, name == interactive ? OutputRole::Interactive
                                         : OutputRole::Wallpaper};
  return result;
}
} // namespace Greeter
