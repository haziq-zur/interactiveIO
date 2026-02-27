#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "tcpip_connection.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

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
    
    // Connect helper buttons
    connect(ui->tcpipHelperButton, &QPushButton::clicked, this, &ConnectionDialog::on_tcpipHelperButton_clicked);
    connect(ui->socketHelperButton, &QPushButton::clicked, this, &ConnectionDialog::on_socketHelperButton_clicked);
    connect(ui->gpibHelperButton, &QPushButton::clicked, this, &ConnectionDialog::on_gpibHelperButton_clicked);
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
        QMessageBox::warning(this, "Validation Error", "IP address cannot be empty.");
        return false;
    }
    
    // Validate IP address format
    if (!TCPIPUtils::isValidIPAddress(ip.toStdString())) {
        QMessageBox::warning(this, "Validation Error", "Invalid IP address format.");
        return false;
    }
    
    int port = ui->portSpinBox->value();
    if (port < 1 || port > 65535) {
        QMessageBox::warning(this, "Validation Error", "Port must be between 1 and 65535.");
        return false;
    }
    
    return true;
}

bool ConnectionDialog::validateVISA()
{
    QString resource = ui->resourceEdit->text().trimmed();
    
    if (resource.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Resource string cannot be empty.");
        return false;
    }
    
    // Basic validation - check if it contains typical VISA keywords
    if (!resource.contains("::")) {
        QMessageBox::warning(this, "Validation Error", 
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
        QMessageBox::warning(this, "Input Required", "Please enter an IP address first.");
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
        QMessageBox::warning(this, "Input Required", "Please enter an IP address first.");
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
