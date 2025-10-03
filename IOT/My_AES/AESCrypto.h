// Minimal AES-128-CBC helper for ESP32 using mbedtls
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

