#include "rest_server.h"

#include <httplib.h>

#include <algorithm>
#include <cctype>

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

// Writes a JSON body with the given HTTP status onto the response.
void writeJson(httplib::Response& res, int status, const json& body)
{
    res.status = status;
    res.set_content(body.dump(2), "application/json");
}

// Builds a Result-shaped JSON error body for failures that originate in the API
// layer itself (e.g. malformed request bodies) rather than the controller.
json errorBody(const std::string& message, ErrorCode code)
{
    Result r;
    r.success = false;
    r.message = message;
    r.code = code;
    r.severity = Severity::Error;
    return resultToJson(r);
}

// Parses the request body as JSON (treating an empty body as {}). On failure it
// writes a 400 response and returns false so the caller can bail out early.
bool tryParseBody(const httplib::Request& req, httplib::Response& res, json& out)
{
    try {
        out = json::parse(req.body.empty() ? std::string("{}") : req.body);
        return true;
    } catch (const std::exception& e) {
        writeJson(res, 400,
                  errorBody(std::string("Invalid JSON body: ") + e.what(),
                            ErrorCode::InvalidInput));
        return false;
    }
}

json settingsToJson(const CommunicationSettings& s)
{
    return json{
        {"eol", s.eol},
        {"responseTimeoutSeconds", s.responseTimeoutSeconds},
        {"showResponseTime", s.showResponseTime},
    };
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

int httpStatusFor(const Result& result)
{
    if (result.success) {
        return 200; // OK
    }
    switch (result.code) {
        case ErrorCode::InvalidInput:         return 400; // Bad Request
        case ErrorCode::NotConnected:         return 409; // Conflict
        case ErrorCode::UnsupportedProtocol:  return 422; // Unprocessable Entity
        case ErrorCode::OperationUnsupported: return 422;
        case ErrorCode::ProtocolUnavailable:  return 503; // Service Unavailable
        case ErrorCode::NoResponse:           return 504; // Gateway Timeout
        case ErrorCode::ConnectionFailed:
        case ErrorCode::InitializationFailed:
        case ErrorCode::SendFailed:
        case ErrorCode::ClearFailed:          return 502; // Bad Gateway
        case ErrorCode::BinaryReadFailed:     return 502; // Bad Gateway
        case ErrorCode::MalformedBlock:       return 502; // Bad Gateway
        case ErrorCode::UnsupportedImageFormat: return 422; // Unprocessable Entity
        case ErrorCode::ResponseTooLarge:     return 413; // Payload Too Large
        case ErrorCode::None:                 return 500; // Internal Server Error
    }
    return 500;
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
        error = "Request body must be a JSON object";
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

RestServer::RestServer(InstrumentController& controller)
    : controller_(controller)
    , server_(std::make_unique<httplib::Server>())
{
}

RestServer::~RestServer() = default;

void RestServer::registerRoutes()
{
    httplib::Server& srv = *server_;

    // Liveness probe; never touches the instrument.
    srv.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        writeJson(res, 200,
                  json{{"status", "ok"},
                       {"service", "interactiveIO"},
                       {"version", "1.0.0"}});
    });

    // Current connection state.
    srv.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mutex_);
        writeJson(res, 200,
                  json{{"connected", controller_.isConnected()},
                       {"protocol",
                        controller_.currentProtocol() == Protocol::TCPIP ? "tcpip" : "visa"},
                       {"connectionType", controller_.connectionType()},
                       {"connectionInfo", controller_.connectionInfo()}});
    });

    // Open a connection. Body: {"protocol":"tcpip"|"visa","address":...,"port":...}
    srv.Post("/api/connect", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!tryParseBody(req, res, body)) {
            return;
        }
        ConnectionConfig config;
        std::string err;
        if (!parseConnectionConfig(body, config, err)) {
            writeJson(res, 400, errorBody(err, ErrorCode::InvalidInput));
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        const Result result = controller_.connect(config);
        writeJson(res, httpStatusFor(result), resultToJson(result));
    });

    // Close the active connection.
    srv.Post("/api/disconnect", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mutex_);
        const Result result = controller_.disconnect();
        writeJson(res, httpStatusFor(result), resultToJson(result));
    });

    // Send a SCPI command. Body: {"command":"*IDN?","timeoutSeconds":5}
    srv.Post("/api/command", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!tryParseBody(req, res, body)) {
            return;
        }
        if (!body.contains("command") || !body["command"].is_string()
            || body["command"].get<std::string>().empty()) {
            writeJson(res, 400,
                      errorBody("Missing or empty string field 'command'",
                                ErrorCode::InvalidInput));
            return;
        }
        const std::string command = body["command"].get<std::string>();
        const int timeoutSeconds = body.value("timeoutSeconds", -1);
        std::lock_guard<std::mutex> lock(mutex_);
        const Result result = controller_.execute(command, timeoutSeconds);
        writeJson(res, httpStatusFor(result), resultToJson(result));
    });

    // VISA device clear.
    srv.Post("/api/clear", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mutex_);
        const Result result = controller_.clearDevice();
        writeJson(res, httpStatusFor(result), resultToJson(result));
    });

    // Capture a screen image from the instrument.
    // Body: {"command":":DISP:DATA? PNG","format":"png","timeoutSeconds":10,
    //        "maxBytes":33554432,"encoding":"base64"|"binary"}
    srv.Post("/api/image", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!tryParseBody(req, res, body)) {
            return;
        }
        if (!body.contains("command") || !body["command"].is_string()
            || body["command"].get<std::string>().empty()) {
            writeJson(res, 400,
                      errorBody("Missing or empty string field 'command'",
                                ErrorCode::InvalidInput));
            return;
        }

        ImageRequest request;
        request.command = body["command"].get<std::string>();
        request.format = body.value("format", std::string());
        request.timeoutSeconds = body.value("timeoutSeconds", -1);
        if (body.contains("maxBytes") && body["maxBytes"].is_number_unsigned()) {
            request.maxBytes = body["maxBytes"].get<size_t>();
        }
        const std::string encoding = toLower(body.value("encoding", std::string("base64")));

        ImageCapture capture;
        Result result;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            result = controller_.captureImage(request, capture);
        }
        if (!result.success) {
            writeJson(res, httpStatusFor(result), resultToJson(result));
            return;
        }

        if (encoding == "binary") {
            // Stream the raw image bytes with the matching content type.
            const std::string mime =
                capture.format == "png"  ? "image/png"
                : capture.format == "bmp" ? "image/bmp"
                : capture.format == "jpg" ? "image/jpeg"
                : capture.format == "gif" ? "image/gif"
                : "application/octet-stream";
            res.status = 200;
            res.set_content(
                std::string(reinterpret_cast<const char*>(capture.data.data()),
                            capture.data.size()),
                mime);
            return;
        }

        // Default: JSON envelope with base64-encoded payload.
        json j = resultToJson(result);
        j["format"] = capture.format;
        j["bytes"] = capture.data.size();
        j["encoding"] = "base64";
        j["dataBase64"] = base64Encode(capture.data);
        writeJson(res, 200, j);
    });

    // Set the VISA I/O timeout. Body: {"timeoutMs":2000}
    srv.Post("/api/timeout", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!tryParseBody(req, res, body)) {
            return;
        }
        if (!body.contains("timeoutMs") || !body["timeoutMs"].is_number_integer()) {
            writeJson(res, 400,
                      errorBody("Missing integer field 'timeoutMs'",
                                ErrorCode::InvalidInput));
            return;
        }
        const auto timeoutMs = body["timeoutMs"].get<unsigned int>();
        std::lock_guard<std::mutex> lock(mutex_);
        const Result result = controller_.setTimeout(timeoutMs);
        writeJson(res, httpStatusFor(result), resultToJson(result));
    });

    // Read the active communication settings.
    srv.Get("/api/settings", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mutex_);
        writeJson(res, 200, settingsToJson(controller_.communicationSettings()));
    });

    // Update communication settings (partial updates allowed).
    srv.Put("/api/settings", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!tryParseBody(req, res, body)) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        CommunicationSettings settings = controller_.communicationSettings();
        try {
            if (body.contains("eol")) {
                settings.eol = body["eol"].get<std::string>();
            }
            if (body.contains("responseTimeoutSeconds")) {
                settings.responseTimeoutSeconds = body["responseTimeoutSeconds"].get<int>();
            }
            if (body.contains("showResponseTime")) {
                settings.showResponseTime = body["showResponseTime"].get<bool>();
            }
        } catch (const std::exception& e) {
            writeJson(res, 400,
                      errorBody(std::string("Invalid settings value: ") + e.what(),
                                ErrorCode::InvalidInput));
            return;
        }
        controller_.setCommunicationSettings(settings);
        writeJson(res, 200, settingsToJson(controller_.communicationSettings()));
    });

    // Uniform JSON for unhandled routes / server errors.
    srv.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.body.empty()) {
            writeJson(res, res.status,
                      errorBody("No such endpoint or method", ErrorCode::InvalidInput));
        }
    });
}

bool RestServer::listen(const std::string& host, int port)
{
    registerRoutes();
    return server_->listen(host, port);
}

void RestServer::stop()
{
    server_->stop();
}

} // namespace api
} // namespace iio
