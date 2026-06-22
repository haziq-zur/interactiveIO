#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <functional>

#include <QString>

#include "instrument_controller.h"
#include "theme.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

// Centralised, severity-aware presentation of controller results for the GUI.
//
// Every user-facing failure flows through a single ErrorHandler so the
// behaviour is consistent: the result is always logged to the console (via the
// supplied logger callback) using a colour that matches its severity, and
// Error-level results additionally raise a themed modal dialog. Warnings and
// info are logged only, so recoverable conditions (e.g. a query timeout) do not
// interrupt the user with a dialog.
class ErrorHandler {
public:
    // Logs a line of text to the console using the given semantic colour role.
    using LogFn = std::function<void(const QString& text, theme::Output role)>;

    ErrorHandler(QWidget* parent, LogFn logger);

    void setTheme(theme::Mode mode);
    // When silent, modal dialogs are suppressed (used by automated tests / CI).
    void setSilentDialogs(bool silent);

    // Central entry point. Logs the result and, for Error severity, shows a
    // modal dialog. Returns result.success for convenient inline use.
    bool handle(const iio::Result& result);

    // Presents an ad-hoc error that did not originate from the controller
    // (e.g. a frontend validation failure such as an empty password).
    void reportError(const QString& message,
                     iio::ErrorCode code = iio::ErrorCode::InvalidInput);

    // Maps a severity to the console colour role used for its log line.
    static theme::Output roleFor(iio::Severity severity);

private:
    void present(iio::Severity severity, iio::ErrorCode code,
                 const QString& message);

    QWidget* parent_;
    LogFn logger_;
    theme::Mode mode_ = theme::Mode::Dark;
    bool silent_ = false;
};

#endif // ERRORHANDLER_H
