#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>
#include <QPixmap>
#include <memory>
#include "instrument_controller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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

private:
    Ui::MainWindow *ui;
    std::unique_ptr<iio::InstrumentController> controller;
    static bool s_testMode;
    
    void setupConnections();
    void setupCommandHistory();
    void setupTooltips();
    void updateUIState(bool connected);
    void appendOutput(const QString& text, const QString& color = "white");
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
};

#endif // MAINWINDOW_H
