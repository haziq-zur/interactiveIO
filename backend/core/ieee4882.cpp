#include "ieee4882.h"

#include <cctype>

namespace ieee4882 {

bool parseHeader(const uint8_t* data, size_t size, BlockHeader& out, std::string& error)
{
    out = BlockHeader{};

    if (data == nullptr || size == 0) {
        // Nothing to inspect yet; not an error, just incomplete.
        out.headerComplete = false;
        return true;
    }

    if (data[0] != '#') {
        error = "Response is not an IEEE 488.2 binary block (missing '#')";
        return false;
    }

    if (size < 2) {
        // Need at least '#' and the digit-count character.
        out.headerComplete = false;
        return true;
    }

    const unsigned char digitCountChar = data[1];
    if (!std::isdigit(digitCountChar)) {
        error = "Malformed binary block header (expected digit after '#')";
        return false;
    }

    const int digitCount = digitCountChar - '0';

    // "#0" is the indefinite-length form: the payload runs until a trailing
    // newline terminator.
    if (digitCount == 0) {
        out.indefinite = true;
        out.headerComplete = true;
        out.headerLength = 2;
        out.payloadLength = 0;
        return true;
    }

    // Definite-length form: 'digitCount' decimal digits follow giving the size.
    const size_t headerLength = 2 + static_cast<size_t>(digitCount);
    if (size < headerLength) {
        // The length digits have not all arrived yet.
        out.headerComplete = false;
        return true;
    }

    size_t payloadLength = 0;
    for (size_t i = 2; i < headerLength; ++i) {
        const unsigned char c = data[i];
        if (!std::isdigit(c)) {
            error = "Malformed binary block header (non-digit in length field)";
            return false;
        }
        payloadLength = payloadLength * 10 + static_cast<size_t>(c - '0');
    }

    out.indefinite = false;
    out.headerComplete = true;
    out.headerLength = headerLength;
    out.payloadLength = payloadLength;
    return true;
}

bool extractPayload(const std::vector<uint8_t>& block, std::vector<uint8_t>& payload,
                    std::string& error)
{
    payload.clear();

    BlockHeader header;
    if (!parseHeader(block.data(), block.size(), header, error)) {
        return false;
    }
    if (!header.headerComplete) {
        error = "Incomplete binary block header";
        return false;
    }

    if (header.indefinite) {
        size_t end = block.size();
        // Strip a single trailing newline terminator if present.
        if (end > header.headerLength && block[end - 1] == '\n') {
            --end;
        }
        payload.assign(block.begin() + static_cast<std::ptrdiff_t>(header.headerLength),
                       block.begin() + static_cast<std::ptrdiff_t>(end));
        return true;
    }

    const size_t required = header.headerLength + header.payloadLength;
    if (block.size() < required) {
        error = "Truncated binary block (expected " + std::to_string(header.payloadLength)
              + " payload bytes, received " + std::to_string(block.size() > header.headerLength
                    ? block.size() - header.headerLength : 0) + ")";
        return false;
    }

    payload.assign(block.begin() + static_cast<std::ptrdiff_t>(header.headerLength),
                   block.begin() + static_cast<std::ptrdiff_t>(required));
    return true;
}

} // namespace ieee4882
