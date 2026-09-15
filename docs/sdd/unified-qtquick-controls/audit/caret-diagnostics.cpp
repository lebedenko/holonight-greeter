// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Andrii L <lebeden@gmail.com>

// Passive, bounded observer for the G07 demo kit. Never changes input, focus,
// blinking or QML properties. Never reads text or captures populated fields.
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFont>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTimer>

namespace {
QJsonArray rect(const QRectF &value) {
  return {value.x(), value.y(), value.width(), value.height()};
}

class CaretObserver : public QObject {
public:
  explicit CaretObserver(QObject *parent) : QObject(parent) {
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer] {
      if (++samples > 1500) { // Five minutes, 5 Hz.
        timer->stop();
        return;
      }
      for (auto *window : QGuiApplication::allWindows()) {
        auto *quick = qobject_cast<QQuickWindow *>(window);
        if (!quick || !quick->isVisible())
          continue;
        auto *field = quick->findChild<QQuickItem *>("responseField");
        if (!field || !field->isVisible())
          continue;
        const bool empty = field->property("length").toInt() == 0;
        const int signature = (empty ? 1 : 0) +
                              (field->hasActiveFocus() ? 2 : 0) +
                              4 * field->property("echoMode").toInt();
        if (signature != previousSignature) {
          previousSignature = signature;
          burst = 12;
        }
        const auto cursor = field->property("cursorRectangle").toRectF();
        const auto mapped = field->mapRectToScene(cursor);
        QJsonObject state{
            {"time_ms", QDateTime::currentMSecsSinceEpoch()},
            {"empty", empty},
            {"focused", field->hasActiveFocus()},
            {"cursorVisible", field->property("cursorVisible").toBool()},
            {"echoMode", field->property("echoMode").toInt()},
            {"windowActive", quick->isActive()},
            {"window", rect(QRectF({}, quick->size()))},
            {"dpr", quick->devicePixelRatio()},
            {"cursor", rect(cursor)},
            {"mappedCursor", rect(mapped)},
            {"blinkMs", QGuiApplication::styleHints()->cursorFlashTime()}};
        for (const char *name : {"width", "height", "leftPadding",
                                 "rightPadding", "topPadding", "bottomPadding"})
          state[name] = QJsonValue::fromVariant(field->property(name));
        QJsonArray ancestors;
        for (auto *item = field; item; item = item->parentItem())
          ancestors.append(QJsonObject{
              {"name", item->objectName()},
              {"scale", item->scale()},
              {"clip", item->clip()},
              {"opacity", item->opacity()},
              {"bounds", rect(item->mapRectToScene(item->boundingRect()))},
              {"clipRect", rect(item->mapRectToScene(item->clipRect()))},
              {"itemRect",
               rect(item->mapRectToScene(QRectF({}, item->size())))}});
        state["ancestors"] = ancestors;
        const auto font = field->property("font").value<QFont>();
        state["fontFamily"] = font.family();
        state["fontPointSize"] = font.pointSizeF();
        state["effectiveScale"] = field->mapToScene(QPointF(1, 0)).x() -
                                  field->mapToScene(QPointF{}).x();
        // Only empty focused fields: retain a small full-window-rendered crop
        // across native blink phases. No text is logged or saved as pixels.
        if (empty && field->hasActiveFocus() && captures < 120 && burst-- > 0) {
          const auto image = quick->grabWindow();
          const auto dpr = quick->devicePixelRatio();
          const auto area = QRectF(mapped.x() * dpr, mapped.y() * dpr,
                                   mapped.width() * dpr, mapped.height() * dpr)
                                .adjusted(-4, -4, 4, 4)
                                .toAlignedRect();
          const auto directory = qEnvironmentVariable("GREETER_CARET_DIR");
          if (!directory.isEmpty() && !image.isNull() &&
              image.rect().contains(area) && QDir{}.mkpath(directory)) {
            const QString name = QString("empty-%1.png").arg(captures++);
            if (image.copy(area).save(directory + QLatin1Char('/') + name))
              state["capture"] = name;
          }
        }
        qInfo().noquote() << "G07_CARET"
                          << QJsonDocument(state).toJson(
                                 QJsonDocument::Compact);
      }
    });
    timer->start(200);
  }

private:
  int samples = 0;
  int captures = 0;
  int previousSignature = -1;
  int burst = 0;
};

void startCaretObserver() {
  if (!qEnvironmentVariableIsSet("GREETER_CARET_DIR"))
    return;
  QMetaObject::invokeMethod(
      qApp, [] { new CaretObserver(qApp); }, Qt::QueuedConnection);
}
Q_COREAPP_STARTUP_FUNCTION(startCaretObserver)
} // namespace
