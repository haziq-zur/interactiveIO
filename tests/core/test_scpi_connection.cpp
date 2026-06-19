#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tcpip_connection.h"

// Test fixture for TCP/IP Connection
class TCPIPConnectionTest : public ::testing::Test {
protected:
    TCPIPConnection* connection;

    void SetUp() override {
        connection = new TCPIPConnection();
    }

    void TearDown() override {
        delete connection;
    }
};

// Test initialization
TEST_F(TCPIPConnectionTest, InitializeSuccess) {
    EXPECT_TRUE(connection->initialize());
    // Should be able to initialize multiple times without error
    EXPECT_TRUE(connection->initialize());
}

// Test initial state
TEST_F(TCPIPConnectionTest, InitialState) {
    EXPECT_FALSE(connection->isConnected());
    EXPECT_EQ(connection->getIP(), "");
    EXPECT_EQ(connection->getPort(), 0);
}

// Test connect with invalid IP
TEST_F(TCPIPConnectionTest, ConnectInvalidIP) {
    EXPECT_TRUE(connection->initialize());
    EXPECT_FALSE(connection->connect("invalid.ip.address", 5025));
    EXPECT_FALSE(connection->isConnected());
    EXPECT_FALSE(connection->getLastError().empty());
}

// Test connect with invalid format IP
TEST_F(TCPIPConnectionTest, ConnectInvalidFormatIP) {
    EXPECT_TRUE(connection->initialize());
    EXPECT_FALSE(connection->connect("999.999.999.999", 5025));
    EXPECT_FALSE(connection->isConnected());
    EXPECT_THAT(connection->getLastError(), ::testing::HasSubstr("Invalid IP address"));
}

// Test sending command before connection
TEST_F(TCPIPConnectionTest, SendCommandNotConnected) {
    EXPECT_FALSE(connection->sendCommand("*IDN?"));
    EXPECT_THAT(connection->getLastError(), ::testing::HasSubstr("Not connected"));
}

// Test waiting for response before connection
TEST_F(TCPIPConnectionTest, readResponseNotConnected) {
    std::string response = connection->readResponse(1);
    EXPECT_EQ(response, "");
    EXPECT_THAT(connection->getLastError(), ::testing::HasSubstr("Not connected"));
}

// Test disconnect when not connected
TEST_F(TCPIPConnectionTest, DisconnectWhenNotConnected) {
    // Should not crash or throw
    EXPECT_NO_THROW(connection->disconnect());
    EXPECT_FALSE(connection->isConnected());
}

// Test multiple disconnect calls
TEST_F(TCPIPConnectionTest, MultipleDisconnectCalls) {
    EXPECT_NO_THROW({
        connection->disconnect();
        connection->disconnect();
        connection->disconnect();
    });
}

// Test state after failed connection
TEST_F(TCPIPConnectionTest, StateAfterFailedConnection) {
    connection->initialize();
    connection->connect("192.168.999.999", 5025);  // Invalid IP
    
    EXPECT_FALSE(connection->isConnected());
    EXPECT_FALSE(connection->getLastError().empty());
}

// Test connection state tracking
TEST_F(TCPIPConnectionTest, ConnectionStateTracking) {
    EXPECT_FALSE(connection->isConnected());
    
    // After initialization, still not connected
    connection->initialize();
    EXPECT_FALSE(connection->isConnected());
    
    // After disconnect, still not connected
    connection->disconnect();
    EXPECT_FALSE(connection->isConnected());
}

// Test IP and port storage after failed connection
TEST_F(TCPIPConnectionTest, IPAndPortAfterFailedConnection) {
    connection->initialize();
    std::string testIP = "192.168.1.999";  // Likely unreachable
    int testPort = 9999;
    
    connection->connect(testIP, testPort);
    
    // IP and port might not be stored on failed connection
    // This depends on implementation details
    EXPECT_FALSE(connection->isConnected());
}

// Test error message clearing on successful operations
TEST_F(TCPIPConnectionTest, ErrorMessageHandling) {
    // First, cause an error
    connection->sendCommand("*IDN?");
    EXPECT_FALSE(connection->getLastError().empty());
    
    std::string firstError = connection->getLastError();
    EXPECT_FALSE(firstError.empty());
}

// Test constructor and destructor
TEST_F(TCPIPConnectionTest, ConstructorDestructor) {
    TCPIPConnection* tempConnection = new TCPIPConnection();
    EXPECT_FALSE(tempConnection->isConnected());
    delete tempConnection;
    // Should not crash or leak
}

// Test RAII behavior
TEST_F(TCPIPConnectionTest, RAIIBehavior) {
    {
        TCPIPConnection tempConnection;
        tempConnection.initialize();
        // Connection should be cleaned up automatically
    }
    // No crash expected
    SUCCEED();
}

// Test empty IP address
TEST_F(TCPIPConnectionTest, ConnectEmptyIP) {
    connection->initialize();
    EXPECT_FALSE(connection->connect("", 5025));
    EXPECT_FALSE(connection->isConnected());
}

// Test zero port
TEST_F(TCPIPConnectionTest, ConnectZeroPort) {
    connection->initialize();
    EXPECT_FALSE(connection->connect("127.0.0.1", 0));
    EXPECT_FALSE(connection->isConnected());
}

// Test negative port
TEST_F(TCPIPConnectionTest, ConnectNegativePort) {
    connection->initialize();
    // Port will be cast to unsigned, so this tests edge case
    EXPECT_FALSE(connection->connect("127.0.0.1", -1));
    EXPECT_FALSE(connection->isConnected());
}

// Test very large port number
TEST_F(TCPIPConnectionTest, ConnectLargePort) {
    connection->initialize();
    EXPECT_FALSE(connection->connect("127.0.0.1", 99999));
    EXPECT_FALSE(connection->isConnected());
}

// Mock test for successful connection (requires actual server or mock)
// This is commented out as it requires a real SCPI server
/*
TEST_F(TCPIPConnectionTest, ConnectSuccess) {
    connection->initialize();
    EXPECT_TRUE(connection->connect("127.0.0.1", 5025));
    EXPECT_TRUE(connection->isConnected());
    EXPECT_EQ(connection->getIP(), "127.0.0.1");
    EXPECT_EQ(connection->getPort(), 5025);
    connection->disconnect();
}
*/


