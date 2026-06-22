#include "errorhandler.h"

#include <QMessageBox>
#include <QWidget>

ErrorHandler::ErrorHandler(QWidget* parent, LogFn logger)
    : parent_(parent)
    , logger_(std::move(logger))
{
}

void ErrorHandler::setTheme(theme::Mode mode)
{
    mode_ = mode;
}

void ErrorHandler::setSilentDialogs(bool silent)
{
    silent_ = silent;
}

theme::Output ErrorHandler::roleFor(iio::Severity severity)
{
    switch (severity) {
        case iio::Severity::Info:    return theme::Output::Info;
        case iio::Severity::Warning: return theme::Output::Warning;
        case iio::Severity::Error:   return theme::Output::Error;
    }
    return theme::Output::Info;
}

bool ErrorHandler::handle(const iio::Result& result)
{
    if (result.success) {
        return true;
    }
    present(result.severity, result.code,
            QString::fromStdString(result.message));
    return false;
}

void ErrorHandler::reportError(const QString& message, iio::ErrorCode code)
{
    present(iio::Severity::Error, code, message);
}

void ErrorHandler::present(iio::Severity severity, iio::ErrorCode code,
                           const QString& message)
{
    // Always log the failure to the console with a severity-appropriate colour.
    const QString prefix = (severity == iio::Severity::Error) ? "✕  "
                         : (severity == iio::Severity::Warning) ? "⚠  "
                         : "ℹ  ";
    const QString codeLabel = QString::fromLatin1(iio::error::codeLabel(code));
    QString line = prefix;
    if (code != iio::ErrorCode::None) {
        line += codeLabel + ": ";
    }
    line += message;

    if (logger_) {
        logger_(line, roleFor(severity));
    }

    // Only Error-level results interrupt the user with a modal dialog; warnings
    // and info remain in the console so the workflow is not disrupted.
    if (severity != iio::Severity::Error || silent_) {
        return;
    }

    QMessageBox box(parent_);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(QStringLiteral("Error"));
    box.setText(message);
    if (code != iio::ErrorCode::None) {
        box.setInformativeText(QStringLiteral("Error type: %1").arg(codeLabel));
    }
    box.setStyleSheet(theme::messageBoxStyleSheet(mode_));
    box.exec();
}
