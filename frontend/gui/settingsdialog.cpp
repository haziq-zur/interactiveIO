#include "settingsdialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

#include <iterator>

namespace {

// EOL menu entries pair a human-readable label with the raw sequence sent on
// the wire. Kept in one place so the combo box and value mapping stay in sync.
struct EolOption {
    const char *label;
    const char *value;
};

const EolOption kEolOptions[] = {
    {"LF  (\\n)",     "\n"},
    {"CR  (\\r)",     "\r"},
    {"CRLF  (\\r\\n)", "\r\n"},
    {"None",          ""},
};

// Per-vendor screen-capture presets. Each pairs a human-readable label with the
// SCPI command that returns the display as an IEEE 488.2 binary block and the
// format that command produces. The capture command differs by manufacturer, so
// these are starting points only — always confirm against the instrument's
// programming manual.
struct CapturePreset {
    const char *label;    // shown in the preset combo
    const char *command;  // SCPI query that returns the image block
    const char *format;   // resulting image format ("" = auto-detect)
};

const CapturePreset kCapturePresets[] = {
    {"Keysight / Agilent  ·  :DISPlay:DATA? PNG", ":DISPlay:DATA? PNG", "png"},
    {"Keysight (BMP)  ·  :DISPlay:DATA? BMP",      ":DISPlay:DATA? BMP", "bmp"},
    {"Rohde & Schwarz  ·  HCOPy:DATA?",            "HCOPy:DATA?",        "png"},
    {"Tektronix  ·  HARDCopy:DATA?",               "HARDCopy:DATA?",     "png"},
    {"Rigol  ·  :DISPlay:DATA?",                   ":DISPlay:DATA?",     "bmp"},
    {"Siglent  ·  :PRINt? PNG",                    ":PRINt? PNG",        "png"},
};

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , eolCombo(new QComboBox(this))
    , timeoutSpinBox(new QSpinBox(this))
    , showTimeCheckBox(new QCheckBox(this))
    , capturePresetCombo(new QComboBox(this))
    , captureCommandEdit(new QLineEdit(this))
    , captureFormatCombo(new QComboBox(this))
    , logPasswordEdit(new QLineEdit(this))
    , showPasswordCheckBox(new QCheckBox(this))
{
    setWindowTitle(tr("Communication Settings"));
    setObjectName("settingsDialog");
    setModal(true);

    eolCombo->setObjectName("eolCombo");
    eolCombo->setToolTip(tr("End-of-line sequence appended to every command before sending"));
    for (const auto& option : kEolOptions) {
        eolCombo->addItem(tr(option.label), QString::fromLatin1(option.value));
    }

    timeoutSpinBox->setObjectName("timeoutSpinBox");
    timeoutSpinBox->setToolTip(tr("Maximum time to wait for an instrument response"));
    timeoutSpinBox->setRange(1, 300);
    timeoutSpinBox->setSuffix(tr(" s"));
    timeoutSpinBox->setValue(10);

    showTimeCheckBox->setObjectName("showTimeCheckBox");
    showTimeCheckBox->setText(tr("Show time to complete each command (ms)"));
    showTimeCheckBox->setToolTip(
        tr("Measure and display how long each command takes, from the moment it\n"
           "is sent until the response (or acknowledgement) is received"));
    showTimeCheckBox->setChecked(true);

    // --- Screen image capture -------------------------------------------------
    capturePresetCombo->setObjectName("capturePresetCombo");
    capturePresetCombo->setToolTip(
        tr("Pick a known instrument to fill in its capture command, or choose\n"
           "Custom to type your own. The command differs by manufacturer."));
    for (const auto& preset : kCapturePresets) {
        // Store the command in data so selecting a preset fills the edit box.
        capturePresetCombo->addItem(tr(preset.label), QString::fromLatin1(preset.command));
    }
    capturePresetCombo->addItem(tr("Custom\u2026"), QString());

    captureCommandEdit->setObjectName("captureCommandEdit");
    captureCommandEdit->setToolTip(
        tr("SCPI query sent to the instrument to capture its screen. It must\n"
           "return the image as an IEEE 488.2 binary block (e.g. #800012345...).\n"
           "Examples by vendor:\n"
           "  • Keysight / Agilent : :DISPlay:DATA? PNG\n"
           "  • Rohde & Schwarz    : HCOPy:DATA?\n"
           "  • Tektronix          : HARDCopy:DATA?\n"
           "  • Rigol              : :DISPlay:DATA?\n"
           "Consult your instrument's programming manual for the exact command."));
    captureCommandEdit->setPlaceholderText(tr(":DISPlay:DATA? PNG"));

    captureFormatCombo->setObjectName("captureFormatCombo");
    captureFormatCombo->setToolTip(
        tr("Image format the capture command returns. Auto-detect inspects the\n"
           "returned bytes (PNG/BMP/JPEG/GIF signatures) and is safe to leave on."));
    captureFormatCombo->addItem(tr("Auto-detect"), QString());
    captureFormatCombo->addItem(tr("PNG"), QStringLiteral("png"));
    captureFormatCombo->addItem(tr("BMP"), QStringLiteral("bmp"));
    captureFormatCombo->addItem(tr("JPEG"), QStringLiteral("jpg"));
    captureFormatCombo->addItem(tr("GIF"), QStringLiteral("gif"));

    // Selecting a preset populates the command (and format when known); typing
    // a custom command switches the preset selector to "Custom".
    connect(capturePresetCombo, QOverload<int>::of(&QComboBox::activated),
            this, [this](int index) {
        const QString command = capturePresetCombo->itemData(index).toString();
        if (command.isEmpty()) {
            return;  // "Custom…" — leave the edit box untouched.
        }
        captureCommandEdit->setText(command);
        if (index >= 0 && index < static_cast<int>(std::size(kCapturePresets))) {
            setCaptureFormat(QString::fromLatin1(kCapturePresets[index].format));
        }
    });
    connect(captureCommandEdit, &QLineEdit::textEdited, this, [this]() {
        // Reflect manual edits by selecting the matching preset, else Custom.
        const QString current = captureCommandEdit->text().trimmed();
        const int match = capturePresetCombo->findData(current);
        capturePresetCombo->setCurrentIndex(
            match >= 0 ? match : capturePresetCombo->count() - 1);
    });

    auto *captureForm = new QFormLayout;
    captureForm->addRow(tr("Instrument preset:"), capturePresetCombo);
    captureForm->addRow(tr("Capture command:"), captureCommandEdit);
    captureForm->addRow(tr("Capture format:"), captureFormatCombo);

    auto *captureHelp = new QLabel(
        tr("<small>The screen-capture SCPI command is <b>not standardised</b> across "
           "instruments. Pick a preset above or enter the command from your device's "
           "programming manual. The query must return the image as an IEEE&nbsp;488.2 "
           "binary block; PNG, BMP, JPEG and GIF are auto-detected. Some instruments "
           "need a one-time setup command first (e.g. <code>:HCOPy:FORMat PNG</code>)."
           "</small>"),
        this);
    captureHelp->setObjectName("captureHelpLabel");
    captureHelp->setWordWrap(true);
    captureHelp->setTextFormat(Qt::RichText);

    auto *captureGroup = new QGroupBox(tr("Screen Image Capture"), this);
    captureGroup->setObjectName("captureGroup");
    auto *captureLayout = new QVBoxLayout(captureGroup);
    captureLayout->addLayout(captureForm);
    captureLayout->addWidget(captureHelp);

    // --- Log file encryption --------------------------------------------------
    logPasswordEdit->setObjectName("logPasswordEdit");
    logPasswordEdit->setEchoMode(QLineEdit::Password);
    logPasswordEdit->setToolTip(
        tr("Password used to encrypt every saved log file and to decrypt logs\n"
           "when opening them. The same value is applied automatically to both\n"
           "Save Log and Open Log, so you are not prompted each time."));
    logPasswordEdit->setPlaceholderText(tr("Log encryption password"));

    showPasswordCheckBox->setObjectName("showPasswordCheckBox");
    showPasswordCheckBox->setText(tr("Show password"));
    connect(showPasswordCheckBox, &QCheckBox::toggled, this, [this](bool shown) {
        logPasswordEdit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto *logForm = new QFormLayout;
    logForm->addRow(tr("Encryption password:"), logPasswordEdit);
    logForm->addRow(QString(), showPasswordCheckBox);

    auto *logHelp = new QLabel(
        tr("<small>This password is used automatically whenever you <b>save</b> or "
           "<b>open</b> a log file — you will not be prompted. Logs saved with one "
           "password can only be reopened with that same password, so keep it safe. "
           "Changing it here affects future saves and opens.</small>"),
        this);
    logHelp->setObjectName("logHelpLabel");
    logHelp->setWordWrap(true);
    logHelp->setTextFormat(Qt::RichText);

    auto *logGroup = new QGroupBox(tr("Log File Encryption"), this);
    logGroup->setObjectName("logGroup");
    auto *logLayout = new QVBoxLayout(logGroup);
    logLayout->addLayout(logForm);
    logLayout->addWidget(logHelp);

    auto *form = new QFormLayout;
    form->addRow(tr("EOL sequence:"), eolCombo);
    form->addRow(tr("Response timeout:"), timeoutSpinBox);
    form->addRow(QString(), showTimeCheckBox);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("settingsButtonBox");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(captureGroup);
    layout->addWidget(logGroup);
    layout->addStretch();
    layout->addWidget(buttons);
}

QString SettingsDialog::eolSequence() const
{
    return eolCombo->currentData().toString();
}

void SettingsDialog::setEolSequence(const QString& eol)
{
    const int index = eolCombo->findData(eol);
    if (index >= 0) {
        eolCombo->setCurrentIndex(index);
    }
}

int SettingsDialog::responseTimeout() const
{
    return timeoutSpinBox->value();
}

void SettingsDialog::setResponseTimeout(int seconds)
{
    timeoutSpinBox->setValue(seconds);
}

bool SettingsDialog::showResponseTime() const
{
    return showTimeCheckBox->isChecked();
}

void SettingsDialog::setShowResponseTime(bool show)
{
    showTimeCheckBox->setChecked(show);
}

QString SettingsDialog::captureCommand() const
{
    return captureCommandEdit->text().trimmed();
}

void SettingsDialog::setCaptureCommand(const QString& command)
{
    captureCommandEdit->setText(command);
    // Reflect the command in the preset selector (Custom when no preset matches).
    const int match = capturePresetCombo->findData(command.trimmed());
    capturePresetCombo->setCurrentIndex(
        match >= 0 ? match : capturePresetCombo->count() - 1);
}

QString SettingsDialog::captureFormat() const
{
    return captureFormatCombo->currentData().toString();
}

void SettingsDialog::setCaptureFormat(const QString& format)
{
    const int index = captureFormatCombo->findData(format);
    captureFormatCombo->setCurrentIndex(index >= 0 ? index : 0);
}

QString SettingsDialog::logPassword() const
{
    return logPasswordEdit->text();
}

void SettingsDialog::setLogPassword(const QString& password)
{
    logPasswordEdit->setText(password);
}
