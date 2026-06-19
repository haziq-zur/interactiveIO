#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QColor>
#include <QString>

QT_BEGIN_NAMESPACE
class QTimer;
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

    // Shows the overlay with the given message and starts the animation.
    void start(const QString& message);
    // Hides the overlay and stops the animation.
    void stop();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QTimer *timer;
    int angle;
    QString message;
    QColor backdropColor;
    QColor accentColor;
    QColor textColor;
};

#endif // LOADINGOVERLAY_H
