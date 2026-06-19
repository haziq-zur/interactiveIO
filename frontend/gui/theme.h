#ifndef THEME_H
#define THEME_H

#include <QString>

// Centralised theming for the GUI. Provides a dark and a light design system,
// the matching message-box styling used by the in-app dialogs, and helpers to
// load / persist the user's choice via QSettings so it survives restarts.
namespace theme {

enum class Mode {
    Dark,
    Light
};

// Semantic colour roles for the console output. Each resolves to a different
// hex value per theme so text stays legible on both the dark and light
// backgrounds.
enum class Output {
    Accent,     // titles, sent commands
    Muted,      // subtitles, separators, dim notices
    MutedText,  // secondary body text (tips)
    Success,    // received data, success notices
    Error,      // error notices
    Info,       // informational notices (connecting, loaded)
    Warning,    // warnings (disconnected, timeouts)
    Timestamp   // the leading timestamp on every line
};

// Resolves a console colour role to a concrete hex string for the given mode.
QString consoleColor(Mode mode, Output role);


// Full application stylesheet for the given mode (applied with
// qApp->setStyleSheet()).
QString styleSheet(Mode mode);

// Stylesheet for QMessageBox instances (About / errors) so they match the
// active theme rather than always rendering dark.
QString messageBoxStyleSheet(Mode mode);

// Persisted preference (defaults to Dark when nothing is stored yet).
Mode loadSavedMode();
void saveMode(Mode mode);

} // namespace theme

#endif // THEME_H
