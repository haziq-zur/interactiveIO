#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectiondialog.h"
#include "tcpip_connection.h"
#include "visa_connection.h"
#include <QMessageBox>
#include <QTime>
#include <QCompleter>

bool MainWindow::s_testMode = false;

void MainWindow::setTestMode(bool enabled)
{
    s_testMode = enabled;
}

bool MainWindow::isTestMode()
{
    return s_testMode;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , connection(nullptr)
    , lastPort(5025)
    , commandHistoryIndex(-1)
{
    ui->setupUi(this);
    setupConnections();
    updateUIState(false);
    
    // Set window title
    setWindowTitle("Interactive Instrument Communication Tool");
    
    // Setup command history completer
    setupCommandHistory();
    
    // Setup tooltips for better UX
    setupTooltips();
    
    // Welcome message with dark mode colors
    appendOutput("═════════════════════════════════════════════════", "#40a0ff");
    appendOutput("  Interactive Instrument Communication Tool", "#40a0ff");
    appendOutput("  Supports: TCP/IP Socket & VISA Protocols", "#40a0ff");
    appendOutput("═════════════════════════════════════════════════", "#40a0ff");
    appendOutput("", "white");
    appendOutput("💡 Quick Tips:", "#4ec9b0");
    appendOutput("  • Select protocol and click Connect to begin", "#c0c0c0");
    appendOutput("  • Use Up/Down arrows for command history", "#c0c0c0");
    appendOutput("  • Commands ending with '?' are queries", "#c0c0c0");
    appendOutput("", "white");
}

MainWindow::~MainWindow()
{
    if (connection && connection->isConnected()) {
        connection->disconnect();
    }
    delete ui;
}

void MainWindow::setupConnections()
{
    // NOTE: The slots named on_<widget>_<signal>() (e.g. on_connectButton_clicked)
    // are connected automatically by QMetaObject::connectSlotsByName(), which is
    // invoked from ui->setupUi(this). Connecting them again manually here would
    // fire each slot twice per signal (e.g. the connection dialog would reopen
    // immediately after being closed), so only non-auto-connected wiring is done.

    // Install event filter for command history navigation
    ui->commandInput->installEventFilter(this);
}

void MainWindow::setupTooltips()
{
    ui->protocolCombo->setToolTip("Select communication protocol: TCP/IP Socket or VISA");
    ui->connectButton->setToolTip("Connect to the instrument using selected protocol");
    ui->disconnectButton->setToolTip("Disconnect from the currently connected instrument");
    ui->sendButton->setToolTip("Send SCPI command to instrument (or press Enter)");
    ui->clearButton->setToolTip("Clear the output display");
    ui->commandInput->setToolTip("Enter SCPI command here\nUse Up/Down arrows to navigate command history\nCommands ending with '?' are queries that expect a response");
    ui->outputText->setToolTip("Command and response history with timestamps");
}

void MainWindow::setupCommandHistory()
{
    // Initialize command history
    commandHistory.clear();
    commandHistoryIndex = -1;
    
    // Setup completer with command history
    historyCompleter = new QCompleter(commandHistory, this);
    historyCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    historyCompleter->setCompletionMode(QCompleter::InlineCompletion);
    ui->commandInput->setCompleter(historyCompleter);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->commandInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Handle Up/Down arrows for command history
        if (keyEvent->key() == Qt::Key_Up) {
            navigateCommandHistory(true);
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            navigateCommandHistory(false);
            return true;
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::navigateCommandHistory(bool up)
{
    if (commandHistory.isEmpty()) {
        return;
    }
    
    if (up) {
        // Navigate backwards in history
        if (commandHistoryIndex < commandHistory.size() - 1) {
            commandHistoryIndex++;
            ui->commandInput->setText(commandHistory[commandHistoryIndex]);
        }
    } else {
        // Navigate forwards in history
        if (commandHistoryIndex > 0) {
            commandHistoryIndex--;
            ui->commandInput->setText(commandHistory[commandHistoryIndex]);
        } else if (commandHistoryIndex == 0) {
            commandHistoryIndex = -1;
            ui->commandInput->clear();
        }
    }
}

void MainWindow::addToCommandHistory(const QString& command)
{
    // Don't add empty commands or duplicates of the most recent command
    if (command.isEmpty() || (!commandHistory.isEmpty() && commandHistory.first() == command)) {
        return;
    }
    
    // Add to beginning of history
    commandHistory.prepend(command);
    
    // Limit history size
    if (commandHistory.size() > 100) {
        commandHistory.removeLast();
    }
    
    // Update completer
    historyCompleter->setModel(new QStringListModel(commandHistory, historyCompleter));
    
    // Reset history index
    commandHistoryIndex = -1;
}

void MainWindow::updateUIState(bool connected)
{
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    ui->sendButton->setEnabled(connected);
    ui->commandInput->setEnabled(connected);
    ui->protocolCombo->setEnabled(!connected);
    
    if (connected) {
        ui->statusLabel->setText("Status: ✓ Connected");
        ui->statusLabel->setStyleSheet("QLabel { color: #4ec9b0; font-weight: bold; font-size: 11pt; }");
        statusBar()->showMessage("🔗 Connected to instrument - Ready to send commands");
        statusBar()->setStyleSheet("QStatusBar { background-color: #0e639c; color: white; }");
    } else {
        ui->statusLabel->setText("Status: ✗ Disconnected");
        ui->statusLabel->setStyleSheet("QLabel { color: #f48771; font-weight: bold; font-size: 11pt; }");
        statusBar()->showMessage("Ready - Select protocol and connect to begin");
        statusBar()->setStyleSheet("QStatusBar { background-color: #3f3f46; color: #e0e0e0; }");
    }
}

void MainWindow::appendOutput(const QString& text, const QString& color)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    QString html = QString("<span style='color: #808080;'>[%1]</span> <span style='color: %2;'>%3</span>")
                   .arg(timestamp, color, text.toHtmlEscaped());
    ui->outputText->append(html);
}

void MainWindow::showError(const QString& message)
{
    appendOutput("❌ ERROR: " + message, "#f48771");
    if (s_testMode) {
        return;
    }
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(message);
    msgBox.setStyleSheet("QMessageBox { background-color: #2d2d30; color: #e0e0e0; }");
    msgBox.exec();
}

void MainWindow::showSuccess(const QString& message)
{
    appendOutput("✓ " + message, "#4ec9b0");
}

void MainWindow::on_protocolCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // Protocol selection changed - nothing to do until connect is clicked
}

void MainWindow::on_connectButton_clicked()
{
    int protocolIndex = ui->protocolCombo->currentIndex();
    
    // Create connection based on selected protocol
    if (protocolIndex == 0) {
        // TCP/IP
        connection = std::make_unique<TCPIPConnection>();
        
        // Show connection dialog for TCP/IP
        ConnectionDialog dialog(ConnectionDialog::Protocol::TCPIP, this);
        dialog.setAddress(lastAddress);
        dialog.setPort(lastPort);
        
        if (dialog.exec() == QDialog::Accepted) {
            lastAddress = dialog.getAddress();
            lastPort = dialog.getPort();
            
            appendOutput("🔌 Initializing TCP/IP connection...", "#ce9178");
            
            if (!connection->initialize()) {
                showError("Failed to initialize: " + QString::fromStdString(connection->getLastError()));
                connection.reset();
                return;
            }
            
            appendOutput("🌐 Connecting to " + lastAddress + ":" + QString::number(lastPort) + "...", "#40a0ff");
            
            if (!connection->connect(lastAddress.toStdString(), lastPort)) {
                showError("Failed to connect: " + QString::fromStdString(connection->getLastError()));
                connection.reset();
                return;
            }
            
            showSuccess("Connected successfully via TCP/IP!");
            updateUIState(true);
            ui->commandInput->setFocus();
        } else {
            connection.reset();
        }
        
    } else if (protocolIndex == 1) {
        // VISA
        connection = std::make_unique<VISAConnection>();
        
        // Check if VISA is available
        if (!connection->isAvailable()) {
            showError("VISA library not found! Please install NI-VISA from: https://www.ni.com/visa");
            connection.reset();
            return;
        }
        
        // Show connection dialog for VISA
        ConnectionDialog dialog(ConnectionDialog::Protocol::VISA, this);
        dialog.setResourceString(lastResourceString);
        
        if (dialog.exec() == QDialog::Accepted) {
            lastResourceString = dialog.getResourceString();
            
            appendOutput("🔌 Initializing VISA connection...", "#ce9178");
            
            if (!connection->initialize()) {
                showError("Failed to initialize: " + QString::fromStdString(connection->getLastError()));
                connection.reset();
                return;
            }
            
            appendOutput("🌐 Connecting to " + lastResourceString + "...", "#40a0ff");
            
            if (!connection->connect(lastResourceString.toStdString())) {
                showError("Failed to connect: " + QString::fromStdString(connection->getLastError()));
                connection.reset();
                return;
            }
            
            showSuccess("Connected successfully via VISA!");
            updateUIState(true);
            ui->commandInput->setFocus();
        } else {
            connection.reset();
        }
    }
}

void MainWindow::on_disconnectButton_clicked()
{
    if (connection && connection->isConnected()) {
        connection->disconnect();
        appendOutput("🔌 Disconnected from instrument.", "#dcdcaa");
        updateUIState(false);
    }
    connection.reset();
}

void MainWindow::on_sendButton_clicked()
{
    on_commandInput_returnPressed();
}

void MainWindow::on_commandInput_returnPressed()
{
    if (!connection || !connection->isConnected()) {
        return;
    }
    
    QString command = ui->commandInput->text().trimmed();
    if (command.isEmpty()) {
        return;
    }
    
    // Add to command history
    addToCommandHistory(command);
    
    // Display sent command
    appendOutput("📤 >> " + command, "#569cd6");
    
    // Send command
    std::string cmdStr = command.toStdString();
    bool isQuery = command.contains('?');
    
    if (!connection->sendCommand(cmdStr)) {
        showError("Failed to send command: " + QString::fromStdString(connection->getLastError()));
        return;
    }
    
    // If it's a query, read response
    if (isQuery) {
        std::string response = connection->readResponse(10);
        if (!response.empty()) {
            appendOutput("📥 << " + QString::fromStdString(response), "#4ec9b0");
        } else {
            appendOutput("⚠ No response received (timeout or error)", "#ce9178");
            if (!connection->getLastError().empty()) {
                appendOutput("Error: " + QString::fromStdString(connection->getLastError()), "#f48771");
            }
        }
    } else {
        appendOutput("✓ Command sent successfully", "#4ec9b0");
    }
    
    // Clear input field
    ui->commandInput->clear();
}

void MainWindow::on_clearButton_clicked()
{
    ui->outputText->clear();
    appendOutput("🗑 Output cleared.", "#808080");
}

void MainWindow::on_actionAbout_triggered()
{
    QString aboutText = 
        "<h2 style='color: #40a0ff;'>Interactive Instrument Communication Tool</h2>"
        "<p style='color: #e0e0e0;'><b>Version:</b> 1.0.0</p>"
        "<p style='color: #e0e0e0;'>A modern GUI application for communicating with SCPI-compatible instruments.</p>"
        "<br>"
        "<p style='color: #4ec9b0;'><b>Supported Protocols:</b></p>"
        "<ul style='color: #c0c0c0;'>"
        "<li>TCP/IP Socket - Direct network communication</li>"
        "<li>VISA - Industry-standard instrument control</li>"
        "</ul>"
        "<br>"
        "<p style='color: #4ec9b0;'><b>Features:</b></p>"
        "<ul style='color: #c0c0c0;'>"
        "<li>Dark mode interface for comfortable viewing</li>"
        "<li>Command history with Up/Down arrow navigation</li>"
        "<li>Auto-completion for previously used commands</li>"
        "<li>Timestamped command and response logging</li>"
        "<li>Support for both write commands and queries</li>"
        "</ul>"
        "<br>"
        "<p style='color: #808080;'><i>Built with Qt6 and C++17</i></p>";
    
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutText);
    aboutBox.setIconPixmap(QPixmap()); // No icon
    aboutBox.setStyleSheet(
        "QMessageBox { background-color: #2d2d30; }"
        "QMessageBox QLabel { color: #e0e0e0; background-color: transparent; }"
        "QPushButton { background-color: #0e639c; color: white; border: none; "
        "border-radius: 5px; padding: 8px 20px; min-width: 80px; }"
        "QPushButton:hover { background-color: #1177bb; }"
    );
    aboutBox.exec();
}
