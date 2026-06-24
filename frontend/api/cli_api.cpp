#include "cli_api.h"

#include "iio_version.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace iio {
namespace api {

using nlohmann::json;

namespace {

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Builds a Result-shaped JSON error body for failures that originate in the CLI
// layer itself (e.g. malformed arguments) rather than the controller.
json errorBody(const std::string& message, ErrorCode code)
{
    Result r;
    r.success = false;
    r.message = message;
    r.code = code;
    r.severity = Severity::Error;
    return resultToJson(r);
}

json settingsToJson(const CommunicationSettings& s)
{
    return json{
        {"eol", s.eol},
        {"responseTimeoutSeconds", s.responseTimeoutSeconds},
        {"showResponseTime", s.showResponseTime},
    };
}

// Translates a friendly end-of-line token ("lf"/"cr"/"crlf"/"none") into the
// actual control sequence. Any other value is treated as a literal EOL string.
std::string resolveEol(const std::string& token)
{
    const std::string t = toLower(token);
    if (t == "lf" || t == "\\n") return "\n";
    if (t == "cr" || t == "\\r") return "\r";
    if (t == "crlf" || t == "\\r\\n") return "\r\n";
    if (t == "none" || t == "") return "";
    return token;
}

void printUsage(std::ostream& out)
{
    out
        << "interactiveIO command-line API\n\n"
        << "Usage: interactiveIO-api [connection] [settings] <action>\n\n"
        << "Connection:\n"
        << "  --protocol <tcpip|visa>  Transport (default tcpip)\n"
        << "  --address <addr>         Host IP (tcpip) or VISA resource string\n"
        << "  --port <n>               TCP port for tcpip (default 5025)\n\n"
        << "Settings (optional, applied before the action):\n"
        << "  --eol <lf|cr|crlf|none>  End-of-line appended to commands (default lf)\n"
        << "  --response-timeout <n>   Response timeout in seconds\n"
        << "  --no-response-time       Do not measure elapsed time\n"
        << "  --set-timeout-ms <n>     VISA I/O timeout in milliseconds\n\n"
        << "Actions (choose one):\n"
        << "  --command \"<scpi>\"       Send a SCPI command / query\n"
        << "    --timeout-seconds <n>  Override response timeout for this command\n"
        << "  --clear                  VISA device clear\n"
        << "  --image \"<scpi>\"         Capture an image (IEEE 488.2 binary block)\n"
        << "    --format <png|bmp|jpg> Format hint\n"
        << "    --output <file>        Write raw image bytes to <file>\n"
        << "    --max-bytes <n>        Maximum image size to accept\n"
        << "  --status                 Report connection status only\n"
        << "  --health                 Print service health/version (no connection)\n"
        << "  --help                   Show this help and exit\n\n"
        << "Output: a JSON object on stdout. Exit code 0 on success.\n\n"
        << "Examples:\n"
        << "  interactiveIO-api --address 192.168.1.100 --command \"*IDN?\"\n"
        << "  interactiveIO-api --protocol visa --address \"TCPIP::10.0.0.5::INSTR\" --clear\n"
        << "  interactiveIO-api --address 192.168.1.100 --image \":DISP:DATA? PNG\" --output screen.png\n";
}

} // namespace

json resultToJson(const Result& result)
{
    json j;
    j["success"] = result.success;
    j["message"] = result.message;
    j["severity"] = error::severityLabel(result.severity);
    j["code"] = error::codeLabel(result.code);
    j["elapsedMs"] = result.elapsedMs;
    if (result.hasResponse) {
        j["response"] = result.response;
    }
    if (!result.success) {
        // Single-line, ready-to-display summary for CLI consumers.
        j["error"] = error::format(result);
    }
    return j;
}

int exitCodeFor(const Result& result)
{
    if (result.success) {
        return 0;
    }
    if (result.code == ErrorCode::InvalidInput) {
        return 2; // usage / bad-input error
    }
    return 1; // any other operational failure
}

bool parseProtocol(const std::string& name, Protocol& out)
{
    const std::string n = toLower(name);
    if (n == "tcpip" || n == "tcp" || n == "tcp/ip" || n == "socket") {
        out = Protocol::TCPIP;
        return true;
    }
    if (n == "visa") {
        out = Protocol::VISA;
        return true;
    }
    return false;
}

bool parseConnectionConfig(const json& body, ConnectionConfig& out, std::string& error)
{
    if (!body.is_object()) {
        error = "Connection parameters must form a JSON object";
        return false;
    }
    if (!body.contains("protocol") || !body["protocol"].is_string()) {
        error = "Missing string field 'protocol' (expected 'tcpip' or 'visa')";
        return false;
    }
    Protocol protocol;
    if (!parseProtocol(body["protocol"].get<std::string>(), protocol)) {
        error = "Unknown 'protocol' (expected 'tcpip' or 'visa')";
        return false;
    }
    if (!body.contains("address") || !body["address"].is_string()
        || body["address"].get<std::string>().empty()) {
        error = "Missing or empty string field 'address'";
        return false;
    }

    out.protocol = protocol;
    out.address = body["address"].get<std::string>();
    if (protocol == Protocol::TCPIP) {
        out.port = body.value("port", 5025);
        if (out.port <= 0 || out.port > 65535) {
            error = "'port' out of range (1-65535)";
            return false;
        }
    }
    return true;
}

std::string base64Encode(const std::vector<uint8_t>& data)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 2 < data.size()) {
        const uint32_t n = (static_cast<uint32_t>(data[i]) << 16)
                         | (static_cast<uint32_t>(data[i + 1]) << 8)
                         | static_cast<uint32_t>(data[i + 2]);
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += table[n & 0x3F];
        i += 3;
    }

    const size_t remaining = data.size() - i;
    if (remaining == 1) {
        const uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += '=';
        out += '=';
    } else if (remaining == 2) {
        const uint32_t n = (static_cast<uint32_t>(data[i]) << 16)
                         | (static_cast<uint32_t>(data[i + 1]) << 8);
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += '=';
    }
    return out;
}

namespace {

// Writes a JSON object to `out` (pretty-printed, trailing newline).
void emit(std::ostream& out, const json& body)
{
    out << body.dump(2) << "\n";
}

} // namespace

int runCli(InstrumentController& controller,
           const std::vector<std::string>& args,
           std::ostream& out,
           std::ostream& err)
{
    // --- Parsed argument state ----------------------------------------------
    std::string protocol = "tcpip";
    std::string address;
    int port = 5025;
    bool havePort = false;

    bool haveEol = false;
    std::string eol;
    bool haveResponseTimeout = false;
    int responseTimeout = 0;
    bool noResponseTime = false;
    bool haveSetTimeoutMs = false;
    unsigned int setTimeoutMs = 0;

    // Actions
    bool haveCommand = false;
    std::string command;
    int timeoutSeconds = -1;
    bool clear = false;
    bool haveImage = false;
    std::string imageCommand;
    std::string imageFormat;
    std::string imageOutput;
    bool haveMaxBytes = false;
    size_t maxBytes = 0;
    bool status = false;
    bool health = false;

    // Fetches the value following a flag, reporting a usage error when absent.
    auto needValue = [&](size_t& i, const std::string& flag, std::string& value) -> bool {
        if (i + 1 >= args.size()) {
            err << "Missing value for " << flag << "\n";
            return false;
        }
        value = args[++i];
        return true;
    };

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        std::string value;

        if (arg == "--help" || arg == "-h") {
            printUsage(out);
            return 0;
        } else if (arg == "--protocol") {
            if (!needValue(i, arg, protocol)) return 2;
        } else if (arg == "--address") {
            if (!needValue(i, arg, address)) return 2;
        } else if (arg == "--port") {
            if (!needValue(i, arg, value)) return 2;
            port = std::atoi(value.c_str());
            havePort = true;
        } else if (arg == "--eol") {
            if (!needValue(i, arg, value)) return 2;
            eol = resolveEol(value);
            haveEol = true;
        } else if (arg == "--response-timeout") {
            if (!needValue(i, arg, value)) return 2;
            responseTimeout = std::atoi(value.c_str());
            haveResponseTimeout = true;
        } else if (arg == "--no-response-time") {
            noResponseTime = true;
        } else if (arg == "--set-timeout-ms") {
            if (!needValue(i, arg, value)) return 2;
            setTimeoutMs = static_cast<unsigned int>(std::strtoul(value.c_str(), nullptr, 10));
            haveSetTimeoutMs = true;
        } else if (arg == "--command") {
            if (!needValue(i, arg, command)) return 2;
            haveCommand = true;
        } else if (arg == "--timeout-seconds") {
            if (!needValue(i, arg, value)) return 2;
            timeoutSeconds = std::atoi(value.c_str());
        } else if (arg == "--clear") {
            clear = true;
        } else if (arg == "--image") {
            if (!needValue(i, arg, imageCommand)) return 2;
            haveImage = true;
        } else if (arg == "--format") {
            if (!needValue(i, arg, imageFormat)) return 2;
        } else if (arg == "--output") {
            if (!needValue(i, arg, imageOutput)) return 2;
        } else if (arg == "--max-bytes") {
            if (!needValue(i, arg, value)) return 2;
            maxBytes = static_cast<size_t>(std::strtoull(value.c_str(), nullptr, 10));
            haveMaxBytes = true;
        } else if (arg == "--status") {
            status = true;
        } else if (arg == "--health") {
            health = true;
        } else {
            err << "Unknown argument: " << arg << "\n";
            emit(out, errorBody("Unknown argument: " + arg, ErrorCode::InvalidInput));
            return 2;
        }
    }

    // --- Health: no connection required -------------------------------------
    if (health) {
        emit(out, json{{"status", "ok"},
                       {"service", "interactiveIO"},
                       {"version", IIO_VERSION_STRING},
                       {"build", IIO_VERSION_FULL}});
        return 0;
    }

    // --- Exactly one instrument action --------------------------------------
    const int actionCount =
        (haveCommand ? 1 : 0) + (clear ? 1 : 0) + (haveImage ? 1 : 0) + (status ? 1 : 0);
    if (actionCount > 1) {
        const std::string msg =
            "Choose a single action (--command, --clear, --image or --status)";
        err << msg << "\n";
        emit(out, errorBody(msg, ErrorCode::InvalidInput));
        return 2;
    }

    // --- Build and validate the connection ----------------------------------
    json connBody;
    connBody["protocol"] = protocol;
    connBody["address"] = address;
    if (havePort) {
        connBody["port"] = port;
    }

    ConnectionConfig config;
    std::string cfgError;
    if (!parseConnectionConfig(connBody, config, cfgError)) {
        err << cfgError << "\n";
        emit(out, errorBody(cfgError, ErrorCode::InvalidInput));
        return 2;
    }

    const Result connectResult = controller.connect(config);
    if (!connectResult.success) {
        emit(out, resultToJson(connectResult));
        return exitCodeFor(connectResult);
    }

    // --- Apply settings before the action -----------------------------------
    if (haveEol || haveResponseTimeout || noResponseTime) {
        CommunicationSettings settings = controller.communicationSettings();
        if (haveEol) {
            settings.eol = eol;
        }
        if (haveResponseTimeout) {
            settings.responseTimeoutSeconds = responseTimeout;
        }
        if (noResponseTime) {
            settings.showResponseTime = false;
        }
        controller.setCommunicationSettings(settings);
    }

    if (haveSetTimeoutMs) {
        const Result timeoutResult = controller.setTimeout(setTimeoutMs);
        if (!timeoutResult.success) {
            emit(out, resultToJson(timeoutResult));
            controller.disconnect();
            return exitCodeFor(timeoutResult);
        }
    }

    // --- Perform the requested action ---------------------------------------
    int exitCode = 0;

    if (haveCommand) {
        const Result result = controller.execute(command, timeoutSeconds);
        emit(out, resultToJson(result));
        exitCode = exitCodeFor(result);
    } else if (clear) {
        const Result result = controller.clearDevice();
        emit(out, resultToJson(result));
        exitCode = exitCodeFor(result);
    } else if (haveImage) {
        ImageRequest request;
        request.command = imageCommand;
        request.format = imageFormat;
        request.timeoutSeconds = timeoutSeconds;
        if (haveMaxBytes) {
            request.maxBytes = maxBytes;
        }

        ImageCapture capture;
        const Result result = controller.captureImage(request, capture);
        if (!result.success) {
            emit(out, resultToJson(result));
            exitCode = exitCodeFor(result);
        } else {
            json j = resultToJson(result);
            j["format"] = capture.format;
            j["bytes"] = capture.data.size();
            if (!imageOutput.empty()) {
                const Result save = controller.saveImage(capture.data, imageOutput);
                if (!save.success) {
                    emit(out, resultToJson(save));
                    controller.disconnect();
                    return exitCodeFor(save);
                }
                j["output"] = imageOutput;
                j["encoding"] = "file";
            } else {
                j["encoding"] = "base64";
                j["dataBase64"] = base64Encode(capture.data);
            }
            emit(out, j);
            exitCode = 0;
        }
    } else if (status) {
        emit(out, json{{"connected", controller.isConnected()},
                       {"protocol",
                        controller.currentProtocol() == Protocol::TCPIP ? "tcpip" : "visa"},
                       {"connectionType", controller.connectionType()},
                       {"connectionInfo", controller.connectionInfo()},
                       {"settings", settingsToJson(controller.communicationSettings())}});
        exitCode = 0;
    } else {
        // No explicit action: report a successful connection (a "ping").
        emit(out, json{{"success", true},
                       {"message", "Connected"},
                       {"connected", controller.isConnected()},
                       {"connectionType", controller.connectionType()},
                       {"connectionInfo", controller.connectionInfo()}});
        exitCode = 0;
    }

    controller.disconnect();
    return exitCode;
}

} // namespace api
} // namespace iio
