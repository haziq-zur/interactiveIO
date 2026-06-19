#ifndef LOGCRYPTO_H
#define LOGCRYPTO_H

#include <QByteArray>
#include <QString>

// Lightweight symmetric encryption helper used to protect saved SCPI logs.
//
// Scheme: AES-256-CBC with PKCS#7 padding. The 256-bit key is derived from the
// user supplied password with PBKDF2-HMAC-SHA256 using a per-file random salt.
// A per-file random IV is used for the CBC mode. The on-disk container is:
//
//     magic[8]  "IIOLOG1\0"
//     salt[16]
//     iv[16]
//     ciphertext[...]   (multiple of 16 bytes)
//
// This keeps logs unreadable at rest while allowing the application to decrypt
// them again given the same password.
namespace LogCrypto {

// Encrypts plaintext, returning the full on-disk container (magic + salt + iv +
// ciphertext). Returns an empty QByteArray on failure.
QByteArray encrypt(const QByteArray& plaintext, const QString& password);

// Decrypts a container produced by encrypt(). On success returns the recovered
// plaintext and sets *ok to true (if ok is non-null). On failure (bad magic,
// truncated data, wrong password / corrupt padding) returns an empty
// QByteArray and sets *ok to false.
QByteArray decrypt(const QByteArray& container, const QString& password, bool* ok = nullptr);

// Returns true if the buffer begins with the expected magic header.
bool isEncrypted(const QByteArray& data);

} // namespace LogCrypto

#endif // LOGCRYPTO_H
