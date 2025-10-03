#include "AESCrypto.h"

// Constructor
AESCrypto::AESCrypto(const String& key) : encryption_key(key) {}

// Generate random IV bytes using ESP32's hardware RNG (private method)
void AESCrypto::generateIVBytes(uint8_t* iv, size_t length) {
  for (size_t i = 0; i < length; i++) {
    iv[i] = esp_random() & 0xFF;
  }
}

// Static method to generate IV as hex string (public method)
String AESCrypto::generateIV() {
  uint8_t iv[16];
  for (size_t i = 0; i < 16; i++) {
    iv[i] = esp_random() & 0xFF;
  }
  
  // Convert to hex string
  String result = "";
  for (size_t i = 0; i < 16; i++) {
    if (iv[i] < 16) result += "0";
    result += String(iv[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// Convert hex string to bytes
void AESCrypto::hexStringToBytes(const String& hex, uint8_t* bytes, size_t length) {
  for (size_t i = 0; i < length; i++) {
    String byteString = hex.substring(i * 2, i * 2 + 2);
    bytes[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
  }
}

// Convert bytes to hex string
String AESCrypto::bytesToHexString(const uint8_t* bytes, size_t length) {
  String result = "";
  for (size_t i = 0; i < length; i++) {
    if (bytes[i] < 16) result += "0";
    result += String(bytes[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// PKCS7 padding
size_t AESCrypto::addPKCS7Padding(uint8_t* data, size_t dataLength, size_t blockSize) {
  size_t paddingLength = blockSize - (dataLength % blockSize);
  for (size_t i = 0; i < paddingLength; i++) {
    data[dataLength + i] = paddingLength;
  }
  return dataLength + paddingLength;
}

// Remove PKCS7 padding
size_t AESCrypto::removePKCS7Padding(uint8_t* data, size_t dataLength) {
  if (dataLength == 0) return 0;
  uint8_t paddingLength = data[dataLength - 1];
  if (paddingLength > dataLength || paddingLength == 0) return dataLength;
  return dataLength - paddingLength;
}

// Encrypt function using AES-128-CBC
String AESCrypto::encrypt(const String& plaintext) {
  if (plaintext.length() == 0) return "";
  
  // Initialize AES context
  mbedtls_aes_init(&aes_ctx);
  
  // Convert encryption key from hex string to bytes
  uint8_t key[16];
  hexStringToBytes(encryption_key, key, 16);
  
  // Set encryption key
  if (mbedtls_aes_setkey_enc(&aes_ctx, key, 128) != 0) {
    mbedtls_aes_free(&aes_ctx);
    return "";
  }
  
  // Generate random IV
  uint8_t iv[16];
  generateIVBytes(iv, 16);
  
  // Prepare data for encryption
  size_t plaintextLength = plaintext.length();
  size_t paddedLength = ((plaintextLength / 16) + 1) * 16; // Round up to next 16-byte boundary
  uint8_t* paddedData = new uint8_t[paddedLength];
  
  // Copy plaintext to padded buffer
  memcpy(paddedData, plaintext.c_str(), plaintextLength);
  
  // Add PKCS7 padding
  size_t finalLength = addPKCS7Padding(paddedData, plaintextLength, 16);
  
  // Encrypt data
  uint8_t* encryptedData = new uint8_t[finalLength];
  uint8_t ivCopy[16];
  memcpy(ivCopy, iv, 16); // mbedtls modifies IV during encryption
  
  if (mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, finalLength, ivCopy, paddedData, encryptedData) != 0) {
    delete[] paddedData;
    delete[] encryptedData;
    mbedtls_aes_free(&aes_ctx);
    return "";
  }
  
  // Combine IV + encrypted data and convert to hex
  uint8_t* combined = new uint8_t[16 + finalLength];
  memcpy(combined, iv, 16);
  memcpy(combined + 16, encryptedData, finalLength);
  
  String result = bytesToHexString(combined, 16 + finalLength);
  
  // Cleanup
  delete[] paddedData;
  delete[] encryptedData;
  delete[] combined;
  mbedtls_aes_free(&aes_ctx);
  
  return result;
}

// Decrypt function using AES-128-CBC
String AESCrypto::decrypt(const String& ciphertext) {
  if (ciphertext.length() < 32) return ""; // At least IV (32 hex chars)
  
  // Initialize AES context
  mbedtls_aes_init(&aes_ctx);
  
  // Convert encryption key from hex string to bytes
  uint8_t key[16];
  hexStringToBytes(encryption_key, key, 16);
  
  // Set decryption key
  if (mbedtls_aes_setkey_dec(&aes_ctx, key, 128) != 0) {
    mbedtls_aes_free(&aes_ctx);
    return "";
  }
  
  // Convert hex ciphertext to bytes
  size_t totalLength = ciphertext.length() / 2;
  uint8_t* combined = new uint8_t[totalLength];
  hexStringToBytes(ciphertext, combined, totalLength);
  
  // Extract IV and encrypted data
  uint8_t iv[16];
  memcpy(iv, combined, 16);
  
  size_t encryptedLength = totalLength - 16;
  uint8_t* encryptedData = new uint8_t[encryptedLength];
  memcpy(encryptedData, combined + 16, encryptedLength);
  
  // Decrypt data
  uint8_t* decryptedData = new uint8_t[encryptedLength];
  
  if (mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, encryptedLength, iv, encryptedData, decryptedData) != 0) {
    delete[] combined;
    delete[] encryptedData;
    delete[] decryptedData;
    mbedtls_aes_free(&aes_ctx);
    return "";
  }
  
  // Remove padding
  size_t finalLength = removePKCS7Padding(decryptedData, encryptedLength);
  
  // Convert to string
  String result = "";
  for (size_t i = 0; i < finalLength; i++) {
    result += (char)decryptedData[i];
  }
  
  // Cleanup
  delete[] combined;
  delete[] encryptedData;
  delete[] decryptedData;
  mbedtls_aes_free(&aes_ctx);
  
  return result;
}

// Set new encryption key
void AESCrypto::setEncryptionKey(const String& key) {
  encryption_key = key;
}