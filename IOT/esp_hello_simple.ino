#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// Piny dla ESP32
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22

//PODŁĄCZENIE WYŚWIETLACZ - ESP32D
//VCC i LED/BL - 3v3
//GND - GND
//CS - D5
//RESET - D22
//DC - D21
//MOSI(SDI) - D23
//SCK - D18
//MISO (SDO) - niepodłączone bo to komunikacja z wyświetlacza do ESP


// Tworzymy obiekt ILI9341
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(19200);
  delay(500);

  tft.begin();                 // Inicjalizacja ILI9341
  tft.setRotation(1);          // Obrót ekranu (0-3)
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.println("AKTUALNA TEMPERATURA: ");
  tft.println("OCZEKIWANA TEMPERATURA: ");
  tft.println("STATUS: ");
  tft.println("FUNKCJA: ");
}

void loop() {
}
