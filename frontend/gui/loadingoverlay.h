#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QColor>
#include <QString>

QT_BEGIN_NAMESPACE
class QTimer;
class QPushButton;
QT_END_NAMESPACE

// A lightweight translucent overlay with an animated spinner and a status
// message. It is laid over its parent widget while a blocking operation runs on
// a worker thread, so the UI keeps animating instead of appearing frozen.
class LoadingOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingOverlay(QWidget *parent);

    // Adapts the backdrop / spinner / text colours to the active theme.
    void setDarkMode(bool dark);

    // Shows the overlay with the given message and starts the animation. When
    // `cancellable` is true a Stop button is shown that emits cancelled().
    void start(const QString& message, bool cancellable = false);
    // Hides the overlay and stops the animation.
    void stop();

    // Updates the status text while the overlay stays visible (e.g. to show
    // "Stopping…" after the user clicked Stop).
    void setMessage(const QString& message);

signals:
    // Emitted when the user clicks the Stop button.
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void repositionCancelButton();

    QTimer *timer;
    QPushButton *cancelButton;
    int angle;
    QString message;
    QColor backdropColor;
    QColor accentColor;
    QColor textColor;
};

#endif // LOADINGOVERLAY_H
