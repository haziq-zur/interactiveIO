#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ieee4882.h"

using ieee4882::BlockHeader;

namespace {

std::vector<uint8_t> bytes(const std::string& s)
{
    return std::vector<uint8_t>(s.begin(), s.end());
}

} // namespace

TEST(Ieee4882HeaderTest, ParsesDefiniteHeader)
{
    const auto data = bytes("#3123");
    BlockHeader h;
    std::string err;
    ASSERT_TRUE(ieee4882::parseHeader(data.data(), data.size(), h, err)) << err;
    EXPECT_TRUE(h.headerComplete);
    EXPECT_FALSE(h.indefinite);
    EXPECT_EQ(h.headerLength, 5u);     // '#', '3', and three length digits
    EXPECT_EQ(h.payloadLength, 123u);
}

TEST(Ieee4882HeaderTest, ParsesIndefiniteHeader)
{
    const auto data = bytes("#0abc\n");
    BlockHeader h;
    std::string err;
    ASSERT_TRUE(ieee4882::parseHeader(data.data(), data.size(), h, err)) << err;
    EXPECT_TRUE(h.headerComplete);
    EXPECT_TRUE(h.indefinite);
    EXPECT_EQ(h.headerLength, 2u);
}

TEST(Ieee4882HeaderTest, IncompleteHeaderIsNotAnError)
{
    const auto data = bytes("#3");  // length digits not yet arrived
    BlockHeader h;
    std::string err;
    ASSERT_TRUE(ieee4882::parseHeader(data.data(), data.size(), h, err)) << err;
    EXPECT_FALSE(h.headerComplete);
}

TEST(Ieee4882HeaderTest, RejectsNonBlock)
{
    const auto data = bytes("ERROR: no block");
    BlockHeader h;
    std::string err;
    EXPECT_FALSE(ieee4882::parseHeader(data.data(), data.size(), h, err));
    EXPECT_FALSE(err.empty());
}

TEST(Ieee4882HeaderTest, RejectsNonDigitAfterHash)
{
    const auto data = bytes("#X10");
    BlockHeader h;
    std::string err;
    EXPECT_FALSE(ieee4882::parseHeader(data.data(), data.size(), h, err));
    EXPECT_FALSE(err.empty());
}

TEST(Ieee4882PayloadTest, ExtractsDefinitePayload)
{
    // #2 + "05" length + 5 payload bytes, then a trailing newline.
    auto block = bytes("#205");
    const std::vector<uint8_t> payloadBytes = {0x89, 0x50, 0x4E, 0x47, 0x00};
    block.insert(block.end(), payloadBytes.begin(), payloadBytes.end());
    block.push_back('\n');

    std::vector<uint8_t> payload;
    std::string err;
    ASSERT_TRUE(ieee4882::extractPayload(block, payload, err)) << err;
    EXPECT_EQ(payload, payloadBytes);
}

TEST(Ieee4882PayloadTest, ExtractsIndefinitePayloadStrippingNewline)
{
    const auto block = bytes("#0hello\n");
    std::vector<uint8_t> payload;
    std::string err;
    ASSERT_TRUE(ieee4882::extractPayload(block, payload, err)) << err;
    EXPECT_EQ(std::string(payload.begin(), payload.end()), "hello");
}

TEST(Ieee4882PayloadTest, RejectsTruncatedPayload)
{
    // Declares 10 bytes but only provides 3.
    const auto block = bytes("#210abc");
    std::vector<uint8_t> payload;
    std::string err;
    EXPECT_FALSE(ieee4882::extractPayload(block, payload, err));
    EXPECT_FALSE(err.empty());
}
