#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectiondialog.h"
#include "settingsdialog.h"
#include "logcrypto.h"
#include "instrument_controller.h"
#include "iio_version.h"
#include "theme.h"
#include "loadingoverlay.h"
#include "errorhandler.h"
#include <QApplication>
#include <QMessageBox>
#include <QTime>
#include <QCompleter>
#include <QFileDialog>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QPointer>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSignalBlocker>
#include <QColor>
#include <QClipboard>
#include <QAction>
#include <QKeySequence>
#include <QPixmap>
#include <QImage>
#include <QDialog>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <climits>
#include <chrono>
#include <ctime>
#include <thread>

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
    // Default the VISA resource to a VXI-11 (TCPIP INSTR) connection template.
    , lastResourceString("TCPIP::192.168.1.100::INSTR")
    , commandHistoryIndex(-1)
    , captureCommand(":DISPlay:DATA? PNG")
{
    ui->setupUi(this);
    setupConnections();

    // Default the protocol selection to VISA (VXI-11 connections).
    ui->protocolCombo->setCurrentIndex(1);

    // Load the persisted theme before the first UI-state paint so the status
    // badge colours match the active theme.
    currentTheme = theme::loadSavedMode();
    ui->actionToggleTheme->setChecked(currentTheme == theme::Mode::Light);
    ui->actionToggleTheme->setText(
        currentTheme == theme::Mode::Light ? "Dark Mode" : "Light Mode");

    updateUIState(false);
    
    // Set window title
    setWindowTitle("Interactive Instrument Communication Tool");

    // Configure the console log table (columns, sizing, filter wiring).
    setupOutputTable();

    // Animated overlay shown during blocking connect / query operations.
    loadingOverlay = new LoadingOverlay(centralWidget());
    loadingOverlay->setDarkMode(currentTheme == theme::Mode::Dark);

    // Route all controller failures through a single severity-aware handler so
    // the console logging and modal dialogs stay consistent. The logger writes
    // each error line to the console table; dialogs are suppressed in tests.
    errorHandler = std::make_unique<ErrorHandler>(
        this, [this](const QString& text, theme::Output role) {
            appendOutput(text, role);
        });
    errorHandler->setTheme(currentTheme);
    errorHandler->setSilentDialogs(s_testMode);
    
    // Setup command history completer
    setupCommandHistory();
    
    // Setup tooltips for better UX
    setupTooltips();
    
    // Welcome message
    appendOutput("Interactive Instrument Communication Tool", theme::Output::Accent);
    appendOutput("SCPI console  ·  TCP/IP Socket & VISA", theme::Output::Muted);
    appendOutput("Quick tips", theme::Output::Success);
    appendOutput("   •  Select a protocol and click Connect to begin", theme::Output::MutedText);
    appendOutput("   •  Use the Up / Down arrows to browse command history", theme::Output::MutedText);
    appendOutput("   •  Commands ending with '?' are treated as queries", theme::Output::MutedText);
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

    setupCommandPresets();
}

void MainWindow::setupCommandPresets()
{
    // The reserved IEEE 488.2 "common commands" (the *XXX set) plus a few
    // universally supported SCPI system queries. Each entry pairs a readable
    // label with the literal command placed into the input box on selection.
    struct CommandPreset {
        const char *label;    // shown in the dropdown
        const char *command;  // text inserted into the command input
    };

    static const CommandPreset kCommonCommands[] = {
        // IEEE 488.2 mandated common commands (reserved "*" set).
        {"*IDN?  —  Identification query",        "*IDN?"},
        {"*RST  —  Reset to defaults",            "*RST"},
        {"*CLS  —  Clear status",                 "*CLS"},
        {"*ESE  —  Event status enable",          "*ESE "},
        {"*ESE?  —  Event status enable query",   "*ESE?"},
        {"*ESR?  —  Event status register query", "*ESR?"},
        {"*OPC  —  Operation complete",           "*OPC"},
        {"*OPC?  —  Operation complete query",    "*OPC?"},
        {"*OPT?  —  Installed options query",     "*OPT?"},
        {"*PSC  —  Power-on status clear",        "*PSC "},
        {"*PSC?  —  Power-on status clear query", "*PSC?"},
        {"*RCL  —  Recall stored state",          "*RCL "},
        {"*SAV  —  Save current state",           "*SAV "},
        {"*SRE  —  Service request enable",       "*SRE "},
        {"*SRE?  —  Service request enable query","*SRE?"},
        {"*STB?  —  Status byte query",           "*STB?"},
        {"*TRG  —  Trigger",                      "*TRG"},
        {"*TST?  —  Self-test query",             "*TST?"},
        {"*WAI  —  Wait to continue",             "*WAI"},
        // Common SCPI system queries (widely supported).
        {"SYSTem:ERRor?  —  Next error in queue", ":SYSTem:ERRor?"},
        {"SYSTem:VERSion?  —  SCPI version",      ":SYSTem:VERSion?"},
        {"STATus:OPERation?  —  Operation status",":STATus:OPERation?"},
    };

    ui->commandPresetCombo->clear();
    // First entry is a non-selectable prompt so the box reads as a menu.
    ui->commandPresetCombo->addItem(tr("Common SCPI \u25be"), QString());
    for (const auto& preset : kCommonCommands) {
        ui->commandPresetCombo->addItem(tr(preset.label),
                                        QString::fromLatin1(preset.command));
    }
    ui->commandPresetCombo->setToolTip(
        tr("Reserved IEEE 488.2 common commands and standard SCPI queries.\n"
           "Select one to load it into the input box, then press Send / Enter.\n"
           "Commands taking a parameter (e.g. *SAV) are loaded with a trailing\n"
           "space so you can type the value."));

    // Selecting a command loads it into the input for review (some commands such
    // as *RST are destructive), then resets the menu back to its prompt.
    connect(ui->commandPresetCombo, QOverload<int>::of(&QComboBox::activated),
            this, [this](int index) {
        const QString command = ui->commandPresetCombo->itemData(index).toString();
        if (command.isEmpty()) {
            return;  // The prompt row.
        }
        ui->commandInput->setText(command);
        ui->commandInput->setFocus();
        // Place the cursor at the end so parameters can be typed immediately.
        ui->commandInput->end(false);
        // Reset the selector back to the prompt for the next pick.
        ui->commandPresetCombo->setCurrentIndex(0);
    });
}

void MainWindow::setupTooltips()
{
    ui->protocolCombo->setToolTip("Select communication protocol: TCP/IP Socket or VISA");
    ui->connectButton->setToolTip("Connect to the instrument using selected protocol");
    ui->disconnectButton->setToolTip("Disconnect from the currently connected instrument");
    ui->sendButton->setToolTip("Send SCPI command to instrument (or press Enter)");
    ui->clearButton->setToolTip("Clear the output display");
    ui->commandInput->setToolTip("Enter SCPI command here\nUse Up/Down arrows to navigate command history\nCommands ending with '?' are queries that expect a response");
    ui->outputTable->setToolTip("Command and response history with timestamps, instrument and duration");
    ui->addressFilterCombo->setToolTip("Filter the log by instrument address");
    updateCaptureTooltip();
}

void MainWindow::updateCaptureTooltip()
{
    const QString command = captureCommand.trimmed();
    const QString shown = command.isEmpty()
        ? tr("(not set)")
        : command.toHtmlEscaped();
    const QString fmt = captureFormat.isEmpty()
        ? tr("auto-detect")
        : captureFormat.toUpper();

    ui->captureButton->setToolTip(
        tr("<b>Capture a screen image from the instrument</b>"
           "<hr style='margin:2px 0;'>"
           "Current SCPI command:&nbsp; <code>%1</code><br>"
           "Expected format:&nbsp; %2"
           "<hr style='margin:2px 0;'>"
           "The capture command is <b>not the same for all instruments</b>. "
           "To change it, open <b>Settings</b> "
           "(menu&nbsp;\u2192&nbsp;Settings) and edit the "
           "<i>Screen Image Capture</i> section \u2014 pick an instrument preset "
           "or enter your device's command.")
            .arg(shown, fmt));
}

void MainWindow::setupOutputTable()
{
    QTableWidget *table = ui->outputTable;
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        QStringList() << "Time" << "Instrument" << "I/O Data" << "Duration");

    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setWordWrap(true);
    table->setTextElideMode(Qt::ElideNone);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Allow highlighting individual fields (cells) as well as whole rows so any
    // part of the log can be selected and copied.
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setAlternatingRowColors(true);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Copy support: Ctrl+C and a right-click "Copy" action serialize the
    // selected cells to the clipboard (tab-separated columns, newline rows).
    QAction *copyAction = new QAction(tr("Copy"), table);
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copySelectionToClipboard);
    table->addAction(copyAction);

    QAction *selectAllAction = new QAction(tr("Select All"), table);
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    selectAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(selectAllAction, &QAction::triggered, table, &QTableWidget::selectAll);
    table->addAction(selectAllAction);

    table->setContextMenuPolicy(Qt::ActionsContextMenu);

    QHeaderView *header = table->horizontalHeader();
    // Time / Instrument / Duration stay user-resizable; the I/O Data column
    // stretches to absorb the remaining width so the table tracks the window
    // size instead of leaving blank space or forcing a horizontal scrollbar.
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    header->setStretchLastSection(false);
    header->setHighlightSections(false);
    header->setMinimumSectionSize(60);
    header->setCascadingSectionResizes(true);

    table->setColumnWidth(0, 130);   // Time
    table->setColumnWidth(1, 160);   // Instrument
    table->setColumnWidth(3, 110);   // Duration

    // Rows grow to fit wrapped content so long responses stay fully visible.
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // When the I/O Data (stretch) column changes width — e.g. the user resizes
    // the window — row heights must be recomputed so wrapped long responses are
    // not clipped or pushed under the neighbouring column.
    connect(header, &QHeaderView::sectionResized, this,
            [table](int /*index*/, int /*oldSize*/, int /*newSize*/) {
                table->resizeRowsToContents();
            });

    // The Duration column visibility follows the "show response time" setting.
    table->setColumnHidden(3, !controller->communicationSettings().showResponseTime);
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
    ui->commandPresetCombo->setEnabled(connected);
    ui->protocolCombo->setEnabled(!connected);
    ui->captureButton->setEnabled(connected);
    
    if (connected) {
        const QString color =
            (currentTheme == theme::Mode::Light) ? "#059669" : "#34d399";
        const QString bg = (currentTheme == theme::Mode::Light)
            ? "rgba(5, 150, 105, 0.10)" : "rgba(52, 211, 153, 0.12)";
        ui->statusLabel->setText("●  Connected");
        ui->statusLabel->setStyleSheet(
            "QLabel#statusLabel { color: " + color + "; background-color: " + bg + "; "
            "font-weight: 700; font-size: 9pt; padding: 6px 14px; border-radius: 12px; }");
        statusBar()->showMessage("Connected — ready to send commands");
    } else {
        const QString color =
            (currentTheme == theme::Mode::Light) ? "#dc2626" : "#f87171";
        const QString bg = (currentTheme == theme::Mode::Light)
            ? "rgba(220, 38, 38, 0.10)" : "rgba(248, 113, 113, 0.12)";
        ui->statusLabel->setText("●  Disconnected");
        ui->statusLabel->setStyleSheet(
            "QLabel#statusLabel { color: " + color + "; background-color: " + bg + "; "
            "font-weight: 700; font-size: 9pt; padding: 6px 14px; border-radius: 12px; }");
        statusBar()->showMessage("Ready — select a protocol and connect to begin");
    }
}

void MainWindow::appendOutput(const QString& text, theme::Output role)
{
    // System / notice rows carry no instrument address and no duration.
    appendIoRow(QString(), text, QString(), role);
}

void MainWindow::appendIoRow(const QString& address, const QString& ioData,
                             const QString& duration, theme::Output role)
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

    LogEntry entry{ timestamp, address, ioData, duration, role };
    logEntries.push_back(entry);
    registerAddress(address);
    addEntryRow(entry);
}

void MainWindow::addEntryRow(const LogEntry& entry)
{
    QTableWidget *table = ui->outputTable;
    const int row = table->rowCount();
    table->insertRow(row);

    auto makeItem = [](const QString& text, const QString& colorHex) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setForeground(QColor(colorHex));
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        // Full text on hover as a fallback for very long, unwrappable content.
        item->setToolTip(text);
        return item;
    };

    table->setItem(row, 0, makeItem(entry.timestamp, outColor(theme::Output::Timestamp)));
    table->setItem(row, 1, makeItem(entry.address, outColor(theme::Output::MutedText)));
    table->setItem(row, 2, makeItem(entry.ioData, outColor(entry.role)));
    table->setItem(row, 3, makeItem(entry.duration, outColor(theme::Output::Muted)));

    // Honour the active instrument filter for the freshly added row.
    const bool all = (ui->addressFilterCombo->currentIndex() <= 0);
    const QString sel = ui->addressFilterCombo->currentText();
    const bool show = all || entry.address.isEmpty() || entry.address == sel;
    table->setRowHidden(row, !show);

    // Size the row to its (now wrapped) content so long responses are fully
    // visible rather than overlapping the next column.
    table->resizeRowToContents(row);

    table->scrollToBottom();
}

void MainWindow::rebuildOutputTable()
{
    ui->outputTable->setRowCount(0);
    for (const LogEntry& entry : logEntries) {
        addEntryRow(entry);
    }
}

void MainWindow::registerAddress(const QString& address)
{
    if (address.isEmpty()) {
        return;
    }
    if (ui->addressFilterCombo->findText(address) >= 0) {
        return;
    }
    QSignalBlocker blocker(ui->addressFilterCombo);
    ui->addressFilterCombo->addItem(address);
}

void MainWindow::applyAddressFilter()
{
    const bool all = (ui->addressFilterCombo->currentIndex() <= 0);
    const QString sel = ui->addressFilterCombo->currentText();
    for (int row = 0; row < ui->outputTable->rowCount() && row < logEntries.size(); ++row) {
        const QString& addr = logEntries[row].address;
        const bool show = all || addr.isEmpty() || addr == sel;
        ui->outputTable->setRowHidden(row, !show);
    }
}

void MainWindow::on_addressFilterCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    applyAddressFilter();
}

QString MainWindow::consoleText() const
{
    QStringList lines;
    lines.reserve(logEntries.size());
    for (const LogEntry& entry : logEntries) {
        QStringList parts;
        parts << entry.timestamp;
        if (!entry.address.isEmpty()) {
            parts << entry.address;
        }
        parts << entry.ioData;
        if (!entry.duration.isEmpty()) {
            parts << entry.duration;
        }
        lines << parts.join('\t');
    }
    return lines.join('\n');
}

QString MainWindow::outColor(theme::Output role) const
{
    return theme::consoleColor(currentTheme, role);
}

void MainWindow::copySelectionToClipboard() const
{
    const QList<QTableWidgetItem*> items = ui->outputTable->selectedItems();
    if (items.isEmpty()) {
        return;
    }

    // Determine the bounding box of the selection so the copied text preserves
    // the table's row/column layout (tab-separated columns, newline rows).
    int minRow = INT_MAX, maxRow = -1, minCol = INT_MAX, maxCol = -1;
    for (const QTableWidgetItem *item : items) {
        minRow = qMin(minRow, item->row());
        maxRow = qMax(maxRow, item->row());
        minCol = qMin(minCol, item->column());
        maxCol = qMax(maxCol, item->column());
    }

    QStringList rows;
    for (int r = minRow; r <= maxRow; ++r) {
        if (ui->outputTable->isRowHidden(r)) {
            continue;
        }
        QStringList cols;
        for (int c = minCol; c <= maxCol; ++c) {
            if (ui->outputTable->isColumnHidden(c)) {
                continue;
            }
            QTableWidgetItem *item = ui->outputTable->item(r, c);
            QString text = (item && item->isSelected()) ? item->text() : QString();
            // Strip the leading display-only direction markers (› sent, ‹ received)
            // so the copied data contains only the raw command / response text.
            if (text.startsWith(QChar(0x203A)) || text.startsWith(QChar(0x2039))) {
                text = text.mid(1);
                while (text.startsWith(' ')) {
                    text.remove(0, 1);
                }
            }
            cols << text;
        }
        rows << cols.join('\t');
    }

    QApplication::clipboard()->setText(rows.join('\n'));
}

void MainWindow::runWithLoading(const QString& message,
                                std::function<iio::Result()> work,
                                std::function<void(const iio::Result&)> onFinished)
{
    // In test mode (no event loop pumping / determinism), run synchronously.
    if (s_testMode) {
        onFinished(work());
        return;
    }

    loadingOverlay->start(message);

    // Run the blocking controller call off the UI thread so the spinner keeps
    // animating, then marshal the result back to the UI thread.
    QPointer<MainWindow> self(this);
    std::thread([self, work = std::move(work), onFinished = std::move(onFinished)]() mutable {
        iio::Result result = work();
        QMetaObject::invokeMethod(qApp, [self, result, onFinished = std::move(onFinished)]() {
            if (!self) {
                return;
            }
            self->loadingOverlay->stop();
            onFinished(result);
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::showError(const QString& message)
{
    // Route ad-hoc (non-controller) errors through the central handler so the
    // console styling and modal dialog behaviour stay consistent.
    if (errorHandler) {
        errorHandler->reportError(message);
        return;
    }
    appendOutput("✕  ERROR: " + message, theme::Output::Error);
    if (s_testMode) {
        return;
    }
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(message);
    msgBox.setStyleSheet(theme::messageBoxStyleSheet(currentTheme));
    msgBox.exec();
}

void MainWindow::showSuccess(const QString& message)
{
    appendOutput("✓  " + message, theme::Output::Success);
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

        const QString target = lastAddress + ":" + QString::number(lastPort);
        appendOutput("Connecting to " + target + "…", theme::Output::Info);

        iio::ConnectionConfig config;
        config.protocol = iio::Protocol::TCPIP;
        config.address = lastAddress.toStdString();
        config.port = lastPort;

        // Connect on a worker thread with an animated overlay so the window
        // stays responsive while the socket handshake completes.
        runWithLoading("Connecting to " + lastAddress + "…",
            [this, config]() { return controller->connect(config); },
            [this, target](const iio::Result& result) {
                if (!result.success) {
                    errorHandler->handle(result);
                    return;
                }
                currentAddress = target;
                showSuccess("Connected successfully via TCP/IP!");
                updateUIState(true);
                ui->commandInput->setFocus();
            });

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

        appendOutput("Connecting to " + lastResourceString + "…", theme::Output::Info);

        iio::ConnectionConfig config;
        config.protocol = iio::Protocol::VISA;
        config.address = lastResourceString.toStdString();

        runWithLoading("Connecting to instrument…",
            [this, config]() { return controller->connect(config); },
            [this](const iio::Result& result) {
                if (!result.success) {
                    errorHandler->handle(result);
                    return;
                }
                currentAddress = lastResourceString;
                showSuccess("Connected successfully via VISA!");
                updateUIState(true);
                ui->commandInput->setFocus();
            });
    }
}

void MainWindow::on_disconnectButton_clicked()
{
    const bool wasConnected = controller->isConnected();
    controller->disconnect();
    if (wasConnected) {
        appendOutput("Disconnected from instrument.", theme::Output::Warning);
        currentAddress.clear();
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
    
    // Display sent command as an outgoing I/O row.
    appendIoRow(currentAddress, "›  " + command, QString(), theme::Output::Accent);

    const std::string cmd = command.toStdString();
    const bool isQuery = command.contains('?');
    const QString address = currentAddress;

    // Keep the command in the input after a successful send so it can be
    // re-sent or tweaked without retyping. It is select-highlighted on success
    // so typing a new command replaces it immediately.
    const QString sentCommand = command;

    // Queries block until a response (or timeout) arrives, so run them on a
    // worker thread with the loading overlay. Plain writes return quickly but
    // use the same path for consistency.
    const QString busy = isQuery ? "Waiting for response…" : "Sending command…";
    runWithLoading(busy,
        [this, cmd]() { return controller->execute(cmd); },
        [this, isQuery, address, sentCommand](const iio::Result& result) {
            // Time to complete the request, formatted for the Duration column.
            const QString duration =
                QString("%1 ms").arg(result.elapsedMs, 0, 'f', 3);

            if (!result.success) {
                if (result.hasResponse || isQuery) {
                    // Query path: surface timeout / error without tearing down the UI.
                    appendIoRow(address, QString::fromStdString(result.message),
                                duration, ErrorHandler::roleFor(result.severity));
                } else {
                    errorHandler->handle(result);
                }
            } else if (result.hasResponse) {
                appendIoRow(address, "‹  " + QString::fromStdString(result.response),
                            duration, theme::Output::Success);
            } else {
                appendIoRow(address, "✓  Command sent successfully",
                            duration, theme::Output::Success);
            }

            // Retain the last successful command in the input, highlighted so
            // the next keystroke overwrites it while still allowing a re-send.
            if (result.success) {
                ui->commandInput->setText(sentCommand);
                ui->commandInput->selectAll();
                ui->commandInput->setFocus();
            }
        });
}

void MainWindow::on_clearButton_clicked()
{
    ui->outputTable->setRowCount(0);
    logEntries.clear();
    {
        // Reset the instrument filter to just "All Instruments".
        QSignalBlocker blocker(ui->addressFilterCombo);
        while (ui->addressFilterCombo->count() > 1) {
            ui->addressFilterCombo->removeItem(ui->addressFilterCombo->count() - 1);
        }
        ui->addressFilterCombo->setCurrentIndex(0);
    }
    appendOutput("Output cleared.", theme::Output::Muted);
}

void MainWindow::on_captureButton_clicked()
{
    if (!controller->isConnected()) {
        return;
    }

    const QString command = captureCommand.trimmed();
    if (command.isEmpty()) {
        showError("No capture command configured. Set one in Settings first.");
        return;
    }

    const QString address = currentAddress;
    appendIoRow(address, "›  " + command + "  (image capture)", QString(),
                theme::Output::Accent);

    iio::ImageRequest request;
    request.command = command.toStdString();
    request.format = captureFormat.toStdString();

    // Capture is a blocking binary read; run it off the UI thread. The captured
    // bytes are stashed so the completion handler can preview / save them.
    auto capture = std::make_shared<iio::ImageCapture>();
    runWithLoading("Capturing screen image…",
        [this, request, capture]() {
            return controller->captureImage(request, *capture);
        },
        [this, address, capture](const iio::Result& result) {
            const QString duration =
                QString("%1 ms").arg(result.elapsedMs, 0, 'f', 3);

            if (!result.success) {
                if (result.severity == iio::Severity::Warning) {
                    appendIoRow(address, QString::fromStdString(result.message),
                                duration, ErrorHandler::roleFor(result.severity));
                } else {
                    errorHandler->handle(result);
                }
                return;
            }

            // Non-fatal warnings (e.g. format mismatch) are still logged.
            if (result.severity == iio::Severity::Warning) {
                appendIoRow(address, QString::fromStdString(result.message),
                            duration, ErrorHandler::roleFor(result.severity));
            }

            const QString fmt = QString::fromStdString(capture->format);
            appendIoRow(address,
                        QString("‹  captured %1 bytes (%2)")
                            .arg(capture->data.size())
                            .arg(fmt),
                        duration, theme::Output::Success);

            showImagePreview(*capture);
        });
}

void MainWindow::showImagePreview(const iio::ImageCapture& capture)
{
    QImage image;
    const bool loaded = image.loadFromData(
        capture.data.data(), static_cast<int>(capture.data.size()),
        capture.format.empty() ? nullptr : capture.format.c_str());

    if (s_testMode) {
        return;  // No modal dialogs under automated tests.
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Captured Screen"));
    dialog.setObjectName("imagePreviewDialog");
    dialog.setStyleSheet(theme::messageBoxStyleSheet(currentTheme));

    auto *layout = new QVBoxLayout(&dialog);

    if (loaded && !image.isNull()) {
        auto *scroll = new QScrollArea(&dialog);
        auto *label = new QLabel(scroll);
        label->setPixmap(QPixmap::fromImage(image));
        label->setAlignment(Qt::AlignCenter);
        scroll->setWidget(label);
        scroll->setWidgetResizable(true);
        layout->addWidget(scroll, 1);
    } else {
        auto *notice = new QLabel(
            tr("Captured %1 bytes but the image could not be rendered for preview.")
                .arg(capture.data.size()),
            &dialog);
        notice->setWordWrap(true);
        layout->addWidget(notice);
    }

    auto *buttons = new QDialogButtonBox(&dialog);
    QPushButton *saveButton = buttons->addButton(tr("Save…"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, [this, &capture]() {
        saveCapturedImage(capture);
    });
    layout->addWidget(buttons);

    dialog.resize(640, 480);
    dialog.exec();
}

void MainWindow::saveCapturedImage(const iio::ImageCapture& capture)
{
    const QString ext =
        QString::fromStdString(iio::image::fileExtension(capture.format));
    const QString defaultName =
        "capture_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ext;

    const QString fileName = s_testMode ? QString() : QFileDialog::getSaveFileName(
        this, tr("Save Captured Image"), defaultName,
        tr("Image Files (*.png *.bmp *.jpg *.jpeg *.gif);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    const iio::Result result =
        controller->saveImage(capture.data, fileName.toStdString());
    if (result.success) {
        showSuccess(QString::fromStdString(result.message));
    } else {
        errorHandler->handle(result);
    }
}

void MainWindow::on_actionSaveLog_triggered()
{
    if (logEntries.isEmpty()) {
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

    const QByteArray plaintext = consoleText().toUtf8();
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

    // Replace the current log with the decrypted content. Each decrypted line
    // becomes a system row so it renders in the table and is searchable.
    ui->outputTable->setRowCount(0);
    logEntries.clear();
    {
        QSignalBlocker blocker(ui->addressFilterCombo);
        while (ui->addressFilterCombo->count() > 1) {
            ui->addressFilterCombo->removeItem(ui->addressFilterCombo->count() - 1);
        }
        ui->addressFilterCombo->setCurrentIndex(0);
    }

    appendOutput("Loaded log: " + fileName, theme::Output::Info);
    appendOutput("─────────────────────────────────────────────", theme::Output::Muted);
    const QStringList loadedLines = contents.split('\n');
    for (const QString& line : loadedLines) {
        appendOutput(line, theme::Output::MutedText);
    }

    showSuccess("Log decrypted and loaded successfully.");
    return true;
}

void MainWindow::on_actionAbout_triggered()
{
    // Body text colour follows the active theme so it stays readable.
    const QString bodyColor =
        (currentTheme == theme::Mode::Light) ? "#1c2029" : "#e6e8ec";

    QString aboutText =
        "<h2 style='color: #818cf8;'>Interactive Instrument Communication Tool</h2>"
        "<p style='color: " + bodyColor + ";'><b>Version:</b> " IIO_VERSION_FULL "</p>"
        "<p style='color: " + bodyColor + ";'>A modern GUI application for communicating with SCPI-compatible instruments.</p>"
        "<br>"
        "<p style='color: #34d399;'><b>Supported Protocols:</b></p>"
        "<ul style='color: #9aa1ad;'>"
        "<li>TCP/IP Socket - Direct network communication</li>"
        "<li>VISA - Industry-standard instrument control</li>"
        "</ul>"
        "<br>"
        "<p style='color: #34d399;'><b>Features:</b></p>"
        "<ul style='color: #9aa1ad;'>"
        "<li>Light and dark interface themes</li>"
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
    aboutBox.setStyleSheet(theme::messageBoxStyleSheet(currentTheme));
    aboutBox.exec();
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dialog(this);

    // Populate the dialog from the controller's current settings.
    const iio::CommunicationSettings current = controller->communicationSettings();
    dialog.setEolSequence(QString::fromStdString(current.eol));
    dialog.setResponseTimeout(current.responseTimeoutSeconds);
    dialog.setShowResponseTime(current.showResponseTime);
    dialog.setCaptureCommand(captureCommand);
    dialog.setCaptureFormat(captureFormat);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    iio::CommunicationSettings updated;
    updated.eol = dialog.eolSequence().toStdString();
    updated.responseTimeoutSeconds = dialog.responseTimeout();
    updated.showResponseTime = dialog.showResponseTime();
    controller->setCommunicationSettings(updated);

    captureCommand = dialog.captureCommand();
    captureFormat = dialog.captureFormat();
    updateCaptureTooltip();

    // The Duration column is shown only when response timing is enabled.
    ui->outputTable->setColumnHidden(3, !updated.showResponseTime);

    appendOutput("Settings updated  \u00b7  response timeout " +
                     QString::number(updated.responseTimeoutSeconds) + "s" +
                     "  \u00b7  response time " +
                     (updated.showResponseTime ? "on" : "off"),
                 theme::Output::Info);
}

void MainWindow::applyTheme(theme::Mode mode)
{
    currentTheme = mode;

    // Apply globally so the main window and any open dialogs follow suit.
    // Fall back to setting it on this window when there is no QApplication
    // stylesheet owner (e.g. when driven from automated tests).
    if (qApp) {
        qApp->setStyleSheet(theme::styleSheet(mode));
    } else {
        setStyleSheet(theme::styleSheet(mode));
    }

    theme::saveMode(mode);

    ui->actionToggleTheme->setChecked(mode == theme::Mode::Light);
    ui->actionToggleTheme->setText(
        mode == theme::Mode::Light ? "Dark Mode" : "Light Mode");

    // Keep the loading overlay in step with the theme.
    if (loadingOverlay) {
        loadingOverlay->setDarkMode(mode == theme::Mode::Dark);
    }

    // Keep the error handler's dialogs themed to match.
    if (errorHandler) {
        errorHandler->setTheme(mode);
    }

    // Re-render the log rows so their colours match the new theme.
    rebuildOutputTable();

    // Repaint the status badge with theme-appropriate colours.
    updateUIState(controller && controller->isConnected());
}

void MainWindow::on_actionToggleTheme_triggered()
{
    const theme::Mode next =
        (currentTheme == theme::Mode::Dark) ? theme::Mode::Light : theme::Mode::Dark;
    applyTheme(next);

    appendOutput(QString("Switched to %1 mode")
                     .arg(next == theme::Mode::Light ? "light" : "dark"),
                 theme::Output::Info);
}
