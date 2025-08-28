#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// Definicje pinów wyświetlacza
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RESET 4

// Inicjalizacja wyświetlacza
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicjalizacja wyświetlacza TFT...");
  
  // Inicjalizacja wyświetlacza
  tft.begin();
  tft.setRotation(1); // Orientacja pozioma (landscape)
  
  // Wyświetlenie tekstu
  displayHelloESP32();
  
  Serial.println("Wyświetlacz gotowy!");
}

void loop() {
  // Pusta pętla - tekst wyświetlany raz w setup()
  delay(1000);
}

void displayHelloESP32() {
  // Czarne tło
  tft.fillScreen(ILI9341_BLACK);
  
  // Zielony tekst
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(4); // Duży rozmiar tekstu
  
  // Obliczenie pozycji dla centrowania tekstu
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("HELLO ESP32", 0, 0, &x1, &y1, &w, &h);
  
  // Centrowanie na ekranie
  int x = (tft.width() - w) / 2;
  int y = (tft.height() - h) / 2;
  
  // Wyświetlenie tekstu
  tft.setCursor(x, y);
  tft.println("HELLO ESP32");
  
  Serial.println("Tekst 'HELLO ESP32' wyświetlony na ekranie");
  Serial.print("Pozycja: x=");
  Serial.print(x);
  Serial.print(", y=");
  Serial.println(y);
  Serial.print("Rozmiar ekranu: ");
  Serial.print(tft.width());
  Serial.print("x");
  Serial.println(tft.height());
}
