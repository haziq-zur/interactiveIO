#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "tcpip_connection.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

bool ConnectionDialog::s_testMode = false;

void ConnectionDialog::setTestMode(bool enabled)
{
    s_testMode = enabled;
}

bool ConnectionDialog::isTestMode()
{
    return s_testMode;
}

ConnectionDialog::ConnectionDialog(Protocol protocol, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
    , currentProtocol(protocol)
    , isProcessing(false)
{
    ui->setupUi(this);
    
    if (protocol == Protocol::TCPIP) {
        setupForTCPIP();
        setWindowTitle("TCP/IP Connection Setup");
    } else {
        setupForVISA();
        setWindowTitle("VISA Connection Setup");
    }
    
    // Connect OK button
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectionDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::setupForTCPIP()
{
    // Show TCP/IP widgets, hide VISA widgets
    ui->tcpipWidget->setVisible(true);
    ui->visaWidget->setVisible(false);
    
    // Set default values
    ui->ipAddressEdit->setText("192.168.1.100");
    ui->portSpinBox->setValue(5025);
    ui->portSpinBox->setRange(1, 65535);
    
    // Set focus to IP address field
    ui->ipAddressEdit->setFocus();
    
    // Add validator for IP address (basic validation)
    QRegularExpression ipRegex("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    ui->ipAddressEdit->setValidator(new QRegularExpressionValidator(ipRegex, this));
}

void ConnectionDialog::setupForVISA()
{
    // Show VISA widgets, hide TCP/IP widgets
    ui->tcpipWidget->setVisible(false);
    ui->visaWidget->setVisible(true);
    
    // Set example text
    ui->resourceEdit->setPlaceholderText("e.g., TCPIP::192.168.1.100::INSTR");
    
    // Set focus to resource field
    ui->resourceEdit->setFocus();

    // NOTE: The helper-button slots named on_<widget>_clicked() are connected
    // automatically by QMetaObject::connectSlotsByName() (invoked from setupUi),
    // so they must not be connected again here or each would fire twice.
}

QString ConnectionDialog::getAddress() const
{
    return ui->ipAddressEdit->text();
}

void ConnectionDialog::setAddress(const QString& address)
{
    ui->ipAddressEdit->setText(address);
}

int ConnectionDialog::getPort() const
{
    return ui->portSpinBox->value();
}

void ConnectionDialog::setPort(int port)
{
    ui->portSpinBox->setValue(port);
}

QString ConnectionDialog::getResourceString() const
{
    return ui->resourceEdit->text();
}

void ConnectionDialog::setResourceString(const QString& resource)
{
    ui->resourceEdit->setText(resource);
}

bool ConnectionDialog::validateTCPIP()
{
    QString ip = ui->ipAddressEdit->text();
    
    if (ip.isEmpty()) {
        if (!s_testMode) QMessageBox::warning(this, "Validation Error", "IP address cannot be empty.");
        return false;
    }
    
    // Validate IP address format
    if (!TCPIPUtils::isValidIPAddress(ip.toStdString())) {
        if (!s_testMode) QMessageBox::warning(this, "Validation Error", "Invalid IP address format.");
        return false;
    }
    
    int port = ui->portSpinBox->value();
    if (port < 1 || port > 65535) {
        if (!s_testMode) QMessageBox::warning(this, "Validation Error", "Port must be between 1 and 65535.");
        return false;
    }
    
    return true;
}

bool ConnectionDialog::validateVISA()
{
    QString resource = ui->resourceEdit->text().trimmed();
    
    if (resource.isEmpty()) {
        if (!s_testMode) QMessageBox::warning(this, "Validation Error", "Resource string cannot be empty.");
        return false;
    }
    
    // Basic validation - check if it contains typical VISA keywords
    if (!resource.contains("::")) {
        if (!s_testMode) QMessageBox::warning(this, "Validation Error", 
                            "Invalid VISA resource string format. Must contain '::' separator.");
        return false;
    }
    
    return true;
}

void ConnectionDialog::validateAndAccept()
{
    // Prevent double-triggering
    if (isProcessing) {
        return;
    }
    
    isProcessing = true;
    
    if (currentProtocol == Protocol::TCPIP) {
        if (validateTCPIP()) {
            accept();
        } else {
            isProcessing = false;
        }
    } else {
        if (validateVISA()) {
            accept();
        } else {
            isProcessing = false;
        }
    }
}

void ConnectionDialog::on_tcpipHelperButton_clicked()
{
    QString ip = ui->helperIpEdit->text();
    if (ip.isEmpty()) {
        if (!s_testMode) QMessageBox::warning(this, "Input Required", "Please enter an IP address first.");
        return;
    }
    
    QString resource = QString("TCPIP::%1::INSTR").arg(ip);
    ui->resourceEdit->setText(resource);
}

void ConnectionDialog::on_socketHelperButton_clicked()
{
    QString ip = ui->helperIpEdit->text();
    int port = ui->helperPortSpinBox->value();
    
    if (ip.isEmpty()) {
        if (!s_testMode) QMessageBox::warning(this, "Input Required", "Please enter an IP address first.");
        return;
    }
    
    QString resource = QString("TCPIP::%1::%2::SOCKET").arg(ip).arg(port);
    ui->resourceEdit->setText(resource);
}

void ConnectionDialog::on_gpibHelperButton_clicked()
{
    int board = ui->helperBoardSpinBox->value();
    int address = ui->helperAddressSpinBox->value();
    
    QString resource = QString("GPIB%1::%2::INSTR").arg(board).arg(address);
    ui->resourceEdit->setText(resource);
}

void ConnectionDialog::on_usbHelperButton_clicked()
{
    QString vendorId = ui->helperVendorIdEdit->text().trimmed();
    QString productId = ui->helperProductIdEdit->text().trimmed();
    QString serial = ui->helperSerialEdit->text().trimmed();
    
    // Validate vendor and product IDs
    if (vendorId.isEmpty() || productId.isEmpty()) {
        if (!s_testMode) QMessageBox::warning(this, "Input Required", 
                           "Please enter both Vendor ID and Product ID.\n\n"
                           "Example: 0x1234 for Vendor ID, 0x5678 for Product ID\n"
                           "Or: 2A8D for hex without prefix");
        return;
    }
    
    // Convert decimal to hex if needed
    bool isVidDecimal = false;
    bool isPidDecimal = false;
    vendorId.toInt(&isVidDecimal, 10);
    productId.toInt(&isPidDecimal, 10);
    
    // If it's a pure decimal number (no 0x prefix and no hex letters), convert to hex
    if (isVidDecimal && !vendorId.startsWith("0x", Qt::CaseInsensitive) && 
        !vendorId.contains(QRegularExpression("[a-fA-F]"))) {
        int vid = vendorId.toInt();
        vendorId = QString("0x%1").arg(vid, 4, 16, QChar('0'));
    }
    if (isPidDecimal && !productId.startsWith("0x", Qt::CaseInsensitive) && 
        !productId.contains(QRegularExpression("[a-fA-F]"))) {
        int pid = productId.toInt();
        productId = QString("0x%1").arg(pid, 4, 16, QChar('0'));
    }
    
    // Ensure IDs start with 0x prefix if they're just hex digits
    if (!vendorId.startsWith("0x", Qt::CaseInsensitive)) {
        vendorId = "0x" + vendorId;
    }
    if (!productId.startsWith("0x", Qt::CaseInsensitive)) {
        productId = "0x" + productId;
    }
    
    // Build USB resource string (correct format: USB0::VID::PID[::SN]::INSTR)
    QString resource;
    if (serial.isEmpty()) {
        // Without serial number
        resource = QString("USB0::%1::%2::INSTR").arg(vendorId).arg(productId);
    } else {
        // With serial number
        resource = QString("USB0::%1::%2::%3::INSTR").arg(vendorId).arg(productId).arg(serial);
    }
    
    ui->resourceEdit->setText(resource);
    
    // Show info message
    if (!s_testMode) QMessageBox::information(this, "USB Resource String Generated",
                           QString("Generated resource string:\n%1\n\n"
                                   "Format: USB0::VendorID::ProductID::SerialNumber::INSTR\n"
                                   "Do NOT add ::0 before ::INSTR!").arg(resource));
}
