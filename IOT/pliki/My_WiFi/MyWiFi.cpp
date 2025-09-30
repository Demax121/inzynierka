
#include "MyWiFiv2.h"
#include <Arduino.h>

const char* WIFI_SSID     = "Praca_dyplomowa";
const char* WIFI_PASSWORD = "AJP@INZYNIERKA2137";

// Dodano certyfikat serwera reverse proxy
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIBojCCAUmgAwIBAgIQclrH74b1qBkrxEUrWixB5DAKBggqhkjOPQQDAjAwMS4w\n" \
"LAYDVQQDEyVDYWRkeSBMb2NhbCBBdXRob3JpdHkgLSAyMDI1IEVDQyBSb290MB4X\n" \
"DTI1MDkzMDE0NDg1MFoXDTM1MDgwOTE0NDg1MFowMDEuMCwGA1UEAxMlQ2FkZHkg\n" \
"TG9jYWwgQXV0aG9yaXR5IC0gMjAyNSBFQ0MgUm9vdDBZMBMGByqGSM49AgEGCCqG\n" \
"SM49AwEHA0IABD2cfk8bBovTr8Q7A3RcpUYlS2kjF10WxxCe56sc4SWuEc3p7YNK\n" \
"IJKQk1h1mQejDUQXWtbKtwmcbJFIkh9pgKyjRTBDMA4GA1UdDwEB/wQEAwIBBjAS\n" \
"BgNVHRMBAf8ECDAGAQH/AgEBMB0GA1UdDgQWBBQ5yfJ1/K9cPqqDLIu/i6aBWWKK\n" \
"cjAKBggqhkjOPQQDAgNHADBEAiAg6sL1SlgilD//v51dVjpg/CZoejG9l1mszjZa\n" \
"wx7WpAIgbWvCgs0MX2cc2uJJ0DcKg6qiJ0ZHjcKdqW43z6vNtc0=\n" \
"-----END CERTIFICATE-----\n";

void MyWiFiv2::connect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++retry > 60) {
      return;
    }
  }
}

const char* MyWiFiv2::getCert() {
  return root_ca;
}