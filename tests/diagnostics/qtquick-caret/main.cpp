#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTest>

// No Controls, HoloNight, custom cursor or platform theme. Exit 1 preserves
// the visibility assertion; exit 2 means the reproduction geometry is invalid.
int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORMTHEME", "");
  QGuiApplication app(argc, argv);
  app.styleHints()->setCursorFlashTime(0);
  QQmlEngine engine;
  QQmlComponent component(&engine);
  component.setData(R"(
import QtQuick
TextInput {
    width: 420; height: 57
    transformOrigin: Item.TopLeft
    scale: 0.78
    font.family: "Inter"; font.pointSize: 14.25
    leftPadding: 54; rightPadding: 54
    topPadding: 6; bottomPadding: 6
    verticalAlignment: TextInput.AlignVCenter
    color: "white"
    echoMode: TextInput.Password
}
)",
                    QUrl("file:///qtquick-caret.qml"));
  std::unique_ptr<QObject> owner(component.create());
  auto *field = qobject_cast<QQuickItem *>(owner.get());
  if (!field) {
    qCritical() << component.errors();
    return 2;
  }
  QQuickWindow window;
  window.setColor(QColor("#141b23"));
  field->setParentItem(window.contentItem());
  const double dpr = window.devicePixelRatio();
  if (qAbs(dpr - 1) > 0.001 && qAbs(dpr - 1.25) > 0.001)
    return 2;
  const bool fractional = dpr > 1;
  const QSize size = fractional ? QSize(1021, 1257) : QSize(1276, 1571);
  const QPointF caret =
      fractional ? QPointF(543.136, 633.026) : QPointF(771.616, 789.111);
  field->setPosition(caret - QPointF(54 * 0.78, 17 * 0.78));
  window.resize(size);
  window.show();
  if (!QTest::qWaitForWindowExposed(&window))
    return 2;
  window.resize(size);
  if (!QTest::qWaitFor([&] { return window.size() == size; }))
    return 2;
  field->forceActiveFocus();
  if (!QTest::qWaitFor([&] { return field->hasActiveFocus(); }))
    return 2;
  const auto cursor = field->property("cursorRectangle").toRectF();
  const auto mapped = field->mapRectToScene(cursor);
  qInfo() << "Qt" << qVersion() << "DPR" << dpr << "window" << window.size()
          << "cursor" << cursor << "mapped" << mapped;
  if (cursor != QRectF(54, 17, 1, 23) ||
      QLineF(mapped.topLeft(), caret).length() > 0.001 ||
      qAbs(mapped.width() - 0.78) > 0.001 ||
      qAbs(mapped.height() - 17.94) > 0.001)
    return 2;
  auto capture = [&] {
    window.update();
    QCoreApplication::processEvents();
    return window.grabWindow();
  };
  QImage on;
  const QSize pixelsSize(qRound(size.width() * dpr),
                         qRound(size.height() * dpr));
  if (!QTest::qWaitFor([&] {
        on = capture();
        return window.size() == size && on.size() == pixelsSize;
      })) {
    qCritical() << "Unexpected framebuffer" << on.size() << pixelsSize;
    return 2;
  }
  field->setProperty("cursorVisible", false);
  const auto off = capture();
  if (on.isNull() || on.size() != off.size()) {
    qCritical() << "Capture mismatch" << on.size() << off.size();
    return 2;
  }
  QImage difference(on.size(), QImage::Format_RGB32);
  difference.fill(Qt::black);
  const QRect region = QRectF(mapped.topLeft() * dpr, mapped.size() * dpr)
                           .adjusted(-2, -2, 2, 2)
                           .toAlignedRect();
  int pixels = 0;
  for (int y = 0; y < on.height(); ++y)
    for (int x = 0; x < on.width(); ++x)
      if (on.pixel(x, y) != off.pixel(x, y)) {
        difference.setPixelColor(x, y, Qt::white);
        if (region.contains(x, y))
          ++pixels;
      }
  const auto path = qEnvironmentVariable("GREETER_CARET_ARTIFACTS");
  if (!path.isEmpty()) {
    if (!QDir{}.mkpath(path) || !on.save(path + "/on.png") ||
        !off.save(path + "/off.png") || !difference.save(path + "/diff.png"))
      return 2;
  }
  qInfo() << "CARET_PIXELS" << pixels;
  return pixels > 0 ? 0 : 1;
}
