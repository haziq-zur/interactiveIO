#include <iostream>
#include <string>
#include <memory>
#include <sstream>
#include "instrument_connection.h"
#include "tcpip_connection.h"
#include "visa_connection.h"

void printBanner() {
    std::cout << "=================================================\n";
    std::cout << "  Interactive Instrument Communication Tool\n";
    std::cout << "  Supports: TCP/IP Socket & VISA Protocols\n";
    std::cout << "=================================================\n\n";
}

void printProtocolMenu() {
    std::cout << "\nSelect Communication Protocol:\n";
    std::cout << "  1. TCP/IP Socket (Direct Winsock2)\n";
    std::cout << "  2. VISA (GPIB, USB, VXI-11, Serial)\n";
    std::cout << "  3. Exit\n";
    std::cout << "\nChoice: ";
}

std::unique_ptr<IInstrumentConnection> createConnection(int choice) {
    switch (choice) {
        case 1:
            return std::make_unique<TCPIPConnection>();
        case 2:
            return std::make_unique<VISAConnection>();
        default:
            return nullptr;
    }
}

bool setupTCPIPConnection(TCPIPConnection* conn) {
    std::string ip;
    int port;

    std::cout << "\n--- TCP/IP Socket Configuration ---\n";
    std::cout << "Enter IP address (e.g., 192.168.1.100): ";
    std::cin >> ip;
    std::cin.ignore();

    if (!TCPIPUtils::isValidIPAddress(ip)) {
        std::cerr << "Invalid IP address format.\n";
        return false;
    }

    std::cout << "Enter port (e.g., 5025): ";
    std::cin >> port;
    std::cin.ignore();

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number.\n";
        return false;
    }

    std::cout << "Connecting to " << ip << ":" << port << "...\n";
    
    if (!conn->initialize()) {
        std::cerr << "Failed to initialize: " << conn->getLastError() << "\n";
        return false;
    }

    if (!conn->connect(ip, port)) {
        std::cerr << "Failed to connect: " << conn->getLastError() << "\n";
        return false;
    }

    std::cout << "Connected successfully!\n";
    return true;
}

bool setupVISAConnection(VISAConnection* conn) {
    std::cout << "\n--- VISA Configuration ---\n";
    
    if (!conn->isAvailable()) {
        std::cerr << "VISA library not found!\n";
        std::cerr << "Please install NI-VISA from: https://www.ni.com/visa\n";
        return false;
    }

    std::cout << "\nVISA Resource String Examples:\n";
    std::cout << "  TCPIP::192.168.1.100::INSTR        (VXI-11)\n";
    std::cout << "  TCPIP::192.168.1.100::5025::SOCKET (Raw Socket)\n";
    std::cout << "  GPIB0::1::INSTR                    (GPIB)\n";
    std::cout << "  USB0::0x1234::0x5678::SN123::INSTR (USB)\n";
    std::cout << "\nHelper Commands:\n";
    std::cout << "  tcpip <ip>              - Create TCPIP::IP::INSTR\n";
    std::cout << "  socket <ip> <port>      - Create socket resource\n";
    std::cout << "  gpib <board> <address>  - Create GPIB resource\n";

    std::string input;
    std::cout << "\nEnter resource string or helper command: ";
    std::getline(std::cin, input);

    if (input.empty()) {
        std::cerr << "No resource string provided.\n";
        return false;
    }

    // Parse helper commands
    std::string resourceString = input;
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;

    if (cmd == "tcpip") {
        std::string ip;
        iss >> ip;
        resourceString = VISAUtils::buildTCPIPResource(ip);
        std::cout << "Using resource: " << resourceString << "\n";
    } else if (cmd == "socket") {
        std::string ip;
        int port;
        iss >> ip >> port;
        resourceString = VISAUtils::buildTCPIPResource(ip, port);
        std::cout << "Using resource: " << resourceString << "\n";
    } else if (cmd == "gpib") {
        int board, address;
        iss >> board >> address;
        resourceString = VISAUtils::buildGPIBResource(board, address);
        std::cout << "Using resource: " << resourceString << "\n";
    }

    if (!VISAUtils::isValidResourceString(resourceString)) {
        std::cerr << "Invalid VISA resource string format.\n";
        return false;
    }

    std::cout << "Connecting to " << resourceString << "...\n";

    if (!conn->initialize()) {
        std::cerr << "Failed to initialize VISA: " << conn->getLastError() << "\n";
        return false;
    }

    if (!conn->connect(resourceString)) {
        std::cerr << "Failed to connect: " << conn->getLastError() << "\n";
        return false;
    }

    std::cout << "Connected successfully!\n";
    std::cout << "Interface: " << VISAUtils::getInterfaceType(resourceString) << "\n";
    return true;
}

void printCommandHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  help       - Show this help\n";
    std::cout << "  info       - Show connection information\n";
    std::cout << "  timeout    - Set timeout (VISA only)\n";
    std::cout << "  clear      - Clear instrument buffer (VISA only)\n";
    std::cout << "  exit       - Disconnect and return to menu\n";
    std::cout << "  quit       - Exit program\n";
    std::cout << "\nSCPI Commands:\n";
    std::cout << "  *IDN?      - Query instrument identification\n";
    std::cout << "  *RST       - Reset instrument\n";
    std::cout << "  (Any SCPI command supported by your instrument)\n";
}

void interactiveSession(IInstrumentConnection* conn) {
    std::cout << "\n" << conn->getConnectionInfo() << "\n";
    std::cout << "Type 'help' for available commands\n";

    std::string command;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command.empty()) {
            continue;
        }

        if (command == "exit") {
            break;
        }

        if (command == "quit") {
            conn->disconnect();
            exit(0);
        }

        if (command == "help") {
            printCommandHelp();
            continue;
        }

        if (command == "info") {
            std::cout << conn->getConnectionInfo() << "\n";
            std::cout << "Type: " << conn->getConnectionType() << "\n";
            continue;
        }

        if (command == "timeout") {
            VISAConnection* visaConn = dynamic_cast<VISAConnection*>(conn);
            if (visaConn) {
                unsigned int timeout;
                std::cout << "Enter timeout in milliseconds: ";
                std::cin >> timeout;
                std::cin.ignore();
                visaConn->setTimeout(timeout);
                std::cout << "Timeout set to " << timeout << " ms\n";
            } else {
                std::cout << "Timeout command only available for VISA connections\n";
            }
            continue;
        }

        if (command == "clear") {
            VISAConnection* visaConn = dynamic_cast<VISAConnection*>(conn);
            if (visaConn) {
                if (visaConn->clear()) {
                    std::cout << "Instrument buffer cleared\n";
                } else {
                    std::cout << "Clear failed: " << visaConn->getLastError() << "\n";
                }
            } else {
                std::cout << "Clear command only available for VISA connections\n";
            }
            continue;
        }

        // Check if it's a query command
        bool isQuery = command.find('?') != std::string::npos;

        if (isQuery) {
            std::string response = conn->query(command);
            if (!response.empty()) {
                std::cout << "Response: " << response;
                if (response.back() != '\n') {
                    std::cout << "\n";
                }
            } else {
                std::string error = conn->getLastError();
                if (!error.empty()) {
                    std::cout << "Error: " << error << "\n";
                } else {
                    std::cout << "(No response received)\n";
                }
            }
        } else {
            if (conn->sendCommand(command)) {
                std::cout << "Command sent.\n";
            } else {
                std::cout << "Send failed: " << conn->getLastError() << "\n";
                break;
            }
        }
    }

    conn->disconnect();
    std::cout << "\nDisconnected.\n";
}

int main() {
    printBanner();

    while (true) {
        printProtocolMenu();
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 3) {
            std::cout << "\nGoodbye!\n";
            break;
        }

        auto conn = createConnection(choice);
        if (!conn) {
            std::cerr << "Invalid choice. Please try again.\n";
            continue;
        }

        std::cout << "\nSelected Protocol: " << conn->getConnectionType() << "\n";

        bool connected = false;
        if (choice == 1) {
            TCPIPConnection* tcpipConn = dynamic_cast<TCPIPConnection*>(conn.get());
            connected = setupTCPIPConnection(tcpipConn);
        } else if (choice == 2) {
            VISAConnection* visaConn = dynamic_cast<VISAConnection*>(conn.get());
            connected = setupVISAConnection(visaConn);
        }

        if (connected) {
            interactiveSession(conn.get());
        } else {
            std::cout << "\nReturning to protocol menu...\n";
        }
    }

    return 0;
}
