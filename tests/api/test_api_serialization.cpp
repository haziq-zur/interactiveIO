#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "cli_api.h"

// These tests exercise the pure request/response mapping helpers, so they need
// no instrument or running process.

using iio::ConnectionConfig;
using iio::ErrorCode;
using iio::Protocol;
using iio::Result;
using iio::Severity;
using nlohmann::json;

TEST(ApiSerializationTest, SuccessResultSerialises)
{
    Result r;
    r.success = true;
    r.message = "ok";
    r.hasResponse = true;
    r.response = "1.0";
    r.elapsedMs = 12.5;

    const json j = iio::api::resultToJson(r);
    EXPECT_TRUE(j["success"].get<bool>());
    EXPECT_EQ(j["message"], "ok");
    EXPECT_EQ(j["response"], "1.0");
    EXPECT_EQ(j["severity"], "INFO");
    EXPECT_EQ(j["code"], "None");
    EXPECT_DOUBLE_EQ(j["elapsedMs"].get<double>(), 12.5);
    EXPECT_FALSE(j.contains("error"));
}

TEST(ApiSerializationTest, FailureResultIncludesError)
{
    Result r;
    r.success = false;
    r.code = ErrorCode::ConnectionFailed;
    r.severity = Severity::Error;
    r.message = "boom";

    const json j = iio::api::resultToJson(r);
    EXPECT_FALSE(j["success"].get<bool>());
    EXPECT_EQ(j["code"], "ConnectionFailed");
    EXPECT_EQ(j["severity"], "ERROR");
    EXPECT_EQ(j["error"], "[ERROR] ConnectionFailed: boom");
    EXPECT_FALSE(j.contains("response"));
}

TEST(ApiStatusTest, MapsResultsToExitCodes)
{
    Result ok;
    ok.success = true;
    EXPECT_EQ(iio::api::exitCodeFor(ok), 0);

    Result invalid;
    invalid.code = ErrorCode::InvalidInput;
    EXPECT_EQ(iio::api::exitCodeFor(invalid), 2);

    Result notConnected;
    notConnected.code = ErrorCode::NotConnected;
    EXPECT_EQ(iio::api::exitCodeFor(notConnected), 1);

    Result timeout;
    timeout.code = ErrorCode::NoResponse;
    EXPECT_EQ(iio::api::exitCodeFor(timeout), 1);

    Result connFailed;
    connFailed.code = ErrorCode::ConnectionFailed;
    EXPECT_EQ(iio::api::exitCodeFor(connFailed), 1);
}

TEST(ApiStatusTest, ImageErrorCodesAreOperationalFailures)
{
    Result tooLarge;
    tooLarge.code = ErrorCode::ResponseTooLarge;
    EXPECT_EQ(iio::api::exitCodeFor(tooLarge), 1);

    Result malformed;
    malformed.code = ErrorCode::MalformedBlock;
    EXPECT_EQ(iio::api::exitCodeFor(malformed), 1);

    Result binaryFail;
    binaryFail.code = ErrorCode::BinaryReadFailed;
    EXPECT_EQ(iio::api::exitCodeFor(binaryFail), 1);

    Result badFormat;
    badFormat.code = ErrorCode::UnsupportedImageFormat;
    EXPECT_EQ(iio::api::exitCodeFor(badFormat), 1);
}

TEST(ApiBase64Test, EncodesKnownVectors)
{
    using iio::api::base64Encode;
    EXPECT_EQ(base64Encode({}), "");
    EXPECT_EQ(base64Encode({'M'}), "TQ==");
    EXPECT_EQ(base64Encode({'M', 'a'}), "TWE=");
    EXPECT_EQ(base64Encode({'M', 'a', 'n'}), "TWFu");
    EXPECT_EQ(base64Encode({'f', 'o', 'o', 'b', 'a', 'r'}), "Zm9vYmFy");
    // Binary bytes including a PNG signature prefix.
    EXPECT_EQ(base64Encode({0x89, 0x50, 0x4E, 0x47}), "iVBORw==");
}

TEST(ApiParseTest, ParsesProtocols)
{
    Protocol p;
    EXPECT_TRUE(iio::api::parseProtocol("tcpip", p));
    EXPECT_EQ(p, Protocol::TCPIP);
    EXPECT_TRUE(iio::api::parseProtocol("VISA", p));
    EXPECT_EQ(p, Protocol::VISA);
    EXPECT_FALSE(iio::api::parseProtocol("serial", p));
}

TEST(ApiParseTest, ParsesTcpipConfig)
{
    const json body = {{"protocol", "tcpip"}, {"address", "192.168.1.10"}, {"port", 5025}};
    ConnectionConfig cfg;
    std::string err;
    ASSERT_TRUE(iio::api::parseConnectionConfig(body, cfg, err)) << err;
    EXPECT_EQ(cfg.protocol, Protocol::TCPIP);
    EXPECT_EQ(cfg.address, "192.168.1.10");
    EXPECT_EQ(cfg.port, 5025);
}

TEST(ApiParseTest, ParsesVisaConfig)
{
    const json body = {{"protocol", "visa"}, {"address", "TCPIP::192.168.1.10::INSTR"}};
    ConnectionConfig cfg;
    std::string err;
    ASSERT_TRUE(iio::api::parseConnectionConfig(body, cfg, err)) << err;
    EXPECT_EQ(cfg.protocol, Protocol::VISA);
    EXPECT_EQ(cfg.address, "TCPIP::192.168.1.10::INSTR");
}

TEST(ApiParseTest, RejectsMissingAddress)
{
    const json body = {{"protocol", "visa"}};
    ConnectionConfig cfg;
    std::string err;
    EXPECT_FALSE(iio::api::parseConnectionConfig(body, cfg, err));
    EXPECT_FALSE(err.empty());
}

TEST(ApiParseTest, RejectsInvalidPort)
{
    const json body = {{"protocol", "tcpip"}, {"address", "10.0.0.1"}, {"port", 0}};
    ConnectionConfig cfg;
    std::string err;
    EXPECT_FALSE(iio::api::parseConnectionConfig(body, cfg, err));
    EXPECT_FALSE(err.empty());
}
