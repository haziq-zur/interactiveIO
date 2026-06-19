#include "settingsdialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

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

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , eolCombo(new QComboBox(this))
    , timeoutSpinBox(new QSpinBox(this))
    , showTimeCheckBox(new QCheckBox(this))
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
