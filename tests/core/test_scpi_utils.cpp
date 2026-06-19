#include <gtest/gtest.h>
#include <chrono>
#include "tcpip_connection.h"

// Test fixture for TCP/IP utilities
class TCPIPUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test IP address validation
TEST_F(TCPIPUtilsTest, ValidIPAddress) {
    EXPECT_TRUE(TCPIPUtils::isValidIPAddress("192.168.1.1"));
    EXPECT_TRUE(TCPIPUtils::isValidIPAddress("10.0.0.1"));
    EXPECT_TRUE(TCPIPUtils::isValidIPAddress("255.255.255.255"));
    EXPECT_TRUE(TCPIPUtils::isValidIPAddress("0.0.0.0"));
}

TEST_F(TCPIPUtilsTest, InvalidIPAddress) {
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress(""));
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress("256.1.1.1"));
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress("192.168.1"));
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress("abc.def.ghi.jkl"));
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress("192.168.1.1.1"));
    EXPECT_FALSE(TCPIPUtils::isValidIPAddress("192.168.-1.1"));
}

// Test newline handling
TEST_F(TCPIPUtilsTest, EnsureNewline_AlreadyHasNewline) {
    std::string command = "*IDN?\n";
    std::string result = TCPIPUtils::ensureNewline(command);
    EXPECT_EQ(result, "*IDN?\n");
}

TEST_F(TCPIPUtilsTest, EnsureNewline_NoNewline) {
    std::string command = "*IDN?";
    std::string result = TCPIPUtils::ensureNewline(command);
    EXPECT_EQ(result, "*IDN?\n");
}

TEST_F(TCPIPUtilsTest, EnsureNewline_EmptyString) {
    std::string command = "";
    std::string result = TCPIPUtils::ensureNewline(command);
    // Empty string should remain empty (no newline added)
    EXPECT_EQ(result, "");
}

// Test query command detection
TEST_F(TCPIPUtilsTest, IsQueryCommand_ValidQuery) {
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("*IDN?"));
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("*IDN?\n"));
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("MEAS:VOLT?"));
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("MEAS:VOLT? "));
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("MEAS:VOLT?\r\n"));
}

TEST_F(TCPIPUtilsTest, IsQueryCommand_NotQuery) {
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("*RST"));
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("OUTPUT ON"));
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("VOLT 5.0"));
    EXPECT_FALSE(TCPIPUtils::isQueryCommand(""));
}

TEST_F(TCPIPUtilsTest, IsQueryCommand_QuestionMarkInMiddle) {
    // A '?' in the middle followed by other characters should not be considered a query
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("TEST?ABC"));
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("WHAT?THEN"));
}

// Performance test for isValidIPAddress
TEST_F(TCPIPUtilsTest, IsValidIPAddress_Performance) {
    const int iterations = 10000;
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        TCPIPUtils::isValidIPAddress("192.168.1.1");
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // This should complete in reasonable time (< 1 second for 10k iterations)
    EXPECT_LT(duration.count(), 1000000);
}

// Edge case tests
TEST_F(TCPIPUtilsTest, EnsureNewline_MultipleNewlines) {
    std::string command = "*IDN?\n\n";
    std::string result = TCPIPUtils::ensureNewline(command);
    // Should not add another newline if it already has one
    EXPECT_EQ(result, "*IDN?\n\n");
}

TEST_F(TCPIPUtilsTest, IsQueryCommand_OnlyQuestionMark) {
    EXPECT_TRUE(TCPIPUtils::isQueryCommand("?"));
}

TEST_F(TCPIPUtilsTest, IsQueryCommand_MultipleQuestionMarks) {
    // Multiple question marks at the end should not be considered a valid query
    // because the second '?' after the first one is not whitespace
    EXPECT_FALSE(TCPIPUtils::isQueryCommand("TEST??"));
}

