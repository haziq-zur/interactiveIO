#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include "instrument_controller.h"
#include "rest_server.h"

// =============================================================================
// REST API entry point.
//
// Launches an HTTP server that exposes the InstrumentController so the
// application can be driven entirely from the command line, e.g.:
//
//   interactiveIO-api --host 127.0.0.1 --port 8080
//
//   curl http://127.0.0.1:8080/api/health
//   curl -X POST http://127.0.0.1:8080/api/connect \
//        -H "Content-Type: application/json" \
//        -d '{"protocol":"tcpip","address":"192.168.1.100","port":5025}'
//   curl -X POST http://127.0.0.1:8080/api/command \
//        -H "Content-Type: application/json" -d '{"command":"*IDN?"}'
//   curl -X POST http://127.0.0.1:8080/api/disconnect
// =============================================================================

namespace {

// Active server pointer so the signal handler can request a clean shutdown.
iio::api::RestServer* g_server = nullptr;

void handleSignal(int)
{
    if (g_server) {
        g_server->stop();
    }
}

void printUsage()
{
    std::cout
        << "Usage: interactiveIO-api [--host <address>] [--port <port>]\n\n"
        << "Options:\n"
        << "  --host <address>   Interface to bind (default 127.0.0.1)\n"
        << "  --port <port>      TCP port to listen on (default 8080)\n"
        << "  --help             Show this help and exit\n\n"
        << "Endpoints:\n"
        << "  GET  /api/health     Liveness probe\n"
        << "  GET  /api/status     Connection state\n"
        << "  POST /api/connect    Open a connection {protocol,address,port}\n"
        << "  POST /api/disconnect Close the connection\n"
        << "  POST /api/command    Send a SCPI command {command,timeoutSeconds?}\n"
        << "  POST /api/clear      VISA device clear\n"
        << "  POST /api/timeout    Set VISA I/O timeout {timeoutMs}\n"
        << "  GET  /api/settings   Read communication settings\n"
        << "  PUT  /api/settings   Update settings {eol,responseTimeoutSeconds,showResponseTime}\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::string host = "127.0.0.1";
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "--help") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port: " << port << " (expected 1-65535)\n";
        return 1;
    }

    iio::InstrumentController controller;
    iio::api::RestServer server(controller);
    g_server = &server;

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::cout << "interactiveIO REST API listening on http://" << host << ":"
              << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n";

    if (!server.listen(host, port)) {
        std::cerr << "Failed to bind " << host << ":" << port
                  << " (port in use or address unavailable)\n";
        return 1;
    }

    std::cout << "Server stopped.\n";
    return 0;
}
