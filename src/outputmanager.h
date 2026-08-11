#pragma once
#include <QObject>
#include <QPointer>
#include <QVector>
class QQuickView;
class QWindow;
namespace Greeter {
class OutputManager final : public QObject {
  Q_OBJECT
public:
  OutputManager(QWindow *interactiveWindow, QString configuredOutput,
                QString background, QObject *parent = nullptr);
  ~OutputManager() override;
  QString interactiveOutput() const;
  int credentialSurfaceCount() const { return interactiveWindow_ ? 1 : 0; }
public slots:
  void refresh();

private slots:
  void backgroundReady();
  void backgroundFailed();

private:
  void showSurfacesIfReady();
  QPointer<QWindow> interactiveWindow_;
  QString configuredOutput_;
  QString background_;
  QVector<QQuickView *> backgrounds_;
};
} // namespace Greeter
