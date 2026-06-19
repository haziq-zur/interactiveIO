#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class ConnectionDialog; }
QT_END_NAMESPACE

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Protocol {
        TCPIP,
        VISA
    };

    explicit ConnectionDialog(Protocol protocol, QWidget *parent = nullptr);
    ~ConnectionDialog();

    // When enabled, modal message boxes (warnings / info) are suppressed so the
    // dialog can be driven non-interactively from automated tests / CI.
    static void setTestMode(bool enabled);
    static bool isTestMode();

    // TCP/IP getters and setters
    QString getAddress() const;
    void setAddress(const QString& address);
    int getPort() const;
    void setPort(int port);

    // VISA getters and setters
    QString getResourceString() const;
    void setResourceString(const QString& resource);

private slots:
    void on_tcpipHelperButton_clicked();
    void on_socketHelperButton_clicked();
    void on_gpibHelperButton_clicked();
    void on_usbHelperButton_clicked();
    void validateAndAccept();

private:
    Ui::ConnectionDialog *ui;
    Protocol currentProtocol;
    bool isProcessing;
    static bool s_testMode;
    
    void setupForTCPIP();
    void setupForVISA();
    bool validateTCPIP();
    bool validateVISA();
};

#endif // CONNECTIONDIALOG_H
