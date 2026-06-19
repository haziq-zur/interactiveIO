#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectiondialog.h"
#include "logcrypto.h"
#include "instrument_controller.h"
#include <QMessageBox>
#include <QTime>
#include <QCompleter>
#include <QFileDialog>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <chrono>
#include <ctime>

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
    , controller(std::make_unique<iio::InstrumentController>())
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
    
    // Welcome message
    appendOutput("Interactive Instrument Communication Tool", "#818cf8");
    appendOutput("SCPI console  ·  TCP/IP Socket & VISA", "#6b7280");
    appendOutput("", "#e6e8ec");
    appendOutput("Quick tips", "#34d399");
    appendOutput("   •  Select a protocol and click Connect to begin", "#9aa1ad");
    appendOutput("   •  Use the Up / Down arrows to browse command history", "#9aa1ad");
    appendOutput("   •  Commands ending with '?' are treated as queries", "#9aa1ad");
    appendOutput("", "#e6e8ec");
}

MainWindow::~MainWindow()
{
    if (controller) {
        controller->disconnect();
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
        ui->statusLabel->setText("●  Connected");
        ui->statusLabel->setStyleSheet(
            "QLabel#statusLabel { color: #34d399; background-color: rgba(52, 211, 153, 0.12); "
            "font-weight: 700; font-size: 9pt; padding: 6px 14px; border-radius: 12px; }");
        statusBar()->showMessage("Connected — ready to send commands");
    } else {
        ui->statusLabel->setText("●  Disconnected");
        ui->statusLabel->setStyleSheet(
            "QLabel#statusLabel { color: #f87171; background-color: rgba(248, 113, 113, 0.12); "
            "font-weight: 700; font-size: 9pt; padding: 6px 14px; border-radius: 12px; }");
        statusBar()->showMessage("Ready — select a protocol and connect to begin");
    }
}

void MainWindow::appendOutput(const QString& text, const QString& color)
{
    // High-resolution timestamp with microsecond precision (HH:mm:ss.uuuuuu).
    // QTime only resolves to milliseconds, so derive the sub-second part from
    // the system clock to expose microseconds.
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto secsSinceEpoch = duration_cast<seconds>(now.time_since_epoch());
    const auto micros = duration_cast<microseconds>(now.time_since_epoch() - secsSinceEpoch);

    std::time_t tt = system_clock::to_time_t(now);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif

    QString timestamp = QString::asprintf("%02d:%02d:%02d.%06lld",
                                          localTm.tm_hour, localTm.tm_min, localTm.tm_sec,
                                          static_cast<long long>(micros.count()));

    QString html = QString("<span style='color: #5b6370;'>%1</span>&nbsp;&nbsp;<span style='color: %2;'>%3</span>")
                   .arg(timestamp, color, text.toHtmlEscaped());
    ui->outputText->append(html);
}

void MainWindow::showError(const QString& message)
{
    appendOutput("✕  ERROR: " + message, "#f87171");
    if (s_testMode) {
        return;
    }
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(message);
    msgBox.setStyleSheet("QMessageBox { background-color: #161922; color: #e6e8ec; }");
    msgBox.exec();
}

void MainWindow::showSuccess(const QString& message)
{
    appendOutput("✓  " + message, "#34d399");
}

void MainWindow::on_protocolCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // Protocol selection changed - nothing to do until connect is clicked
}

void MainWindow::on_connectButton_clicked()
{
    int protocolIndex = ui->protocolCombo->currentIndex();

    if (protocolIndex == 0) {
        // TCP/IP: gather parameters from the dialog, then hand off to the controller.
        ConnectionDialog dialog(ConnectionDialog::Protocol::TCPIP, this);
        dialog.setAddress(lastAddress);
        dialog.setPort(lastPort);

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        lastAddress = dialog.getAddress();
        lastPort = dialog.getPort();

        appendOutput("Connecting to " + lastAddress + ":" + QString::number(lastPort) + "…", "#60a5fa");

        iio::ConnectionConfig config;
        config.protocol = iio::Protocol::TCPIP;
        config.address = lastAddress.toStdString();
        config.port = lastPort;

        const iio::Result result = controller->connect(config);
        if (!result.success) {
            showError(QString::fromStdString(result.message));
            return;
        }

        showSuccess("Connected successfully via TCP/IP!");
        updateUIState(true);
        ui->commandInput->setFocus();

    } else if (protocolIndex == 1) {
        // VISA: check availability via the controller before prompting.
        if (!controller->isProtocolAvailable(iio::Protocol::VISA)) {
            showError("VISA library not found! Please install NI-VISA from: https://www.ni.com/visa");
            return;
        }

        ConnectionDialog dialog(ConnectionDialog::Protocol::VISA, this);
        dialog.setResourceString(lastResourceString);

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        lastResourceString = dialog.getResourceString();

        appendOutput("Connecting to " + lastResourceString + "…", "#60a5fa");

        iio::ConnectionConfig config;
        config.protocol = iio::Protocol::VISA;
        config.address = lastResourceString.toStdString();

        const iio::Result result = controller->connect(config);
        if (!result.success) {
            showError(QString::fromStdString(result.message));
            return;
        }

        showSuccess("Connected successfully via VISA!");
        updateUIState(true);
        ui->commandInput->setFocus();
    }
}

void MainWindow::on_disconnectButton_clicked()
{
    const bool wasConnected = controller->isConnected();
    controller->disconnect();
    if (wasConnected) {
        appendOutput("Disconnected from instrument.", "#fbbf24");
        updateUIState(false);
    }
}

void MainWindow::on_sendButton_clicked()
{
    on_commandInput_returnPressed();
}

void MainWindow::on_commandInput_returnPressed()
{
    if (!controller->isConnected()) {
        return;
    }
    
    QString command = ui->commandInput->text().trimmed();
    if (command.isEmpty()) {
        return;
    }
    
    // Add to command history
    addToCommandHistory(command);
    
    // Display sent command
    appendOutput("›  " + command, "#818cf8");
    
    // Dispatch through the controller (it auto-detects queries and reads back
    // the response when needed).
    const iio::Result result = controller->execute(command.toStdString());

    if (!result.success) {
        if (result.hasResponse || command.contains('?')) {
            // Query path: surface timeout / error without tearing down the UI.
            appendOutput(QString::fromStdString(result.message), "#fbbf24");
        } else {
            showError(QString::fromStdString(result.message));
        }
    } else if (result.hasResponse) {
        appendOutput("‹  " + QString::fromStdString(result.response), "#34d399");
    } else {
        appendOutput("✓  Command sent successfully", "#34d399");
    }
    
    // Clear input field
    ui->commandInput->clear();
}

void MainWindow::on_clearButton_clicked()
{
    ui->outputText->clear();
    appendOutput("Output cleared.", "#6b7280");
}

void MainWindow::on_actionSaveLog_triggered()
{
    if (ui->outputText->document()->isEmpty()) {
        showError("There is nothing to save - the log is empty.");
        return;
    }

    QString defaultName = "scpi_log_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";

    QString fileName = s_testMode ? QString() : QFileDialog::getSaveFileName(
        this,
        tr("Save SCPI Log"),
        defaultName,
        tr("Encrypted Log Files (*.log);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    bool okPwd = false;
    QString password = QInputDialog::getText(
        this,
        tr("Encrypt Log"),
        tr("Enter a password to encrypt the log:"),
        QLineEdit::Password,
        QString(),
        &okPwd);

    if (!okPwd) {
        return; // user cancelled
    }
    if (password.isEmpty()) {
        showError("A non-empty password is required to encrypt the log.");
        return;
    }

    saveLogToFile(fileName, password);
}

bool MainWindow::saveLogToFile(const QString& fileName, const QString& password)
{
    if (fileName.isEmpty()) {
        return false;
    }
    if (password.isEmpty()) {
        showError("A non-empty password is required to encrypt the log.");
        return false;
    }

    const QByteArray plaintext = ui->outputText->toPlainText().toUtf8();
    const QByteArray encrypted = LogCrypto::encrypt(plaintext, password);
    if (encrypted.isEmpty()) {
        showError("Encryption failed - the log was not saved.");
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        showError("Could not open file for writing: " + file.errorString());
        return false;
    }

    const qint64 written = file.write(encrypted);
    file.close();
    if (written != encrypted.size()) {
        showError("Failed to write the complete log file.");
        return false;
    }

    showSuccess("Encrypted log saved to " + fileName);
    return true;
}

void MainWindow::on_actionOpenLog_triggered()
{
    QString fileName = s_testMode ? QString() : QFileDialog::getOpenFileName(
        this,
        tr("Open SCPI Log"),
        QString(),
        tr("Encrypted Log Files (*.log);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    bool okPwd = false;
    QString password = QInputDialog::getText(
        this,
        tr("Decrypt Log"),
        tr("Enter the password to decrypt the log:"),
        QLineEdit::Password,
        QString(),
        &okPwd);

    if (!okPwd) {
        return; // user cancelled
    }

    openLogFromFile(fileName, password);
}

bool MainWindow::openLogFromFile(const QString& fileName, const QString& password)
{
    if (fileName.isEmpty()) {
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showError("Could not open file for reading: " + file.errorString());
        return false;
    }

    const QByteArray container = file.readAll();
    file.close();

    if (!LogCrypto::isEncrypted(container)) {
        showError("This file is not a recognized encrypted log.");
        return false;
    }

    bool ok = false;
    const QByteArray plaintext = LogCrypto::decrypt(container, password, &ok);
    if (!ok) {
        showError("Decryption failed - incorrect password or corrupted file.");
        return false;
    }

    const QString contents = QString::fromUtf8(plaintext);

    ui->outputText->clear();
    appendOutput("Loaded log: " + fileName, "#60a5fa");
    appendOutput("─────────────────────────────────────────────", "#6b7280");
    QString escaped = contents.toHtmlEscaped();
    escaped.replace("\n", "<br>");
    ui->outputText->append("<span style='color: #e6e8ec;'>" + escaped + "</span>");

    showSuccess("Log decrypted and loaded successfully.");
    return true;
}

void MainWindow::on_actionAbout_triggered()
{
    QString aboutText = 
        "<h2 style='color: #818cf8;'>Interactive Instrument Communication Tool</h2>"
        "<p style='color: #e6e8ec;'><b>Version:</b> 1.0.0</p>"
        "<p style='color: #e6e8ec;'>A modern GUI application for communicating with SCPI-compatible instruments.</p>"
        "<br>"
        "<p style='color: #34d399;'><b>Supported Protocols:</b></p>"
        "<ul style='color: #9aa1ad;'>"
        "<li>TCP/IP Socket - Direct network communication</li>"
        "<li>VISA - Industry-standard instrument control</li>"
        "</ul>"
        "<br>"
        "<p style='color: #34d399;'><b>Features:</b></p>"
        "<ul style='color: #9aa1ad;'>"
        "<li>Modern dark interface for comfortable viewing</li>"
        "<li>Command history with Up/Down arrow navigation</li>"
        "<li>Auto-completion for previously used commands</li>"
        "<li>Timestamped command and response logging</li>"
        "<li>Support for both write commands and queries</li>"
        "</ul>"
        "<br>"
        "<p style='color: #6b7280;'><i>Built with Qt6 and C++17</i></p>";
    
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutText);
    aboutBox.setIconPixmap(QPixmap()); // No icon
    aboutBox.setStyleSheet(
        "QMessageBox { background-color: #161922; }"
        "QMessageBox QLabel { color: #e6e8ec; background-color: transparent; }"
        "QPushButton { background-color: #6366f1; color: white; border: none; "
        "border-radius: 9px; padding: 9px 20px; min-width: 80px; font-weight: 600; }"
        "QPushButton:hover { background-color: #7c7ff5; }"
    );
    aboutBox.exec();
}
