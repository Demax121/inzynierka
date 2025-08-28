/*
  Modified RFID Reader - Simplified to only read and display UID
  Based on original code by Rui Santos & Sara Santos - Random Nerd Tutorials
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>

// Pin configuration for MFRC522
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened
  
  mfrc522.PCD_Init();    // Init MFRC522 board
  Serial.println(F("RFID Reader Ready - Place card near reader to see UID"));
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Display only the UID
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Halt PICC
  mfrc522.PICC_HaltA();

  delay(1000); // Wait 1 second before next read
}
