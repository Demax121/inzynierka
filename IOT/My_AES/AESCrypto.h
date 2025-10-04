// Minimal AES-128-CBC helper for ESP32 using mbedtls
// Provides:
//  * Key normalization to 16 bytes (AES-128)
//  * Random IV generation (esp_random) returned as lowercase hex
//  * PKCS#7 padding/unpadding
//  * Hex encoding/decoding for ciphertext & IV (transport-friendly)
//  * Simple Arduino String interface for ease of integration in small sketches
//
// Limitations / Notes:
//  - No message authentication (consider HMAC or migrate to AES-GCM for integrity).
//  - Uses dynamic allocations via std::vector (acceptable for small payload sizes).
//  - hexToBytes silently ignores odd-length hex (returns empty vector implied error on caller path).
//  - Not thread-safe (intended for single-task use typical in Arduino context).
//
// Typical usage:
//    AESCrypto crypto("16charSecretKey!");
//    String iv  = AESCrypto::generateIV();
//    String cph = crypto.encrypt("{\"doorOpen\":true}", iv);
//    String plain = crypto.decrypt(cph, iv);
//
// Security recommendation:
//    Pair with an authenticity layer if operating across untrusted networks.
#pragma once

#include <Arduino.h>
#include <vector>

class AESCrypto {
public:
	explicit AESCrypto(const String &keyUtf8);

	// Generate random 16-byte IV, returned as lower-case hex (32 chars)
	static String generateIV();

	// Encrypt plaintext (UTF-8) with given ivHex (32 hex chars). Returns hex ciphertext.
	String encrypt(const String &plainUtf8, const String &ivHex);

	// Decrypt hex ciphertext with given ivHex. Returns UTF-8 plaintext (empty on error).
	String decrypt(const String &cipherHex, const String &ivHex);

private:
	std::vector<uint8_t> key_; // 16 bytes

	static void hexToBytes(const String &hex, std::vector<uint8_t> &out);
	static String bytesToHex(const uint8_t *buf, size_t len);
	static void pkcs7Pad(std::vector<uint8_t> &data, size_t blockSize = 16);
	static bool pkcs7Unpad(std::vector<uint8_t> &data, size_t blockSize = 16);
};

