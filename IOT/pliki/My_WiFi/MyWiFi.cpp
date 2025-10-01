
#include "MyWiFi.h"
#include <Arduino.h>

const char* WIFI_SSID     = "Praca_dyplomowa";
const char* WIFI_PASSWORD = "AJP@INZYNIERKA2137";

// Certyfikat serwera simplySmart (self-signed)
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIID7TCCAtWgAwIBAgIUZAWvj7OYbjDxTsTnSJZ4DKmXTWIwDQYJKoZIhvcNAQEL\n" \
"BQAwgYUxCzAJBgNVBAYTAlBMMREwDwYDVQQIDAhMdWJ1c2tpZTEUMBIGA1UEBwwL\n" \
"R29yem93IFdsa3AxGDAWBgNVBAoMD1ByYWNhIER5cGxvbW93YTEUMBIGA1UECwwL\n" \
"U2ltcGx5U21hcnQxHTAbBgNVBAMMFEFsZWtzYW5kZXIgR3JhYm93c2tpMB4XDTI1\n" \
"MTAwMTA3NTEwOVoXDTI2MTAwMTA3NTEwOVowgYUxCzAJBgNVBAYTAlBMMREwDwYD\n" \
"VQQIDAhMdWJ1c2tpZTEUMBIGA1UEBwwLR29yem93IFdsa3AxGDAWBgNVBAoMD1By\n" \
"YWNhIER5cGxvbW93YTEUMBIGA1UECwwLU2ltcGx5U21hcnQxHTAbBgNVBAMMFEFs\n" \
"ZWtzYW5kZXIgR3JhYm93c2tpMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n" \
"AQEAosOk71NhWsu+Bt7GGUgMPp1Vx5buCrqWsc+9jiaN6qqDqd2wLAHn5rOEAU8y\n" \
"qPXWwo/JgYqRdwi6EHK4aCv4jp1aFcRatW8aQNzV3UykT0do3430Jn0M56pM6RGc\n" \
"MUMbyZOZhqF/spEyGfy1/KAHZ9F6nltxzF5OM9U85WiyM57H8T7F8YnbTKsMmcQW\n" \
"4VnBBEq6cMWLAhPch49D8La0XHUcCsasBFYZyLEleMjqz418QunDfv3dlevGGOms\n" \
"orfJLlAwmBjJuxlAy1qUhPDHXazsuoBiCJdDq2fQW2ZYBSgOEoIwDttJfrr18krb\n" \
"VwbbistUd3e5ivAhUUDUIRhyawIDAQABo1MwUTAdBgNVHQ4EFgQUIX82mHhJ3qJJ\n" \
"5HDN5Lrf2I3qWJEwHwYDVR0jBBgwFoAUIX82mHhJ3qJJ5HDN5Lrf2I3qWJEwDwYD\n" \
"VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAmNUMTBZzJLScvquI5017\n" \
"Pd7yUlhOLYquMcTp/uQv0Ml9X999SizWtcx/RItXw/39dU78F3zb34RP0rtnw8F8\n" \
"Oge+N+p7o06ckulyB7FsS8NoUl7D5qA6VFVs12/OS7+r2zUSdxPbxLnpt0Mn5g5X\n" \
"dchVInjwuTVOcxK8y7qMMdF4rsadUUiuF7S+fImw+33uGGUrHoGYPiLm08+Eluxt\n" \
"CP0ld58xAMZrabx1y0QezVWAQSXmTxrndXEyQRJoIWsrkReQhMhUBMnE3d5B9ArX\n" \
"g5BmBsXlAZgdKGMNfuEPMY05GAFnbusWli8C0epvKhZPzujeC8zki1qMMAgaqt2W\n" \
"NA==\n" \
"-----END CERTIFICATE-----\n";

void MyWiFi::connect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++retry > 60) {
      return;
    }
  }
}

const char* MyWiFi::getCert() {
  return root_ca;
}