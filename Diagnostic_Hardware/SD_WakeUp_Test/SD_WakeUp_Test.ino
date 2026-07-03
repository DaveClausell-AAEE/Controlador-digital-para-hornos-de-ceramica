/*
 * SD_WakeUp_Test.ino
 * Envía pulsos de sincronización manuales para despertar tarjetas viejas.
 */

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define SD_CS   33
#define SD_SCK  18
#define SD_MOSI 23
#define SD_MISO 19
#define TFT_LED 12

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.print("SD WAKE-UP PROTOCOL");

  // 1. PROTOCOLO DE DESPERTAR MANUAL
  tft.setCursor(10, 50);
  tft.print("Enviando pulsos de reset...");
  
  pinMode(SD_CS, OUTPUT);
  pinMode(SD_SCK, OUTPUT);
  pinMode(SD_MOSI, OUTPUT);
  digitalWrite(SD_CS, HIGH); // CS en ALTO es vital para el reset
  digitalWrite(SD_MOSI, HIGH);

  // Enviamos 80 pulsos de reloj para que la SD entre en modo SPI
  for(int i=0; i<80; i++) {
    digitalWrite(SD_SCK, HIGH);
    delayMicroseconds(100);
    digitalWrite(SD_SCK, LOW);
    delayMicroseconds(100);
  }
  
  delay(100);
  tft.print(" OK");

  // 2. INTENTO DE MONTAJE
  tft.setCursor(10, 80);
  tft.print("Intentando montar...");
  
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (SD.begin(SD_CS, SPI, 400000)) { // Velocidad muy baja inicial
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 110);
    tft.print("¡LOGRADO! SD ACTIVA");
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 140);
    tft.printf("Size: %llu MB", cardSize);
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 110);
    tft.print("FALLO TOTAL.");
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 140);
    tft.print("Cambie de tarjeta SD.");
  }
}

void loop() {}
