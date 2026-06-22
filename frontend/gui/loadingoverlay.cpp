#include "loadingoverlay.h"

#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QPushButton>

LoadingOverlay::LoadingOverlay(QWidget *parent)
    : QWidget(parent)
    , timer(new QTimer(this))
    , cancelButton(new QPushButton(tr("Stop"), this))
    , angle(0)
    , backdropColor(13, 15, 20, 180)
    , accentColor(0x63, 0x66, 0xf1)
    , textColor(0xe6, 0xe8, 0xec)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setVisible(false);

    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setVisible(false);
    cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #e5484d; color: #ffffff; border: none;"
        "  border-radius: 6px; padding: 6px 18px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #f2555a; }"
        "QPushButton:pressed { background-color: #c93b40; }");
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        emit cancelled();
    });

    // ~33 fps rotation.
    timer->setInterval(30);
    connect(timer, &QTimer::timeout, this, [this]() {
        angle = (angle + 12) % 360;
        update();
    });

    if (parent) {
        parent->installEventFilter(this);
        setGeometry(parent->rect());
    }
}

void LoadingOverlay::setDarkMode(bool dark)
{
    if (dark) {
        backdropColor = QColor(13, 15, 20, 180);   // #0d0f14 @ ~70%
        textColor = QColor(0xe6, 0xe8, 0xec);
    } else {
        backdropColor = QColor(244, 245, 247, 200); // #f4f5f7 @ ~78%
        textColor = QColor(0x1c, 0x20, 0x29);
    }
    accentColor = QColor(0x63, 0x66, 0xf1);         // indigo on both themes
    if (isVisible()) {
        update();
    }
}

void LoadingOverlay::start(const QString& msg, bool cancellable)
{
    message = msg;
    angle = 0;
    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
    cancelButton->setVisible(cancellable);
    if (cancellable) {
        cancelButton->setEnabled(true);
        repositionCancelButton();
        cancelButton->raise();
    }
    raise();
    setVisible(true);
    timer->start();
    update();
}

void LoadingOverlay::stop()
{
    timer->stop();
    cancelButton->setVisible(false);
    setVisible(false);
}

void LoadingOverlay::setMessage(const QString& msg)
{
    message = msg;
    if (isVisible()) {
        repositionCancelButton();
        update();
    }
}

void LoadingOverlay::repositionCancelButton()
{
    // Centred horizontally, below the spinner + status text.
    const QSize hint = cancelButton->sizeHint();
    const int x = (width() - hint.width()) / 2;
    const int y = static_cast<int>(height() / 2.0 - 10.0) + 22 + 46;
    cancelButton->setGeometry(x, y, hint.width(), hint.height());
}

bool LoadingOverlay::eventFilter(QObject *watched, QEvent *event)
{
    // Keep the overlay sized to its parent.
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        if (cancelButton->isVisible()) {
            repositionCancelButton();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LoadingOverlay::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Translucent backdrop dims the content behind the overlay.
    painter.fillRect(rect(), backdropColor);

    const QPointF center(width() / 2.0, height() / 2.0 - 10.0);
    const double radius = 22.0;

    // Spinner: 12 fading segments rotated by the current angle.
    const int segments = 12;
    painter.save();
    painter.translate(center);
    painter.rotate(angle);
    for (int i = 0; i < segments; ++i) {
        const double alpha = (i + 1) / static_cast<double>(segments);
        QColor c = accentColor;
        c.setAlphaF(alpha);
        QPen pen(c);
        pen.setWidth(4);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(QPointF(0, -radius + 7), QPointF(0, -radius));
        painter.rotate(360.0 / segments);
    }
    painter.restore();

    // Status message centred beneath the spinner.
    if (!message.isEmpty()) {
        QFont f = font();
        f.setPointSizeF(10.5);
        f.setBold(true);
        painter.setFont(f);
        painter.setPen(textColor);
        const QRectF textRect(0, center.y() + radius + 10.0, width(), 30.0);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, message);
    }
}
