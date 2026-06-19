#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include "connectiondialog.h"

class TestConnectionDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // USB Helper Tests
    void testUSBHelperDecimalConversion();
    void testUSBHelperHexWithPrefix();
    void testUSBHelperHexWithoutPrefix();
    void testUSBHelperWithSerialNumber();
    void testUSBHelperWithoutSerialNumber();
    void testUSBHelperValidation();
    void testUSBHelperMixedFormats();
    void testUSBHelperRealDeviceIDs();
    
    // GPIB Helper Tests
    void testGPIBHelperBasic();
    
    // TCP/IP Helper Tests
    void testTCPIPHelperBasic();

private:
    ConnectionDialog *dialog;
};

void TestConnectionDialog::initTestCase()
{
    // Suppress modal message boxes so the suite runs without user interaction (CI-safe).
    ConnectionDialog::setTestMode(true);
    qDebug() << "Starting ConnectionDialog tests...";
}

void TestConnectionDialog::cleanupTestCase()
{
    qDebug() << "ConnectionDialog tests completed";
}

void TestConnectionDialog::init()
{
    // Create fresh dialog for each test
    dialog = nullptr;
}

void TestConnectionDialog::cleanup()
{
    if (dialog) {
        delete dialog;
        dialog = nullptr;
    }
}

void TestConnectionDialog::testUSBHelperDecimalConversion()
{
    // Test decimal VID/PID conversion to hex
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    QVERIFY(vendorEdit != nullptr);
    QVERIFY(productEdit != nullptr);
    QVERIFY(serialEdit != nullptr);
    QVERIFY(resourceEdit != nullptr);
    QVERIFY(usbButton != nullptr);
    
    // Enter decimal values (10893 = 0x2A8D, 513 = 0x0201)
    vendorEdit->setText("10893");
    productEdit->setText("513");
    serialEdit->setText("MY57702221");
    
    // Simulate button click
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    // Verify correct conversion
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("USB0::0x2a8d::0x0201::MY57702221::INSTR"));
    QVERIFY(!result.contains("::0::INSTR")); // Ensure no extra ::0
}

void TestConnectionDialog::testUSBHelperHexWithPrefix()
{
    // Test hex VID/PID already with 0x prefix
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    vendorEdit->setText("0x1234");
    productEdit->setText("0x5678");
    serialEdit->setText("");
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QVERIFY(result.contains("0x1234"));
    QVERIFY(result.contains("0x5678"));
    QCOMPARE(result, QString("USB0::0x1234::0x5678::INSTR"));
}

void TestConnectionDialog::testUSBHelperHexWithoutPrefix()
{
    // Test hex VID/PID without 0x prefix
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    vendorEdit->setText("ABCD");
    productEdit->setText("EF01");
    serialEdit->setText("");
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("USB0::0xABCD::0xEF01::INSTR"));
}

void TestConnectionDialog::testUSBHelperWithSerialNumber()
{
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    vendorEdit->setText("0x0957");
    productEdit->setText("0x1755");
    serialEdit->setText("MY12345678");
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("USB0::0x0957::0x1755::MY12345678::INSTR"));
}

void TestConnectionDialog::testUSBHelperWithoutSerialNumber()
{
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    vendorEdit->setText("0x0699");
    productEdit->setText("0x0368");
    serialEdit->setText(""); // Empty serial
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("USB0::0x0699::0x0368::INSTR"));
    QVERIFY(!result.contains("::::INSTR")); // No double :: for missing serial
}

void TestConnectionDialog::testUSBHelperValidation()
{
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    // Test with empty vendor ID (should fail)
    vendorEdit->setText("");
    productEdit->setText("0x5678");
    
    QString beforeClick = resourceEdit->text();
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    // Resource should not change because validation failed
    QCOMPARE(resourceEdit->text(), beforeClick);
    
    // Test with empty product ID (should fail)
    vendorEdit->setText("0x1234");
    productEdit->setText("");
    
    beforeClick = resourceEdit->text();
    QTest::mouseClick(usbButton, Qt::LeftButton);
    QCOMPARE(resourceEdit->text(), beforeClick);
}

void TestConnectionDialog::testUSBHelperMixedFormats()
{
    // Test mixing decimal and hex formats
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    // Decimal VID, Hex PID
    vendorEdit->setText("4243"); // 0x1093
    productEdit->setText("A0FF");
    serialEdit->setText("SN123");
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QVERIFY(result.startsWith("USB0::"));
    QVERIFY(result.endsWith("::INSTR"));
    QVERIFY(result.contains("::SN123::"));
}

void TestConnectionDialog::testUSBHelperRealDeviceIDs()
{
    // Test with real instrument IDs from the user's problem
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QLineEdit *vendorEdit = dialog->findChild<QLineEdit*>("helperVendorIdEdit");
    QLineEdit *productEdit = dialog->findChild<QLineEdit*>("helperProductIdEdit");
    QLineEdit *serialEdit = dialog->findChild<QLineEdit*>("helperSerialEdit");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *usbButton = dialog->findChild<QPushButton*>("usbHelperButton");
    
    // Keysight/Agilent device (10893 = 0x2A8D, 513 = 0x0201)
    vendorEdit->setText("10893");
    productEdit->setText("513");
    serialEdit->setText("MY57702221");
    
    QTest::mouseClick(usbButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("USB0::0x2a8d::0x0201::MY57702221::INSTR"));
    
    // Verify no malformed format with extra ::0
    QVERIFY(!result.contains("::0::INSTR"));
    QVERIFY(!result.contains("MY57702221::0"));
    
    // Count colons to ensure correct format (should be 7 for with serial)
    int colonCount = result.count("::");
    QCOMPARE(colonCount, 4); // USB0:: VID:: PID:: SN:: INSTR
}

void TestConnectionDialog::testGPIBHelperBasic()
{
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::VISA);
    
    QSpinBox *boardSpinBox = dialog->findChild<QSpinBox*>("helperBoardSpinBox");
    QSpinBox *addressSpinBox = dialog->findChild<QSpinBox*>("helperAddressSpinBox");
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QPushButton *gpibButton = dialog->findChild<QPushButton*>("gpibHelperButton");
    
    QVERIFY(boardSpinBox != nullptr);
    QVERIFY(addressSpinBox != nullptr);
    QVERIFY(gpibButton != nullptr);
    
    boardSpinBox->setValue(0);
    addressSpinBox->setValue(23);
    
    QTest::mouseClick(gpibButton, Qt::LeftButton);
    
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("GPIB0::23::INSTR"));
}

void TestConnectionDialog::testTCPIPHelperBasic()
{
    dialog = new ConnectionDialog(ConnectionDialog::Protocol::TCPIP);
    
    QLineEdit *ipEdit = dialog->findChild<QLineEdit*>("helperIpEdit");
    QSpinBox *portSpinBox = dialog->findChild<QSpinBox*>("helperPortSpinBox");
    QPushButton *socketButton = dialog->findChild<QPushButton*>("socketHelperButton");
    
    QVERIFY(ipEdit != nullptr);
    QVERIFY(portSpinBox != nullptr);
    QVERIFY(socketButton != nullptr);
    
    ipEdit->setText("192.168.1.100");
    portSpinBox->setValue(5025);
    
    QTest::mouseClick(socketButton, Qt::LeftButton);
    
    // For TCPIP socket protocol, it should generate TCPIP::IP::PORT::SOCKET format
    QLineEdit *resourceEdit = dialog->findChild<QLineEdit*>("resourceEdit");
    QString result = resourceEdit->text();
    QCOMPARE(result, QString("TCPIP::192.168.1.100::5025::SOCKET"));
}

QTEST_MAIN(TestConnectionDialog)
#include "test_gui_connectiondialog.moc"
