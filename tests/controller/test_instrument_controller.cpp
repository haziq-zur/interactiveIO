#include <gtest/gtest.h>

#include "instrument_controller.h"

using iio::InstrumentController;
using iio::ConnectionConfig;
using iio::Protocol;
using iio::Result;

// The controller is exercised without a real instrument, so these tests focus
// on its state machine and the structured results it returns, rather than on
// successful I/O (which requires hardware).

TEST(InstrumentControllerTest, StartsDisconnected)
{
    InstrumentController controller;
    EXPECT_FALSE(controller.isConnected());
}

TEST(InstrumentControllerTest, DefaultProtocolIsTcpip)
{
    InstrumentController controller;
    EXPECT_EQ(controller.currentProtocol(), Protocol::TCPIP);
}

TEST(InstrumentControllerTest, ExecuteWhenDisconnectedFails)
{
    InstrumentController controller;
    const Result result = controller.execute("*IDN?");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.hasResponse);
    EXPECT_FALSE(result.message.empty());
}

TEST(InstrumentControllerTest, DisconnectWhenNotConnectedIsSafe)
{
    InstrumentController controller;
    const Result result = controller.disconnect();
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(controller.isConnected());
}

TEST(InstrumentControllerTest, ConnectToUnreachableTcpipReportsFailure)
{
    InstrumentController controller;
    ConnectionConfig config;
    config.protocol = Protocol::TCPIP;
    config.address = "192.0.2.1"; // TEST-NET-1, guaranteed unroutable
    config.port = 5025;

    const Result result = controller.connect(config);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(controller.isConnected());
    EXPECT_FALSE(result.message.empty());
    // The controller tracks the selected protocol even on failure.
    EXPECT_EQ(controller.currentProtocol(), Protocol::TCPIP);
}

TEST(InstrumentControllerTest, SetTimeoutWithoutVisaConnectionFails)
{
    InstrumentController controller;
    const Result result = controller.setTimeout(2000);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
}

TEST(InstrumentControllerTest, ClearDeviceWithoutVisaConnectionFails)
{
    InstrumentController controller;
    const Result result = controller.clearDevice();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
}

TEST(InstrumentControllerTest, InfoIsEmptyWhenDisconnected)
{
    InstrumentController controller;
    EXPECT_TRUE(controller.connectionInfo().empty());
    EXPECT_TRUE(controller.connectionType().empty());
}

TEST(InstrumentControllerTest, DefaultCommunicationSettings)
{
    InstrumentController controller;
    const iio::CommunicationSettings settings = controller.communicationSettings();
    EXPECT_EQ(settings.eol, "\n");
    EXPECT_EQ(settings.responseTimeoutSeconds, 10);
    EXPECT_TRUE(settings.showResponseTime);
}

TEST(InstrumentControllerTest, SetCommunicationSettingsArePersisted)
{
    InstrumentController controller;
    iio::CommunicationSettings settings;
    settings.eol = "\r\n";
    settings.responseTimeoutSeconds = 30;
    settings.showResponseTime = false;
    controller.setCommunicationSettings(settings);

    const iio::CommunicationSettings stored = controller.communicationSettings();
    EXPECT_EQ(stored.eol, "\r\n");
    EXPECT_EQ(stored.responseTimeoutSeconds, 30);
    EXPECT_FALSE(stored.showResponseTime);
}

// --- resource helpers --------------------------------------------------------

TEST(ControllerResourceTest, ValidatesIpAddresses)
{
    EXPECT_TRUE(iio::resource::isValidIPAddress("192.168.1.100"));
    EXPECT_FALSE(iio::resource::isValidIPAddress("999.999.999.999"));
    EXPECT_FALSE(iio::resource::isValidIPAddress("not-an-ip"));
}

TEST(ControllerResourceTest, BuildsVisaResourceStrings)
{
    EXPECT_EQ(iio::resource::buildTCPIPResource("192.168.1.100"),
              "TCPIP::192.168.1.100::INSTR");
    EXPECT_EQ(iio::resource::buildSocketResource("192.168.1.100", 5025),
              "TCPIP::192.168.1.100::5025::SOCKET");
    EXPECT_EQ(iio::resource::buildGPIBResource(0, 1), "GPIB0::1::INSTR");
}

TEST(ControllerResourceTest, ValidatesResourceStrings)
{
    EXPECT_TRUE(iio::resource::isValidResourceString("TCPIP::192.168.1.100::INSTR"));
    EXPECT_FALSE(iio::resource::isValidResourceString("garbage"));
}
