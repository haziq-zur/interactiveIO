#include "logcrypto.h"

#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <cstdint>
#include <cstring>

// =============================================================================
// Compact AES-256 block cipher (ECB primitive) — derived from the public-domain
// "tiny-AES-c" implementation (Unlicense). Only the pieces needed for a 256-bit
// key are included. CBC chaining and PKCS#7 padding are handled in the helpers
// below.
// =============================================================================
namespace {

constexpr int kAesBlockSize = 16;   // AES block size in bytes
constexpr int kAesKeySize   = 32;   // AES-256 key size in bytes
constexpr int kNk           = 8;    // key length in 32-bit words
constexpr int kNr           = 14;   // number of rounds for AES-256

using state_t = uint8_t[4][4];

const uint8_t sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

const uint8_t rsbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

const uint8_t Rcon[11] = {
    0x8d,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

using RoundKey = uint8_t[(kNr + 1) * 16];

void keyExpansion(RoundKey roundKey, const uint8_t* key)
{
    unsigned i, k;
    uint8_t tempa[4];

    for (i = 0; i < kNk; ++i) {
        roundKey[(i * 4) + 0] = key[(i * 4) + 0];
        roundKey[(i * 4) + 1] = key[(i * 4) + 1];
        roundKey[(i * 4) + 2] = key[(i * 4) + 2];
        roundKey[(i * 4) + 3] = key[(i * 4) + 3];
    }

    for (i = kNk; i < 4 * (kNr + 1); ++i) {
        k = (i - 1) * 4;
        tempa[0] = roundKey[k + 0];
        tempa[1] = roundKey[k + 1];
        tempa[2] = roundKey[k + 2];
        tempa[3] = roundKey[k + 3];

        if (i % kNk == 0) {
            const uint8_t u8tmp = tempa[0];
            tempa[0] = tempa[1];
            tempa[1] = tempa[2];
            tempa[2] = tempa[3];
            tempa[3] = u8tmp;

            tempa[0] = sbox[tempa[0]];
            tempa[1] = sbox[tempa[1]];
            tempa[2] = sbox[tempa[2]];
            tempa[3] = sbox[tempa[3]];

            tempa[0] = tempa[0] ^ Rcon[i / kNk];
        } else if (i % kNk == 4) {
            tempa[0] = sbox[tempa[0]];
            tempa[1] = sbox[tempa[1]];
            tempa[2] = sbox[tempa[2]];
            tempa[3] = sbox[tempa[3]];
        }

        const unsigned j = i * 4;
        k = (i - kNk) * 4;
        roundKey[j + 0] = roundKey[k + 0] ^ tempa[0];
        roundKey[j + 1] = roundKey[k + 1] ^ tempa[1];
        roundKey[j + 2] = roundKey[k + 2] ^ tempa[2];
        roundKey[j + 3] = roundKey[k + 3] ^ tempa[3];
    }
}

void addRoundKey(uint8_t round, state_t* state, const RoundKey roundKey)
{
    for (uint8_t i = 0; i < 4; ++i)
        for (uint8_t j = 0; j < 4; ++j)
            (*state)[i][j] ^= roundKey[(round * 16) + (i * 4) + j];
}

void subBytes(state_t* state)
{
    for (uint8_t i = 0; i < 4; ++i)
        for (uint8_t j = 0; j < 4; ++j)
            (*state)[j][i] = sbox[(*state)[j][i]];
}

void shiftRows(state_t* state)
{
    uint8_t temp;
    temp           = (*state)[0][1];
    (*state)[0][1] = (*state)[1][1];
    (*state)[1][1] = (*state)[2][1];
    (*state)[2][1] = (*state)[3][1];
    (*state)[3][1] = temp;

    temp           = (*state)[0][2];
    (*state)[0][2] = (*state)[2][2];
    (*state)[2][2] = temp;
    temp           = (*state)[1][2];
    (*state)[1][2] = (*state)[3][2];
    (*state)[3][2] = temp;

    temp           = (*state)[0][3];
    (*state)[0][3] = (*state)[3][3];
    (*state)[3][3] = (*state)[2][3];
    (*state)[2][3] = (*state)[1][3];
    (*state)[1][3] = temp;
}

uint8_t xtime(uint8_t x)
{
    return static_cast<uint8_t>((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

void mixColumns(state_t* state)
{
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t t   = (*state)[i][0];
        const uint8_t Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3];
        uint8_t Tm;
        Tm = (*state)[i][0] ^ (*state)[i][1]; Tm = xtime(Tm); (*state)[i][0] ^= Tm ^ Tmp;
        Tm = (*state)[i][1] ^ (*state)[i][2]; Tm = xtime(Tm); (*state)[i][1] ^= Tm ^ Tmp;
        Tm = (*state)[i][2] ^ (*state)[i][3]; Tm = xtime(Tm); (*state)[i][2] ^= Tm ^ Tmp;
        Tm = (*state)[i][3] ^ t;              Tm = xtime(Tm); (*state)[i][3] ^= Tm ^ Tmp;
    }
}

uint8_t multiply(uint8_t x, uint8_t y)
{
    return static_cast<uint8_t>(
        ((y & 1) * x) ^
        ((y >> 1 & 1) * xtime(x)) ^
        ((y >> 2 & 1) * xtime(xtime(x))) ^
        ((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^
        ((y >> 4 & 1) * xtime(xtime(xtime(xtime(x))))));
}

void invMixColumns(state_t* state)
{
    for (int i = 0; i < 4; ++i) {
        const uint8_t a = (*state)[i][0];
        const uint8_t b = (*state)[i][1];
        const uint8_t c = (*state)[i][2];
        const uint8_t d = (*state)[i][3];
        (*state)[i][0] = multiply(a, 0x0e) ^ multiply(b, 0x0b) ^ multiply(c, 0x0d) ^ multiply(d, 0x09);
        (*state)[i][1] = multiply(a, 0x09) ^ multiply(b, 0x0e) ^ multiply(c, 0x0b) ^ multiply(d, 0x0d);
        (*state)[i][2] = multiply(a, 0x0d) ^ multiply(b, 0x09) ^ multiply(c, 0x0e) ^ multiply(d, 0x0b);
        (*state)[i][3] = multiply(a, 0x0b) ^ multiply(b, 0x0d) ^ multiply(c, 0x09) ^ multiply(d, 0x0e);
    }
}

void invSubBytes(state_t* state)
{
    for (uint8_t i = 0; i < 4; ++i)
        for (uint8_t j = 0; j < 4; ++j)
            (*state)[j][i] = rsbox[(*state)[j][i]];
}

void invShiftRows(state_t* state)
{
    uint8_t temp;
    temp           = (*state)[3][1];
    (*state)[3][1] = (*state)[2][1];
    (*state)[2][1] = (*state)[1][1];
    (*state)[1][1] = (*state)[0][1];
    (*state)[0][1] = temp;

    temp           = (*state)[0][2];
    (*state)[0][2] = (*state)[2][2];
    (*state)[2][2] = temp;
    temp           = (*state)[1][2];
    (*state)[1][2] = (*state)[3][2];
    (*state)[3][2] = temp;

    temp           = (*state)[0][3];
    (*state)[0][3] = (*state)[1][3];
    (*state)[1][3] = (*state)[2][3];
    (*state)[2][3] = (*state)[3][3];
    (*state)[3][3] = temp;
}

void cipher(state_t* state, const RoundKey roundKey)
{
    addRoundKey(0, state, roundKey);
    for (uint8_t round = 1; ; ++round) {
        subBytes(state);
        shiftRows(state);
        if (round == kNr) break;
        mixColumns(state);
        addRoundKey(round, state, roundKey);
    }
    addRoundKey(kNr, state, roundKey);
}

void invCipher(state_t* state, const RoundKey roundKey)
{
    addRoundKey(kNr, state, roundKey);
    for (uint8_t round = kNr - 1; ; --round) {
        invShiftRows(state);
        invSubBytes(state);
        addRoundKey(round, state, roundKey);
        if (round == 0) break;
        invMixColumns(state);
    }
}

void xorBlock(uint8_t* buf, const uint8_t* iv)
{
    for (int i = 0; i < kAesBlockSize; ++i)
        buf[i] ^= iv[i];
}

// CBC encrypt in place. length must be a multiple of 16.
void aesCbcEncrypt(uint8_t* buf, size_t length, const RoundKey roundKey, const uint8_t* iv)
{
    uint8_t prev[kAesBlockSize];
    std::memcpy(prev, iv, kAesBlockSize);
    for (size_t i = 0; i < length; i += kAesBlockSize) {
        xorBlock(buf + i, prev);
        cipher(reinterpret_cast<state_t*>(buf + i), roundKey);
        std::memcpy(prev, buf + i, kAesBlockSize);
    }
}

// CBC decrypt in place. length must be a multiple of 16.
void aesCbcDecrypt(uint8_t* buf, size_t length, const RoundKey roundKey, const uint8_t* iv)
{
    uint8_t prev[kAesBlockSize];
    uint8_t cipherBlock[kAesBlockSize];
    std::memcpy(prev, iv, kAesBlockSize);
    for (size_t i = 0; i < length; i += kAesBlockSize) {
        std::memcpy(cipherBlock, buf + i, kAesBlockSize);
        invCipher(reinterpret_cast<state_t*>(buf + i), roundKey);
        xorBlock(buf + i, prev);
        std::memcpy(prev, cipherBlock, kAesBlockSize);
    }
}

const char kMagic[] = { 'I', 'I', 'O', 'L', 'O', 'G', '1', '\0' };
constexpr int kMagicSize = 8;
constexpr int kSaltSize  = 16;
constexpr int kIvSize    = 16;
constexpr int kPbkdf2Iterations = 100000;

QByteArray randomBytes(int n)
{
    QByteArray out(n, Qt::Uninitialized);
    QRandomGenerator::system()->generate(
        reinterpret_cast<quint32*>(out.data()),
        reinterpret_cast<quint32*>(out.data() + (n & ~0x3)));
    // Fill any trailing bytes (n not a multiple of 4).
    for (int i = (n & ~0x3); i < n; ++i)
        out[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    return out;
}

QByteArray deriveKey(const QString& password, const QByteArray& salt)
{
    // PBKDF2-HMAC-SHA256 (RFC 8018). Derives a single 32-byte (256-bit) key,
    // so exactly one block (dkLen == hLen == 32) is required.
    const QByteArray pwd = password.toUtf8();
    const int hLen = 32; // SHA-256 output size

    QByteArray block = salt;
    // INT(1) big-endian block index appended to the salt for the first U_1.
    block.append(char(0x00));
    block.append(char(0x00));
    block.append(char(0x00));
    block.append(char(0x01));

    QMessageAuthenticationCode mac(QCryptographicHash::Sha256, pwd);
    mac.addData(block);
    QByteArray u = mac.result();
    QByteArray result = u;

    for (int iter = 1; iter < kPbkdf2Iterations; ++iter) {
        QMessageAuthenticationCode macIter(QCryptographicHash::Sha256, pwd);
        macIter.addData(u);
        u = macIter.result();
        for (int i = 0; i < hLen; ++i)
            result[i] = result[i] ^ u[i];
    }

    return result.left(kAesKeySize);
}

} // namespace

namespace LogCrypto {

bool isEncrypted(const QByteArray& data)
{
    return data.size() >= kMagicSize &&
           std::memcmp(data.constData(), kMagic, kMagicSize) == 0;
}

QByteArray encrypt(const QByteArray& plaintext, const QString& password)
{
    const QByteArray salt = randomBytes(kSaltSize);
    const QByteArray iv   = randomBytes(kIvSize);
    const QByteArray key  = deriveKey(password, salt);
    if (key.size() != kAesKeySize)
        return QByteArray();

    // PKCS#7 padding to a multiple of the block size.
    const int padLen = kAesBlockSize - (plaintext.size() % kAesBlockSize);
    QByteArray buffer = plaintext;
    buffer.append(QByteArray(padLen, static_cast<char>(padLen)));

    RoundKey roundKey;
    keyExpansion(roundKey, reinterpret_cast<const uint8_t*>(key.constData()));
    aesCbcEncrypt(reinterpret_cast<uint8_t*>(buffer.data()),
                  static_cast<size_t>(buffer.size()),
                  roundKey,
                  reinterpret_cast<const uint8_t*>(iv.constData()));

    QByteArray container;
    container.reserve(kMagicSize + kSaltSize + kIvSize + buffer.size());
    container.append(kMagic, kMagicSize);
    container.append(salt);
    container.append(iv);
    container.append(buffer);
    return container;
}

QByteArray decrypt(const QByteArray& container, const QString& password, bool* ok)
{
    auto fail = [ok]() -> QByteArray {
        if (ok) *ok = false;
        return QByteArray();
    };

    const int headerSize = kMagicSize + kSaltSize + kIvSize;
    if (!isEncrypted(container) || container.size() <= headerSize)
        return fail();

    const int cipherLen = container.size() - headerSize;
    if (cipherLen <= 0 || (cipherLen % kAesBlockSize) != 0)
        return fail();

    const QByteArray salt = container.mid(kMagicSize, kSaltSize);
    const QByteArray iv   = container.mid(kMagicSize + kSaltSize, kIvSize);
    const QByteArray key  = deriveKey(password, salt);
    if (key.size() != kAesKeySize)
        return fail();

    QByteArray buffer = container.mid(headerSize);
    RoundKey roundKey;
    keyExpansion(roundKey, reinterpret_cast<const uint8_t*>(key.constData()));
    aesCbcDecrypt(reinterpret_cast<uint8_t*>(buffer.data()),
                  static_cast<size_t>(buffer.size()),
                  roundKey,
                  reinterpret_cast<const uint8_t*>(iv.constData()));

    // Validate and strip PKCS#7 padding. A wrong password almost always yields
    // invalid padding, which we treat as a decryption failure.
    const int padLen = static_cast<uint8_t>(buffer.at(buffer.size() - 1));
    if (padLen < 1 || padLen > kAesBlockSize || padLen > buffer.size())
        return fail();
    for (int i = 0; i < padLen; ++i) {
        if (static_cast<uint8_t>(buffer.at(buffer.size() - 1 - i)) != padLen)
            return fail();
    }
    buffer.chop(padLen);

    if (ok) *ok = true;
    return buffer;
}

} // namespace LogCrypto
