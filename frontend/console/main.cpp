#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <memory>

#include "instrument_controller.h"

// =============================================================================
// Console frontend.
//
// This application is presentation-only: it parses user input and prints
// results, but delegates every instrument operation (connect, disconnect,
// command dispatch, VISA timeout / device-clear) to the InstrumentController.
// It depends solely on the controller header and never touches the backend
// transport classes directly.
// =============================================================================

// Global EOL sequence configuration (frontend input preference).
static std::string g_eolSequence = "\n";

static std::string protocolName(iio::Protocol protocol)
{
    return protocol == iio::Protocol::TCPIP ? "TCP/IP Socket" : "VISA";
}

// Prints a failed controller result with its severity + structured error code,
// e.g. "[ERROR] ConnectionFailed: Failed to connect: ...". Errors go to stderr,
// warnings / info to stdout so recoverable conditions don't look like failures.
static void reportResult(const iio::Result& result)
{
    if (result.success) {
        return;
    }
    std::ostream& out =
        (result.severity == iio::Severity::Error) ? std::cerr : std::cout;
    out << iio::error::format(result) << "\n";
}

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

bool setupTCPIPConnection(iio::InstrumentController& controller) {
    std::string ip;
    int port;

    std::cout << "\n--- TCP/IP Socket Configuration ---\n";
    std::cout << "Enter IP address (e.g., 192.168.1.100): ";
    std::cin >> ip;
    std::cin.ignore();

    if (!iio::resource::isValidIPAddress(ip)) {
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

    iio::ConnectionConfig config;
    config.protocol = iio::Protocol::TCPIP;
    config.address = ip;
    config.port = port;

    const iio::Result result = controller.connect(config);
    if (!result.success) {
        reportResult(result);
        return false;
    }

    std::cout << "Connected successfully!\n";
    return true;
}

bool setupVISAConnection(iio::InstrumentController& controller) {
    std::cout << "\n--- VISA Configuration ---\n";

    if (!controller.isProtocolAvailable(iio::Protocol::VISA)) {
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

    // Parse helper commands.
    std::string resourceString = input;
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;

    if (cmd == "tcpip") {
        std::string ip;
        iss >> ip;
        resourceString = iio::resource::buildTCPIPResource(ip);
        std::cout << "Using resource: " << resourceString << "\n";
    } else if (cmd == "socket") {
        std::string ip;
        int port;
        iss >> ip >> port;
        resourceString = iio::resource::buildSocketResource(ip, port);
        std::cout << "Using resource: " << resourceString << "\n";
    } else if (cmd == "gpib") {
        int board, address;
        iss >> board >> address;
        resourceString = iio::resource::buildGPIBResource(board, address);
        std::cout << "Using resource: " << resourceString << "\n";
    }

    if (!iio::resource::isValidResourceString(resourceString)) {
        std::cerr << "Invalid VISA resource string format.\n";
        return false;
    }

    std::cout << "Connecting to " << resourceString << "...\n";

    iio::ConnectionConfig config;
    config.protocol = iio::Protocol::VISA;
    config.address = resourceString;

    const iio::Result result = controller.connect(config);
    if (!result.success) {
        reportResult(result);
        return false;
    }

    std::cout << "Connected successfully!\n";
    std::cout << "Interface: " << iio::resource::interfaceType(resourceString) << "\n";
    return true;
}

void printCommandHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  help       - Show this help\n";
    std::cout << "  info       - Show connection information\n";
    std::cout << "  connect    - Connect/reconnect to instrument\n";
    std::cout << "  disconnect - Disconnect from current instrument\n";
    std::cout << "  eol        - Set EOL sequence (default: \\n)\n";
    std::cout << "  time       - Toggle showing time to complete (default: on)\n";
    std::cout << "  timeout    - Set timeout (VISA only)\n";
    std::cout << "  clear      - Clear instrument buffer (VISA only)\n";
    std::cout << "  image      - Capture a screen image from the instrument\n";
    std::cout << "  exit       - Return to protocol menu\n";
    std::cout << "  quit       - Exit program\n";
    std::cout << "\nSCPI Commands:\n";
    std::cout << "  *IDN?      - Query instrument identification\n";
    std::cout << "  *RST       - Reset instrument\n";
    std::cout << "  (Any SCPI command supported by your instrument)\n";
}

std::string escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string unescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case 'n': result += '\n'; i++; break;
                case 'r': result += '\r'; i++; break;
                case 't': result += '\t'; i++; break;
                case '\\': result += '\\'; i++; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

void interactiveSession(iio::InstrumentController& controller, iio::Protocol protocol) {
    // Apply the current EOL preference to the controller, which now owns EOL
    // handling and appends it to every dispatched command.
    {
        iio::CommunicationSettings settings = controller.communicationSettings();
        settings.eol = g_eolSequence;
        controller.setCommunicationSettings(settings);
    }

    if (controller.isConnected()) {
        std::cout << "\n" << controller.connectionInfo() << "\n";
    }
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
            controller.disconnect();
            exit(0);
        }

        if (command == "help") {
            printCommandHelp();
            continue;
        }

        if (command == "info") {
            if (controller.isConnected()) {
                std::cout << controller.connectionInfo() << "\n";
                std::cout << "Type: " << controller.connectionType() << "\n";
                std::cout << "EOL Sequence: " << escapeString(g_eolSequence) << "\n";
            } else {
                std::cout << "Not connected to any instrument\n";
                std::cout << "EOL Sequence: " << escapeString(g_eolSequence) << "\n";
            }
            continue;
        }

        if (command == "connect" || command == "reconnect") {
            if (controller.isConnected()) {
                std::cout << "Disconnecting from current instrument...\n";
                controller.disconnect();
            }

            bool reconnected = (protocol == iio::Protocol::TCPIP)
                ? setupTCPIPConnection(controller)
                : setupVISAConnection(controller);

            if (reconnected) {
                std::cout << controller.connectionInfo() << "\n";
            }
            continue;
        }

        if (command == "disconnect") {
            if (controller.isConnected()) {
                controller.disconnect();
                std::cout << "Disconnected from instrument\n";
            } else {
                std::cout << "Not currently connected\n";
            }
            continue;
        }

        if (command == "eol") {
            std::cout << "Current EOL sequence: " << escapeString(g_eolSequence) << "\n";
            std::cout << "Enter new EOL sequence (use \\n, \\r, \\t for special chars): ";
            std::string newEol;
            std::getline(std::cin, newEol);
            if (!newEol.empty()) {
                g_eolSequence = unescapeString(newEol);
                iio::CommunicationSettings settings = controller.communicationSettings();
                settings.eol = g_eolSequence;
                controller.setCommunicationSettings(settings);
                std::cout << "EOL sequence set to: " << escapeString(g_eolSequence) << "\n";
            }
            continue;
        }

        if (command == "time") {
            iio::CommunicationSettings settings = controller.communicationSettings();
            settings.showResponseTime = !settings.showResponseTime;
            controller.setCommunicationSettings(settings);
            std::cout << "Show time to complete: "
                      << (settings.showResponseTime ? "on" : "off") << "\n";
            continue;
        }

        if (command == "timeout") {
            unsigned int timeout;
            std::cout << "Enter timeout in milliseconds: ";
            std::cin >> timeout;
            std::cin.ignore();
            const iio::Result result = controller.setTimeout(timeout);
            if (result.success) {
                std::cout << result.message << "\n";
            } else {
                reportResult(result);
            }
            continue;
        }

        if (command == "clear") {
            const iio::Result result = controller.clearDevice();
            if (result.success) {
                std::cout << result.message << "\n";
            } else {
                reportResult(result);
            }
            continue;
        }

        if (command == "image" || command == "screenshot") {
            if (!controller.isConnected()) {
                std::cout << "Not connected. Use 'connect' command first.\n";
                continue;
            }

            iio::ImageRequest request;

            std::cout << "Enter SCPI capture command (e.g. :DISPlay:DATA? PNG): ";
            std::string captureCommand;
            std::getline(std::cin, captureCommand);
            if (captureCommand.empty()) {
                std::cout << "No capture command entered; aborting.\n";
                continue;
            }
            request.command = captureCommand;

            std::cout << "Expected format (png/bmp/jpg/gif, blank to auto-detect): ";
            std::string format;
            std::getline(std::cin, format);
            request.format = format;

            iio::ImageCapture capture;
            const iio::Result result = controller.captureImage(request, capture);
            if (!result.success) {
                reportResult(result);
                continue;
            }
            // Non-fatal warnings (e.g. format mismatch) are surfaced too.
            if (result.severity == iio::Severity::Warning) {
                reportResult(result);
            }

            std::string defaultName =
                "capture" + iio::image::fileExtension(capture.format);
            std::cout << "Enter output file path (default: " << defaultName << "): ";
            std::string path;
            std::getline(std::cin, path);
            if (path.empty()) {
                path = defaultName;
            }

            const iio::Result saved = controller.saveImage(capture.data, path);
            if (saved.success) {
                std::cout << "Saved " << capture.data.size() << " bytes ("
                          << capture.format << ") to " << path << "\n";
                if (controller.communicationSettings().showResponseTime) {
                    std::cout << "Captured in " << std::fixed << std::setprecision(3)
                              << capture.elapsedMs << " ms\n";
                }
            } else {
                reportResult(saved);
            }
            continue;
        }

        // Check if connected before sending commands.
        if (!controller.isConnected()) {
            std::cout << "Not connected. Use 'connect' command first.\n";
            continue;
        }

        // Dispatch via the controller, which appends the configured EOL
        // sequence, auto-detects queries and reads back responses.
        const iio::Result result = controller.execute(command);

        if (result.hasResponse) {
            std::cout << "Response: " << result.response;
            if (result.response.empty() || result.response.back() != '\n') {
                std::cout << "\n";
            }
        } else if (result.success) {
            std::cout << "Command sent.\n";
        } else {
            reportResult(result);
        }

        // Report how long the command took, when enabled in settings.
        if (controller.communicationSettings().showResponseTime) {
            std::cout << "Completed in " << std::fixed << std::setprecision(3)
                      << result.elapsedMs << " ms\n";
        }
    }

    if (controller.isConnected()) {
        controller.disconnect();
        std::cout << "\nDisconnected.\n";
    }
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

        if (choice != 1 && choice != 2) {
            std::cerr << "Invalid choice. Please try again.\n";
            continue;
        }

        const iio::Protocol protocol =
            (choice == 1) ? iio::Protocol::TCPIP : iio::Protocol::VISA;

        iio::InstrumentController controller;

        std::cout << "\nSelected Protocol: " << protocolName(protocol) << "\n";
        std::cout << "\nConnect now? (y/n, default: y): ";
        std::string connectChoice;
        std::getline(std::cin, connectChoice);

        if (connectChoice.empty() || connectChoice == "y" || connectChoice == "Y" || connectChoice == "yes") {
            if (protocol == iio::Protocol::TCPIP) {
                setupTCPIPConnection(controller);
            } else {
                setupVISAConnection(controller);
            }
        } else {
            std::cout << "\nEntering interactive mode. Use 'connect' command to connect later.\n";
        }

        // Enter interactive session regardless of connection status.
        interactiveSession(controller, protocol);
    }

    return 0;
}
