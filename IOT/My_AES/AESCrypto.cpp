// Implementation of minimal AES-128-CBC helper tailored for ESP32 sketches.
// Provides:
//  * PKCS#7 padding
//  * Hex encoding/decoding helpers (for IV and ciphertext transport)
//  * Random IV generation using esp_random()
//  * Simple String-based interface (adequate for small payloads typical in IoT telemetry)
//
// Security notes:
//  - AES key is provided directly as UTF-8 string (truncated / padded to 16 bytes).
//  - Random IV per message ensures semantic security (no IV reuse).
//  - No authentication (MAC) included. For stronger guarantees against tampering,
//    consider adding HMAC-SHA256 over (IV || ciphertext) or migrating to an AEAD mode
//    (e.g., AES-GCM). Current usage assumes trusted LAN + server side validation.
//  - hexToBytes() silently ignores malformed odd-length hex (returns empty result).
//
// Performance considerations:
//  - Uses mbedtls AES implementation (hardware acceleration on ESP32 where available).
//  - Temporary std::vector allocations are acceptable for small payloads; could be
//    optimized with static buffers if needed.

#include "AESCrypto.h"
#include <mbedtls/aes.h>
#include <esp_random.h>

// Convert a single hex character to its 4-bit value (0 on invalid char)
static inline uint8_t nybble(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
	return 0;
}

AESCrypto::AESCrypto(const String &keyUtf8) {
	// Normalize provided key to exactly 16 bytes (AES-128)
	// Longer input => truncated; shorter => right-padded with zeros.
	key_.assign(16, 0);
	for (size_t i = 0; i < 16 && i < (size_t)keyUtf8.length(); ++i) key_[i] = (uint8_t)keyUtf8[i];
}

String AESCrypto::generateIV() {
	uint8_t iv[16];
	for (int i = 0; i < 16; ++i) iv[i] = (uint8_t)esp_random();
	return bytesToHex(iv, 16);
}

void AESCrypto::hexToBytes(const String &hex, std::vector<uint8_t> &out) {
	out.clear();
	size_t n = hex.length();
	if (n % 2 != 0) return; // invalid length -> leave 'out' empty
	out.reserve(n / 2);
	for (size_t i = 0; i < n; i += 2) {
		uint8_t b = (nybble(hex[i]) << 4) | nybble(hex[i + 1]);
		out.push_back(b);
	}
}

String AESCrypto::bytesToHex(const uint8_t *buf, size_t len) {
	static const char *hex = "0123456789abcdef";
	String s; s.reserve(len * 2); // Preallocate for efficiency
	for (size_t i = 0; i < len; ++i) {
		s += hex[(buf[i] >> 4) & 0xF];
		s += hex[buf[i] & 0xF];
	}
	return s;
}

void AESCrypto::pkcs7Pad(std::vector<uint8_t> &data, size_t blockSize) {
	size_t padLen = blockSize - (data.size() % blockSize);
	if (padLen == 0) padLen = blockSize;
	data.insert(data.end(), padLen, (uint8_t)padLen);
}

bool AESCrypto::pkcs7Unpad(std::vector<uint8_t> &data, size_t blockSize) {
	if (data.empty() || data.size() % blockSize != 0) return false;
	uint8_t padLen = data.back();
	if (padLen == 0 || padLen > blockSize) return false;
	if (padLen > data.size()) return false;
	for (size_t i = 0; i < padLen; ++i) {
		if (data[data.size() - 1 - i] != padLen) return false;
	}
	data.resize(data.size() - padLen);
	return true;
}

String AESCrypto::encrypt(const String &plainUtf8, const String &ivHex) {
	std::vector<uint8_t> iv; hexToBytes(ivHex, iv);
	if (iv.size() != 16) return String("");

	std::vector<uint8_t> data(plainUtf8.begin(), plainUtf8.end());
	pkcs7Pad(data, 16);

	mbedtls_aes_context ctx; mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_enc(&ctx, key_.data(), 128);
	std::vector<uint8_t> out(data.size(), 0);
	uint8_t ivBuf[16]; memcpy(ivBuf, iv.data(), 16);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, data.size(), ivBuf, data.data(), out.data());
	mbedtls_aes_free(&ctx);
	return bytesToHex(out.data(), out.size());
}

String AESCrypto::decrypt(const String &cipherHex, const String &ivHex) {
	std::vector<uint8_t> iv; hexToBytes(ivHex, iv);
	std::vector<uint8_t> cipher; hexToBytes(cipherHex, cipher);
	if (iv.size() != 16 || cipher.empty() || cipher.size() % 16 != 0) return String("");

	mbedtls_aes_context ctx; mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_dec(&ctx, key_.data(), 128);
	std::vector<uint8_t> out(cipher.size(), 0);
	uint8_t ivBuf[16]; memcpy(ivBuf, iv.data(), 16);
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, cipher.size(), ivBuf, cipher.data(), out.data());
	mbedtls_aes_free(&ctx);
	if (!pkcs7Unpad(out, 16)) return String("");
	return String((const char*)out.data());
}

