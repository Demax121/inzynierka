#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <WiFiManager.h>

// WiFiManager instance
WiFiManager wm;

// Server configuration
const char* serverURL = "http://192.168.1.100:8884"; // Adjust to your server IP
const char* authEndpoint = "/deviceAuth.php";

// Device identification
String deviceUID = "ESP32_001";
String macAddress = "";
String tempKey = "";
String permanentKey = "";

// Security status
bool isAuthenticated = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 Device Authentication System");
  
  // Get MAC address
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  macAddress = String(baseMac[0], HEX) + ":" +
               String(baseMac[1], HEX) + ":" +
               String(baseMac[2], HEX) + ":" +
               String(baseMac[3], HEX) + ":" +
               String(baseMac[4], HEX) + ":" +
               String(baseMac[5], HEX);
  macAddress.toUpperCase();
  
  Serial.println("Device UID: " + deviceUID);
  Serial.println("MAC Address: " + macAddress);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Start authentication process
  if (WiFi.status() == WL_CONNECTED) {
    authenticateDevice();
  }
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    connectToWiFi();
    return;
  }
  
  if (isAuthenticated) {
    // Main application logic here
    sendHeartbeat();
    delay(30000); // Send heartbeat every 30 seconds
  } else {
    // Retry authentication
    Serial.println("Authentication failed, retrying...");
    authenticateDevice();
    delay(10000);
  }
}

void connectToWiFi() {
  Serial.println("Starting WiFiManager...");
  
  // WiFiManager configuration
  wm.setConfigPortalTimeout(180); // 3 minutes timeout
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  
  // Custom parameters for server configuration
  WiFiManagerParameter custom_server_url("server", "Server URL", "http://192.168.1.100:8884", 40);
  wm.addParameter(&custom_server_url);
  
  // Try to connect to saved WiFi or start config portal
  if (!wm.autoConnect("ESP32-DeviceAuth")) {
    Serial.println("Failed to connect to WiFi and hit timeout");
    delay(3000);
    // Reset and try again
    ESP.restart();
    delay(5000);
  }
  
  // If we get here, we are connected to WiFi
  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.println("IP address: " + WiFi.localIP().toString());
  
  // Get custom server URL if provided
  String serverURLParam = custom_server_url.getValue();
  if (serverURLParam.length() > 0) {
    // Update server URL (you might want to store this in EEPROM/Preferences)
    Serial.println("Custom server URL: " + serverURLParam);
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());
  Serial.println("Connect to 'ESP32-DeviceAuth' to configure WiFi");
}

void saveConfigCallback() {
  Serial.println("WiFi configuration saved");
}

void authenticateDevice() {
  Serial.println("Starting authentication process...");
  
  // Step 1: Request temporary key
  if (requestTempKey()) {
    Serial.println("Temporary key received: " + tempKey);
    
    // Step 2: Send device authentication
    if (sendDeviceAuth()) {
      Serial.println("Device authenticated successfully!");
      isAuthenticated = true;
    } else {
      Serial.println("Device authentication failed!");
      isAuthenticated = false;
    }
  } else {
    Serial.println("Failed to get temporary key!");
    isAuthenticated = false;
  }
}

bool requestTempKey() {
  HTTPClient http;
  http.begin(String(serverURL) + String(authEndpoint) + "?action=request_temp_key");
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Temp key response: " + response);
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc["status"] == "success") {
      tempKey = doc["temp_key"].as<String>();
      http.end();
      return true;
    }
  } else {
    Serial.println("HTTP Error: " + String(httpResponseCode));
  }
  
  http.end();
  return false;
}

bool sendDeviceAuth() {
  // Create device data JSON
  DynamicJsonDocument deviceDoc(512);
  deviceDoc["device_uid"] = deviceUID;
  deviceDoc["mac_address"] = macAddress;
  deviceDoc["timestamp"] = millis();
  
  String deviceDataJson;
  serializeJson(deviceDoc, deviceDataJson);
  
  // Hash and encrypt device data (simplified)
  String hashedData = hashData(deviceDataJson);
  String encryptedData = simpleEncrypt(hashedData, tempKey);
  
  // Create authentication request
  DynamicJsonDocument authDoc(1024);
  authDoc["action"] = "authenticate";
  authDoc["temp_key"] = tempKey;
  authDoc["encrypted_data"] = encryptedData;
  
  String authJson;
  serializeJson(authDoc, authJson);
  
  HTTPClient http;
  http.begin(String(serverURL) + String(authEndpoint));
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(authJson);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Auth response: " + response);
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc["status"] == "success") {
      String encryptedResponse = doc["encrypted_response"];
      String decryptedResponse = simpleDecrypt(encryptedResponse, tempKey);
      
      DynamicJsonDocument responseDoc(512);
      deserializeJson(responseDoc, decryptedResponse);
      
      if (responseDoc["status"] == "authenticated") {
        permanentKey = responseDoc["permanent_key"].as<String>();
        
        // Clear temporary key for security
        tempKey = "";
        
        Serial.println("Permanent key acquired!");
        http.end();
        return true;
      }
    }
  } else {
    Serial.println("HTTP Error: " + String(httpResponseCode));
  }
  
  http.end();
  return false;
}

void sendHeartbeat() {
  if (!isAuthenticated || permanentKey.length() == 0) {
    return;
  }
  
  DynamicJsonDocument doc(256);
  doc["action"] = "heartbeat";
  doc["device_uid"] = deviceUID;
  doc["uptime"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  String encryptedData = simpleEncrypt(jsonString, permanentKey);
  
  Serial.println("Sending heartbeat...");
  // Implementation for heartbeat sending would go here
}

// Simple hash function (replace with proper hashing)
String hashData(String data) {
  // This is a very basic hash - replace with SHA256 or similar
  uint32_t hash = 0;
  for (int i = 0; i < data.length(); i++) {
    hash = hash * 31 + data.charAt(i);
  }
  return String(hash, HEX);
}

// Simple encryption (replace with proper encryption like AES)
String simpleEncrypt(String data, String key) {
  String encrypted = "";
  for (int i = 0; i < data.length(); i++) {
    char c = data.charAt(i);
    char k = key.charAt(i % key.length());
    encrypted += char(c ^ k);
  }
  return base64Encode(encrypted);
}

// Simple decryption
String simpleDecrypt(String encryptedData, String key) {
  String decoded = base64Decode(encryptedData);
  String decrypted = "";
  for (int i = 0; i < decoded.length(); i++) {
    char c = decoded.charAt(i);
    char k = key.charAt(i % key.length());
    decrypted += char(c ^ k);
  }
  return decrypted;
}

// Basic Base64 encode/decode (simplified)
String base64Encode(String data) {
  // Simplified base64 encoding - replace with proper implementation
  return data + "_encoded";
}

String base64Decode(String data) {
  // Simplified base64 decoding - replace with proper implementation
  data.replace("_encoded", "");
  return data;
}
