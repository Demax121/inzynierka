// AES-128-GCM implementation (replaces previous CBC version) for ESP32.
// Features:
//  * 12-byte random nonce (96-bit) per encryption
//  * Authenticated encryption producing ciphertext + 16-byte tag
//  * Hex encoding for nonce / ciphertext / tag
//  * No padding needed (stream-like GCM)
//  * Empty string returned on decrypt failure (auth/tag mismatch or malformed input)
//
// Notes:
//  - No Additional Authenticated Data (AAD) used presently; extend if context binding required.
//  - Nonce collisions MUST be avoided for a given key; random 96-bit nonces are statistically safe
//    for low-volume IoT traffic. For very high volume, adopt a monotonic counter-based nonce.
//  - Uses mbedtls hardware acceleration when available on ESP32.
//  - Key is normalized (truncate/pad) to 16 bytes.

#include "AESCrypto.h"
#include <mbedtls/gcm.h>
#include <esp_random.h>

// Convert a single hex character to its 4-bit value (0 on invalid char)
static inline uint8_t nybble(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
	return 0;
}

AESCrypto::AESCrypto(const String &keyUtf8) {
    key_.assign(16, 0);
    for (size_t i = 0; i < 16 && i < (size_t)keyUtf8.length(); ++i) key_[i] = (uint8_t)keyUtf8[i];
}

String AESCrypto::generateNonce() {
    uint8_t n[12];
    for (int i = 0; i < 12; ++i) n[i] = (uint8_t)esp_random();
    return bytesToHex(n, 12);
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

bool AESCrypto::encrypt(const String &plainUtf8, const String &nonceHex, String &cipherHexOut, String &tagHexOut) {
	std::vector<uint8_t> nonce; hexToBytes(nonceHex, nonce);
	if (nonce.size() != 12) return false;

	const uint8_t *keyPtr = key_.data();
	const uint8_t *noncePtr = nonce.data();

	std::vector<uint8_t> plain(plainUtf8.begin(), plainUtf8.end());
	std::vector<uint8_t> cipher(plain.size());
	std::vector<uint8_t> tag(16);

	mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
	if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, keyPtr, 128) != 0) {
		mbedtls_gcm_free(&gcm); return false;
	}
	int rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
									   plain.size(),
									   noncePtr, 12,
									   nullptr, 0,
									   plain.data(), cipher.data(),
									   tag.size(), tag.data());
	mbedtls_gcm_free(&gcm);
	if (rc != 0) return false;

	cipherHexOut = bytesToHex(cipher.data(), cipher.size());
	tagHexOut = bytesToHex(tag.data(), tag.size());
	return true;
}

String AESCrypto::decrypt(const String &cipherHex, const String &nonceHex, const String &tagHex) {
	std::vector<uint8_t> nonce; hexToBytes(nonceHex, nonce);
	std::vector<uint8_t> cipher; hexToBytes(cipherHex, cipher);
	std::vector<uint8_t> tag; hexToBytes(tagHex, tag);
	if (nonce.size() != 12 || tag.size() != 16 || cipher.empty()) return String("");

	std::vector<uint8_t> out(cipher.size());
	mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
	if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key_.data(), 128) != 0) {
		mbedtls_gcm_free(&gcm); return String("");
	}
	int rc = mbedtls_gcm_auth_decrypt(&gcm, cipher.size(),
									  nonce.data(), 12,
									  nullptr, 0,
									  tag.data(), tag.size(),
									  cipher.data(), out.data());
	mbedtls_gcm_free(&gcm);
	if (rc != 0) return String("");
	return String((const char*)out.data());
}

