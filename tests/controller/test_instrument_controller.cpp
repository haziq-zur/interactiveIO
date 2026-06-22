#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <vector>

#include "instrument_controller.h"

using iio::InstrumentController;
using iio::ConnectionConfig;
using iio::Protocol;
using iio::Result;
using iio::ErrorCode;
using iio::Severity;

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
    EXPECT_EQ(result.code, ErrorCode::NotConnected);
    EXPECT_EQ(result.severity, Severity::Error);
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
    EXPECT_EQ(result.code, ErrorCode::ConnectionFailed);
    EXPECT_EQ(result.severity, Severity::Error);
    // The controller tracks the selected protocol even on failure.
    EXPECT_EQ(controller.currentProtocol(), Protocol::TCPIP);
}

TEST(InstrumentControllerTest, SetTimeoutWithoutVisaConnectionFails)
{
    InstrumentController controller;
    const Result result = controller.setTimeout(2000);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
    EXPECT_EQ(result.code, ErrorCode::OperationUnsupported);
    EXPECT_EQ(result.severity, Severity::Warning);
}

TEST(InstrumentControllerTest, ClearDeviceWithoutVisaConnectionFails)
{
    InstrumentController controller;
    const Result result = controller.clearDevice();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
    EXPECT_EQ(result.code, ErrorCode::OperationUnsupported);
    EXPECT_EQ(result.severity, Severity::Warning);
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

// --- structured error reporting ---------------------------------------------

TEST(ControllerErrorTest, CodeLabelsAreStable)
{
    EXPECT_STREQ(iio::error::codeLabel(ErrorCode::None), "None");
    EXPECT_STREQ(iio::error::codeLabel(ErrorCode::NotConnected), "NotConnected");
    EXPECT_STREQ(iio::error::codeLabel(ErrorCode::ConnectionFailed), "ConnectionFailed");
    EXPECT_STREQ(iio::error::codeLabel(ErrorCode::NoResponse), "NoResponse");
}

TEST(ControllerErrorTest, SeverityLabelsAreStable)
{
    EXPECT_STREQ(iio::error::severityLabel(Severity::Info), "INFO");
    EXPECT_STREQ(iio::error::severityLabel(Severity::Warning), "WARNING");
    EXPECT_STREQ(iio::error::severityLabel(Severity::Error), "ERROR");
}

TEST(ControllerErrorTest, FormatIncludesSeverityAndCode)
{
    Result result;
    result.success = false;
    result.severity = Severity::Error;
    result.code = ErrorCode::ConnectionFailed;
    result.message = "Failed to connect: host unreachable";
    EXPECT_EQ(iio::error::format(result),
              "[ERROR] ConnectionFailed: Failed to connect: host unreachable");
}

TEST(ControllerErrorTest, FormatOmitsCodeWhenNone)
{
    Result result;
    result.success = true;
    result.severity = Severity::Info;
    result.code = ErrorCode::None;
    result.message = "Connected successfully";
    EXPECT_EQ(iio::error::format(result), "[INFO] Connected successfully");
}

// --- image capture ----------------------------------------------------------

TEST(ControllerImageTest, SniffsKnownFormats)
{
    EXPECT_EQ(iio::image::sniffFormat({0x89, 0x50, 0x4E, 0x47, 0x0D}), "png");
    EXPECT_EQ(iio::image::sniffFormat({0x42, 0x4D, 0x00}), "bmp");
    EXPECT_EQ(iio::image::sniffFormat({0xFF, 0xD8, 0xFF, 0xE0}), "jpg");
    EXPECT_EQ(iio::image::sniffFormat({0x47, 0x49, 0x46, 0x38}), "gif");
    EXPECT_EQ(iio::image::sniffFormat({0x01, 0x02, 0x03}), "");
    EXPECT_EQ(iio::image::sniffFormat({}), "");
}

TEST(ControllerImageTest, FileExtensionFromFormat)
{
    EXPECT_EQ(iio::image::fileExtension("png"), ".png");
    EXPECT_EQ(iio::image::fileExtension(""), ".bin");
}

TEST(ControllerImageTest, CaptureWhenDisconnectedFails)
{
    InstrumentController controller;
    iio::ImageRequest request;
    request.command = ":DISPlay:DATA? PNG";
    iio::ImageCapture capture;
    const Result result = controller.captureImage(request, capture);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ErrorCode::NotConnected);
    EXPECT_TRUE(capture.data.empty());
}

TEST(ControllerImageTest, CaptureWithEmptyCommandRejected)
{
    InstrumentController controller;
    iio::ImageRequest request;  // command left empty
    iio::ImageCapture capture;
    const Result result = controller.captureImage(request, capture);
    EXPECT_FALSE(result.success);
    // Not connected is checked first, but an empty command must never succeed.
    EXPECT_NE(result.code, ErrorCode::None);
}

TEST(ControllerImageTest, SaveImageWritesBytes)
{
    InstrumentController controller;
    const std::vector<uint8_t> data = {0x89, 0x50, 0x4E, 0x47, 0x01, 0x02};
    const std::string path = "controller_test_image_out.png";

    const Result result = controller.saveImage(data, path);
    ASSERT_TRUE(result.success) << result.message;

    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.good());
    const std::vector<uint8_t> readBack((std::istreambuf_iterator<char>(in)),
                                        std::istreambuf_iterator<char>());
    in.close();
    std::remove(path.c_str());
    EXPECT_EQ(readBack, data);
}

TEST(ControllerImageTest, SaveImageRejectsEmptyData)
{
    InstrumentController controller;
    const Result result = controller.saveImage({}, "ignored.png");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.code, ErrorCode::InvalidInput);
}
