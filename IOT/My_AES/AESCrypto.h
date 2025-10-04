// AES-128-GCM helper for ESP32 using mbedtls (migrated from CBC implementation)
// Provides authenticated encryption (confidentiality + integrity).
// Features:
//  * Key normalization to 16 bytes (AES-128)
//  * Random 12-byte nonce (96-bit) generation (hex, 24 chars)
//  * GCM encrypt returns ciphertext + tag (16-byte/32-hex)
//  * GCM decrypt verifies tag (empty string on failure)
//  * Hex helpers reused for all binary fields
//
// Usage example:
//    AESCrypto crypto("16charSecretKey");
//    String nonce = AESCrypto::generateNonce();
//    String cipherHex, tagHex;
//    if (crypto.encrypt("{\"lux\":123}", nonce, cipherHex, tagHex)) {
//        String plain = crypto.decrypt(cipherHex, nonce, tagHex);
//    }
//
// Nonce policy:
//  - Random 96-bit nonces are fine for low/medium volume IoT messages.
//  - Never reuse (key, nonce) pair. If extreme volume: switch to counter-based scheme.
// Limitations:
//  - No AAD currently used; can be added by extending API.
//  - Not thread-safe (typical Arduino single-core usage).
//  - Returns false from encrypt / empty string from decrypt on failure.
#pragma once

#include <Arduino.h>
#include <vector>

class AESCrypto {
public:
	explicit AESCrypto(const String &keyUtf8);

	// Generate random 12-byte nonce (returns 24-char hex string)
	static String generateNonce();

	// Encrypt plaintext with nonce (hex) -> outputs cipher & tag hex; returns true on success
	bool encrypt(const String &plainUtf8, const String &nonceHex, String &cipherHexOut, String &tagHexOut);

	// Decrypt using nonce & tag (hex). Returns plaintext or empty string on auth failure.
	String decrypt(const String &cipherHex, const String &nonceHex, const String &tagHex);

private:
	std::vector<uint8_t> key_; // 16 bytes

	static void hexToBytes(const String &hex, std::vector<uint8_t> &out);
	static String bytesToHex(const uint8_t *buf, size_t len);
};

