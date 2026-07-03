/*
 * Diagnostic_SD_BruteForce.ino
 * Prueba todas las combinaciones posibles para activar la SD.
 */

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define TFT_LED 12
#define SD_CS   33

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
  tft.print("SD BRUTE FORCE TEST");

  // --- INTENTO 1: ESTÁNDAR ---
  tft.setCursor(10, 40);
  tft.print("1. Standard (18,19,23)...");
  SPI.begin(18, 19, 23, SD_CS);
  if (SD.begin(SD_CS, SPI, 4000000)) {
    success("ESTANDAR OK!");
    return;
  }
  tft.setTextColor(TFT_RED); tft.print(" FAIL"); tft.setTextColor(TFT_WHITE);
  SD.end();
  SPI.end();

  // --- INTENTO 2: SWAP MISO/MOSI ---
  tft.setCursor(10, 70);
  tft.print("2. Swap MISO/MOSI...");
  // Probamos invirtiendo 19 y 23
  SPI.begin(18, 23, 19, SD_CS); 
  if (SD.begin(SD_CS, SPI, 4000000)) {
    success("SWAP MISO/MOSI OK!");
    return;
  }
  tft.setTextColor(TFT_RED); tft.print(" FAIL"); tft.setTextColor(TFT_WHITE);
  SD.end();
  SPI.end();

  // --- INTENTO 3: VELOCIDAD ULTRA-LENTA (400KHz) ---
  tft.setCursor(10, 100);
  tft.print("3. Ultra-Slow (400KHz)...");
  SPI.begin(18, 19, 23, SD_CS);
  if (SD.begin(SD_CS, SPI, 4000000)) { // SD.begin maneja la velocidad internamente
    success("SLOW OK!");
    return;
  }
  tft.setTextColor(TFT_RED); tft.print(" FAIL"); tft.setTextColor(TFT_WHITE);

  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 150);
  tft.print("TODO FALLO.");
  tft.setCursor(10, 180);
  tft.print("Pruebe OTRA tarjeta SD.");
}

void success(const char* msg) {
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 130);
  tft.print(msg);
  Serial.println(msg);
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 160);
  tft.printf("Tamano: %llu MB", cardSize);
}

void loop() {}
