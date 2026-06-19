#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>
#include <QPixmap>
#include <QVector>
#include <memory>
#include <functional>
#include "instrument_controller.h"
#include "theme.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class LoadingOverlay;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // When enabled, modal message boxes are suppressed so the window can be
    // driven non-interactively from automated tests / CI.
    static void setTestMode(bool enabled);
    static bool isTestMode();

    // Log persistence. These perform the actual file I/O (independent of the
    // file dialogs) so they can be exercised directly from automated tests.
    // The log is encrypted on save with the given password (AES-256-CBC) and
    // decrypted with the same password on open. Returns true on success.
    bool saveLogToFile(const QString& fileName, const QString& password);
    bool openLogFromFile(const QString& fileName, const QString& password);

    // Plain-text serialization of the console log table (one line per row).
    // Exposed so automated tests can inspect the rendered log content.
    QString consoleText() const;

    // Copies the currently selected console cells to the system clipboard
    // (tab-separated columns, newline-separated rows).
    void copySelectionToClipboard() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_sendButton_clicked();
    void on_clearButton_clicked();
    void on_commandInput_returnPressed();
    void on_protocolCombo_currentIndexChanged(int index);
    void on_actionAbout_triggered();
    void on_actionSaveLog_triggered();
    void on_actionOpenLog_triggered();
    void on_actionSettings_triggered();
    void on_actionToggleTheme_triggered();
    void on_addressFilterCombo_currentIndexChanged(int index);

private:
    // One row of the console log table.
    struct LogEntry {
        QString timestamp;       // HH:mm:ss.uuuuuu
        QString address;         // instrument address (empty for system messages)
        QString ioData;          // request / response / notice text
        QString duration;        // time to complete (e.g. "12.345 ms"), may be empty
        theme::Output role;      // colour role for the I/O data cell
    };

    Ui::MainWindow *ui;
    std::unique_ptr<iio::InstrumentController> controller;
    static bool s_testMode;
    theme::Mode currentTheme;
    
    void setupConnections();
    void setupCommandHistory();
    void setupTooltips();
    void setupOutputTable();
    void applyTheme(theme::Mode mode);
    void updateUIState(bool connected);
    // Appends a system / notice row (no instrument address, no duration).
    void appendOutput(const QString& text, theme::Output role = theme::Output::MutedText);
    // Appends an instrument I/O row tagged with an address and optional duration.
    void appendIoRow(const QString& address, const QString& ioData,
                     const QString& duration, theme::Output role);
    // Adds a single stored entry as a table row (used on append and rebuild).
    void addEntryRow(const LogEntry& entry);
    // Re-creates every table row from logEntries (used after a theme change).
    void rebuildOutputTable();
    // Hides / shows rows according to the instrument filter selection.
    void applyAddressFilter();
    // Adds an address to the filter combo if not already present.
    void registerAddress(const QString& address);
    // Resolves a semantic console colour role to the active theme's hex value.
    QString outColor(theme::Output role) const;
    // Runs a blocking controller operation on a worker thread while showing the
    // loading overlay, then invokes onFinished on the UI thread with the result.
    void runWithLoading(const QString& message,
                        std::function<iio::Result()> work,
                        std::function<void(const iio::Result&)> onFinished);
    void showError(const QString& message);
    void showSuccess(const QString& message);
    void navigateCommandHistory(bool up);
    void addToCommandHistory(const QString& command);
    
    // Connection parameters
    QString lastAddress;
    int lastPort;
    QString lastResourceString;
    
    // Command history
    QStringList commandHistory;
    int commandHistoryIndex;
    QCompleter *historyCompleter;

    // Animated busy overlay shown during blocking operations.
    LoadingOverlay *loadingOverlay;

    // Backing store for the console log table rows.
    QVector<LogEntry> logEntries;
    // Address of the currently connected instrument (tags I/O rows).
    QString currentAddress;
};

#endif // MAINWINDOW_H
