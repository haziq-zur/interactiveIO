#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h> // Required for InetPton
#include <windows.h>  // Required for MultiByteToWideChar
#pragma comment(lib, "ws2_32.lib")

int main()
{
    std::string ip = "141.183.190.1"; // Set your instrument's IP
    int port = 5025; // Set your instrument's port

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    // Setup address structure
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Convert std::string to std::wstring
    std::wstring wide_ip(ip.begin(), ip.end());

    // Use InetPton instead of inet_pton
    if (InetPtonW(AF_INET, wide_ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address format.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Connect to instrument
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to SCPI instrument at " << ip << ":" << port << std::endl;
    std::cout << "Type SCPI commands (type 'exit' to quit):" << std::endl;

    std::string scpi;
    bool waitingForResponse = false;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, scpi);

        if (scpi == "exit")
            break;

        if (scpi.empty())
            continue;

        // If we're already waiting for a response, warn the user
        if (waitingForResponse) {
            std::cout << "Warning: Still waiting for previous response. Sending new command anyway.\n";
        }

        // Ensure command ends with newline
        if (scpi.back() != '\n')
            scpi += '\n';

        // Send command
        int sent = send(sock, scpi.c_str(), scpi.size(), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "Send failed.\n";
            break;
        }

        // If command ends with '?', expect a response
        if (scpi.find('?') != std::string::npos && scpi.find('?') == scpi.size() - 2) {
            waitingForResponse = true;

            // Use select to check for available data with timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            timeval timeout;
            timeout.tv_sec = 10;  // 10 second timeout
            timeout.tv_usec = 0;

            int selectResult = select(0, &readfds, NULL, NULL, &timeout);

            if (selectResult > 0) {
                // Data is available to read
                if (FD_ISSET(sock, &readfds)) {
                    char buffer[1024] = { 0 };
                    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
                        std::cout << "Response: " << buffer;
                        waitingForResponse = false;
                    }
                    else {
                        std::cerr << "Connection closed or error occurred.\n";
                        waitingForResponse = false;
                        if (bytesReceived == 0) {
                            break; // Connection closed
                        }
                    }
                }
            }
            else if (selectResult == 0) {
                // Timeout occurred
                std::cout << "Timeout waiting for response. You can continue sending commands.\n";
                waitingForResponse = false;
            }
            else {
                // Error occurred
                std::cerr << "Error in select(): " << WSAGetLastError() << std::endl;
                waitingForResponse = false;
            }
        }

        // Check for any pending responses before next command prompt
        if (!waitingForResponse) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            timeval immediateTimeout;
            immediateTimeout.tv_sec = 0;
            immediateTimeout.tv_usec = 0;

            int selectResult = select(0, &readfds, NULL, NULL, &immediateTimeout);

            if (selectResult > 0 && FD_ISSET(sock, &readfds)) {
                char buffer[1024] = { 0 };
                int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';
                    std::cout << "Delayed response: " << buffer;
                }
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}