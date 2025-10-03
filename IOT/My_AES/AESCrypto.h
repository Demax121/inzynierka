#ifndef AES_CRYPTO_H
#define AES_CRYPTO_H

#include <Arduino.h>
#include <mbedtls/aes.h>
#include <esp_random.h>

class AESCrypto {
private:
  String encryption_key;
  mbedtls_aes_context aes_ctx;
  
  // Helper functions
  void generateIVBytes(uint8_t* iv, size_t length);
  void hexStringToBytes(const String& hex, uint8_t* bytes, size_t length);
  String bytesToHexString(const uint8_t* bytes, size_t length);
  size_t addPKCS7Padding(uint8_t* data, size_t dataLength, size_t blockSize);
  size_t removePKCS7Padding(uint8_t* data, size_t dataLength);

public:
  // Constructor
  AESCrypto(const String& key);
  
  // Public methods
  String encrypt(const String& plaintext);
  String decrypt(const String& ciphertext);
  void setEncryptionKey(const String& key);
  
  // Static method to generate IV as hex string
  static String generateIV();
};

#endif