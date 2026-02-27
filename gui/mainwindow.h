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
#include "instrument_connection.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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

private:
    Ui::MainWindow *ui;
    std::unique_ptr<IInstrumentConnection> connection;
    
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
