#include "mainwindow.h"
#include "theme.h"
#include <QApplication>
#include <QFont>
#include <QStyleFactory>

// =============================================================================
// GUI entry point.
//
// The visual design (dark + light themes) lives in theme.cpp. On startup the
// last-used theme is loaded from QSettings and applied globally; the user can
// switch at runtime via View → Toggle Theme in the main window.
// =============================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Application metadata (also used by QSettings for persistence).
    app.setApplicationName("Interactive Instrument Communication Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("InteractiveIO");

    // Fusion is the most stylesheet-friendly built-in style.
    app.setStyle(QStyleFactory::create("Fusion"));

    // Base application font.
    QFont appFont("Segoe UI", 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    // Apply the saved theme globally (main window + dialogs inherit it).
    app.setStyleSheet(theme::styleSheet(theme::loadSavedMode()));

    MainWindow window;
    window.show();

    return app.exec();
}
