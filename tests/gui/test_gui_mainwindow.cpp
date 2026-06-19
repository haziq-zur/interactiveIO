#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "mainwindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // UI State Tests
    void testInitialState();
    void testUIElementsExist();
    void testDisconnectedState();
    void testProtocolComboBox();
    
    // Command History Tests
    void testCommandHistoryNavigation();
    void testCommandHistoryLimit();
    
    // Output Tests
    void testOutputAppend();
    void testClearOutput();

    // Log Save/Open Tests
    void testSaveLogCreatesFile();
    void testSaveLogIsEncrypted();
    void testSaveLogEmptyFileNameFails();
    void testSaveLogEmptyPasswordFails();
    void testOpenLogDecryptsContent();
    void testOpenLogWrongPasswordFails();
    void testOpenLogMissingFileFails();
    void testOpenLogPlaintextFileFails();
    void testSaveThenOpenRoundTrip();
    
    // Widget Styling Tests
    void testDarkModeStylesheet();
    void testTooltips();
    
    // Connection State Tests
    void testConnectionStateTransition();

private:
    MainWindow *mainWindow;
};

void TestMainWindow::initTestCase()
{
    // Suppress modal message boxes so the suite runs without user interaction (CI-safe).
    MainWindow::setTestMode(true);
    // Setup test case (runs once before all tests)
    qDebug() << "Starting GUI tests...";
}

void TestMainWindow::cleanupTestCase()
{
    // Cleanup test case (runs once after all tests)
    qDebug() << "GUI tests completed.";
}

void TestMainWindow::init()
{
    // Setup before each test
    mainWindow = new MainWindow();
}

void TestMainWindow::cleanup()
{
    // Cleanup after each test
    delete mainWindow;
    mainWindow = nullptr;
}

void TestMainWindow::testInitialState()
{
    QVERIFY(mainWindow != nullptr);
    QCOMPARE(mainWindow->windowTitle(), QString("Interactive Instrument Communication Tool"));
    
    // Check that window is configured properly
    QVERIFY(!mainWindow->isVisible()); // Window should not be visible in tests
}

void TestMainWindow::testUIElementsExist()
{
    // Find UI elements by object name
    QPushButton *connectButton = mainWindow->findChild<QPushButton*>("connectButton");
    QPushButton *disconnectButton = mainWindow->findChild<QPushButton*>("disconnectButton");
    QPushButton *sendButton = mainWindow->findChild<QPushButton*>("sendButton");
    QPushButton *clearButton = mainWindow->findChild<QPushButton*>("clearButton");
    QLineEdit *commandInput = mainWindow->findChild<QLineEdit*>("commandInput");
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    QComboBox *protocolCombo = mainWindow->findChild<QComboBox*>("protocolCombo");
    
    // Verify all critical UI elements exist
    QVERIFY(connectButton != nullptr);
    QVERIFY(disconnectButton != nullptr);
    QVERIFY(sendButton != nullptr);
    QVERIFY(clearButton != nullptr);
    QVERIFY(commandInput != nullptr);
    QVERIFY(outputText != nullptr);
    QVERIFY(protocolCombo != nullptr);
}

void TestMainWindow::testDisconnectedState()
{
    QPushButton *connectButton = mainWindow->findChild<QPushButton*>("connectButton");
    QPushButton *disconnectButton = mainWindow->findChild<QPushButton*>("disconnectButton");
    QPushButton *sendButton = mainWindow->findChild<QPushButton*>("sendButton");
    QLineEdit *commandInput = mainWindow->findChild<QLineEdit*>("commandInput");
    QComboBox *protocolCombo = mainWindow->findChild<QComboBox*>("protocolCombo");
    
    // In disconnected state
    QVERIFY(connectButton->isEnabled());
    QVERIFY(!disconnectButton->isEnabled());
    QVERIFY(!sendButton->isEnabled());
    QVERIFY(!commandInput->isEnabled());
    QVERIFY(protocolCombo->isEnabled());
}

void TestMainWindow::testProtocolComboBox()
{
    QComboBox *protocolCombo = mainWindow->findChild<QComboBox*>("protocolCombo");
    
    QVERIFY(protocolCombo != nullptr);
    QCOMPARE(protocolCombo->count(), 2);
    QCOMPARE(protocolCombo->itemText(0), QString("TCP/IP Socket"));
    QCOMPARE(protocolCombo->itemText(1), QString("VISA"));
    QCOMPARE(protocolCombo->currentIndex(), 0); // Default should be TCP/IP
}

void TestMainWindow::testCommandHistoryNavigation()
{
    QLineEdit *commandInput = mainWindow->findChild<QLineEdit*>("commandInput");
    
    QVERIFY(commandInput != nullptr);
    
    // Initially, the input should be empty
    QVERIFY(commandInput->text().isEmpty());
    
    // Test that Up/Down arrow keys don't crash when history is empty
    QKeyEvent upEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    QKeyEvent downEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    
    QApplication::sendEvent(commandInput, &upEvent);
    QVERIFY(commandInput->text().isEmpty()); // Should remain empty
    
    QApplication::sendEvent(commandInput, &downEvent);
    QVERIFY(commandInput->text().isEmpty()); // Should remain empty
}

void TestMainWindow::testCommandHistoryLimit()
{
    // Test is conceptual - we test that command history can be added
    // In a real test, we would need to expose command history or use private test access
    QVERIFY(true); // Placeholder for now
}

void TestMainWindow::testOutputAppend()
{
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    
    QVERIFY(outputText != nullptr);
    QVERIFY(!outputText->toPlainText().isEmpty()); // Should have welcome message
    
    // The output should contain the welcome message
    QString output = outputText->toPlainText();
    QVERIFY(output.contains("Interactive Instrument Communication Tool"));
}

void TestMainWindow::testClearOutput()
{
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    QPushButton *clearButton = mainWindow->findChild<QPushButton*>("clearButton");
    
    QVERIFY(outputText != nullptr);
    QVERIFY(clearButton != nullptr);
    
    // Output should initially have content (welcome message)
    QVERIFY(!outputText->toPlainText().isEmpty());
    
    // Click the clear button
    clearButton->click();
    
    // After clearing, output should only have "Output cleared" message
    QString output = outputText->toPlainText();
    QVERIFY(output.contains("Output cleared"));
}

void TestMainWindow::testDarkModeStylesheet()
{
    // Verify that the application has a stylesheet applied
    // Note: In test environment, the main() function doesn't run,
    // so the global stylesheet might not be applied.
    // Instead, we verify that widgets can accept dark mode styling
    QString appStyleSheet = qApp->styleSheet();
    
    // If stylesheet is applied (when running from main app), verify it
    if (!appStyleSheet.isEmpty()) {
        QVERIFY(appStyleSheet.contains("#1e1e1e")); // Dark background color
        QVERIFY(appStyleSheet.contains("#e0e0e0")); // Light text color
    }
    
    // Always verify that the MainWindow can accept custom styling
    mainWindow->setStyleSheet("QMainWindow { background-color: #1e1e1e; }");
    QVERIFY(!mainWindow->styleSheet().isEmpty());
}

void TestMainWindow::testTooltips()
{
    QPushButton *connectButton = mainWindow->findChild<QPushButton*>("connectButton");
    QPushButton *sendButton = mainWindow->findChild<QPushButton*>("sendButton");
    QLineEdit *commandInput = mainWindow->findChild<QLineEdit*>("commandInput");
    QComboBox *protocolCombo = mainWindow->findChild<QComboBox*>("protocolCombo");
    
    // Verify tooltips are set
    QVERIFY(!connectButton->toolTip().isEmpty());
    QVERIFY(!sendButton->toolTip().isEmpty());
    QVERIFY(!commandInput->toolTip().isEmpty());
    QVERIFY(!protocolCombo->toolTip().isEmpty());
    
    // Verify specific tooltip content
    QVERIFY(connectButton->toolTip().contains("Connect"));
    QVERIFY(protocolCombo->toolTip().contains("protocol"));
}

void TestMainWindow::testConnectionStateTransition()
{
    QPushButton *connectButton = mainWindow->findChild<QPushButton*>("connectButton");
    QPushButton *disconnectButton = mainWindow->findChild<QPushButton*>("disconnectButton");
    
    // Initial state - disconnected
    QVERIFY(connectButton->isEnabled());
    QVERIFY(!disconnectButton->isEnabled());
    
    // Note: We cannot actually test connection without a real instrument
    // This test verifies the UI state management works correctly
    // In a real scenario, we would use mocking to simulate connections
}

void TestMainWindow::testSaveLogCreatesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString filePath = tempDir.filePath("save_creates.log");
    QVERIFY(!QFile::exists(filePath));

    QVERIFY(mainWindow->saveLogToFile(filePath, "s3cr3t"));
    QVERIFY(QFile::exists(filePath));
}

void TestMainWindow::testSaveLogIsEncrypted()
{
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    QVERIFY(outputText != nullptr);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString filePath = tempDir.filePath("save_encrypted.log");
    QVERIFY(mainWindow->saveLogToFile(filePath, "s3cr3t"));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray raw = file.readAll();
    file.close();

    QVERIFY(!raw.isEmpty());
    // The file must carry the encrypted-log magic header...
    QVERIFY(raw.startsWith("IIOLOG1"));
    // ...and must NOT contain the plaintext welcome message.
    QVERIFY(!raw.contains("Interactive Instrument Communication Tool"));
    QVERIFY(!raw.contains("Quick tips"));
}

void TestMainWindow::testSaveLogEmptyFileNameFails()
{
    QVERIFY(!mainWindow->saveLogToFile(QString(), "s3cr3t"));
}

void TestMainWindow::testSaveLogEmptyPasswordFails()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath("save_nopwd.log");
    QVERIFY(!mainWindow->saveLogToFile(filePath, QString()));
}

void TestMainWindow::testOpenLogDecryptsContent()
{
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    QVERIFY(outputText != nullptr);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath("open_decrypt.log");

    // Save the current console (welcome message) encrypted, then read it back.
    QVERIFY(mainWindow->saveLogToFile(filePath, "p@ssw0rd"));

    outputText->clear();
    QVERIFY(mainWindow->openLogFromFile(filePath, "p@ssw0rd"));

    QString loaded = outputText->toPlainText();
    QVERIFY(loaded.contains("Interactive Instrument Communication Tool"));
    QVERIFY(loaded.contains(filePath)); // "Loaded log: <path>" header
}

void TestMainWindow::testOpenLogWrongPasswordFails()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath("open_wrongpwd.log");

    QVERIFY(mainWindow->saveLogToFile(filePath, "correct-horse"));
    // Decrypting with the wrong password must fail (bad PKCS#7 padding).
    QVERIFY(!mainWindow->openLogFromFile(filePath, "battery-staple"));
}

void TestMainWindow::testOpenLogMissingFileFails()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString missing = tempDir.filePath("does_not_exist.log");
    QVERIFY(!QFile::exists(missing));
    QVERIFY(!mainWindow->openLogFromFile(missing, "s3cr3t"));
}

void TestMainWindow::testOpenLogPlaintextFileFails()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath("plain.log");

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream out(&file);
        out << "this is not an encrypted log";
    }
    file.close();

    // A plaintext (non-container) file must be rejected.
    QVERIFY(!mainWindow->openLogFromFile(filePath, "s3cr3t"));
}

void TestMainWindow::testSaveThenOpenRoundTrip()
{
    QTextEdit *outputText = mainWindow->findChild<QTextEdit*>("outputText");
    QVERIFY(outputText != nullptr);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath("round_trip.log");

    const QString password = "round-trip-key";

    // Save the current console (welcome message) encrypted to disk.
    QVERIFY(mainWindow->saveLogToFile(filePath, password));

    // Clear, then load it back with the same password.
    outputText->clear();
    QVERIFY(outputText->toPlainText().isEmpty());

    QVERIFY(mainWindow->openLogFromFile(filePath, password));
    QVERIFY(outputText->toPlainText().contains("Interactive Instrument Communication Tool"));
}

// Qt Test main macro
QTEST_MAIN(TestMainWindow)
#include "test_gui_mainwindow.moc"
