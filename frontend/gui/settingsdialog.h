#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QCheckBox;
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

private:
    QComboBox *eolCombo;
    QSpinBox *timeoutSpinBox;
    QCheckBox *showTimeCheckBox;
};

#endif // SETTINGSDIALOG_H
