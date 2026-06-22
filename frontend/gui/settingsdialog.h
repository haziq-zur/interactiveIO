#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

// Dialog for configuring the active communication channel: the end-of-line
// (EOL) sequence appended to every command, and the timeout used when waiting
// for an instrument response. The dialog is purely presentational — the values
// it collects are applied to the InstrumentController by MainWindow.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // EOL sequence as the raw control characters (e.g. "\n", "\r\n", "").
    QString eolSequence() const;
    void setEolSequence(const QString& eol);

    // Response timeout in seconds.
    int responseTimeout() const;
    void setResponseTimeout(int seconds);

    // Whether to display the time taken to complete each command.
    bool showResponseTime() const;
    void setShowResponseTime(bool show);

    // SCPI command used to capture a screen image from the instrument.
    QString captureCommand() const;
    void setCaptureCommand(const QString& command);

    // Expected image format ("" = auto-detect, or "png"/"bmp"/"jpg"/"gif").
    QString captureFormat() const;
    void setCaptureFormat(const QString& format);

    // Password used to encrypt / decrypt saved log files. Applied to every
    // Save Log and Open Log action without prompting.
    QString logPassword() const;
    void setLogPassword(const QString& password);

private:
    QComboBox *eolCombo;
    QSpinBox *timeoutSpinBox;
    QCheckBox *showTimeCheckBox;
    QComboBox *capturePresetCombo;
    QLineEdit *captureCommandEdit;
    QComboBox *captureFormatCombo;
    QLineEdit *logPasswordEdit;
    QCheckBox *showPasswordCheckBox;
};

#endif // SETTINGSDIALOG_H
