#ifndef IEEE4882_H
#define IEEE4882_H

#include <cstdint>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// IEEE 488.2 arbitrary block parsing (transport-agnostic, pure helpers).
//
// Many SCPI binary payloads (notably screen captures) are returned as an
// IEEE 488.2 "definite length arbitrary block":
//
//     #<n><length><...length bytes...>[terminator]
//
// where <n> is a single digit giving the number of decimal digits in <length>,
// and <length> is the payload byte count. The "indefinite length" form is
// "#0<...bytes...>\n" with no declared size, terminated by a trailing newline.
//
// These helpers are free of any socket/VISA dependency so they can be unit
// tested in isolation and reused by both transports.
// -----------------------------------------------------------------------------

namespace ieee4882 {

// Describes the leading block header of a buffer.
struct BlockHeader {
    bool headerComplete = false; // true once the full "#n<digits>" prefix is present
    bool indefinite = false;     // true for the "#0" form (no declared length)
    size_t headerLength = 0;     // number of bytes the header occupies
    size_t payloadLength = 0;    // declared payload size (definite form only)
};

// Parses the block header at the start of `data`.
//
// Returns false when `data` does not begin with '#' or the header is malformed
// (filling `error`). Returns true when the buffer begins with a valid header;
// inspect `out.headerComplete` to know whether enough bytes were present to read
// the whole "#n<digits>" prefix (more transport reads are needed when false).
bool parseHeader(const uint8_t* data, size_t size, BlockHeader& out, std::string& error);

// Extracts the payload from a fully received block. For definite blocks the
// header is stripped and exactly `payloadLength` bytes are returned. For
// indefinite blocks everything after the header is returned minus a single
// optional trailing newline. Returns false (filling `error`) when the block is
// incomplete or malformed.
bool extractPayload(const std::vector<uint8_t>& block, std::vector<uint8_t>& payload,
                    std::string& error);

} // namespace ieee4882

#endif // IEEE4882_H
