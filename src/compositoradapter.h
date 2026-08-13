#pragma once

#include "config.h"
#include <QObject>
#include <QVariantList>

namespace Greeter {
enum class OutputRole { Interactive, Wallpaper };
struct OutputAssignment {
  QString name;
  OutputRole role;
  bool operator==(const OutputAssignment &) const = default;
};

class CompositorAdapter final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString backendName READ backendName CONSTANT)
  Q_PROPERTY(bool canCycleLayout READ canCycleLayout CONSTANT)
  Q_PROPERTY(QString keyboardLabel READ keyboardLabel NOTIFY layoutChanged)
  Q_PROPERTY(
      QString keyboardLayoutId READ keyboardLayoutId NOTIFY layoutChanged)
  Q_PROPERTY(QVariantList layouts READ layouts CONSTANT)
public:
  explicit CompositorAdapter(Config config, QObject *parent = nullptr);
  QString backendName() const { return backend_; }
  bool canCycleLayout() const;
  QString keyboardLabel() const;
  QString keyboardLayoutId() const;
  QVariantList layouts() const;
  Q_INVOKABLE bool cycleLayout();
  Q_INVOKABLE bool selectLayout(const QString &id);
signals:
  void layoutChanged();

private:
  QString backend_;
  QString legacyLabel_;
  QList<KeyboardLayout> layouts_;
  int current_ = 0;
};

[[nodiscard]] QString selectInteractiveOutput(const QStringList &connected,
                                              const QString &configured,
                                              const QString &primary);
[[nodiscard]] QList<OutputAssignment> planOutputs(const QStringList &connected,
                                                  const QString &configured,
                                                  const QString &primary);
} // namespace Greeter
