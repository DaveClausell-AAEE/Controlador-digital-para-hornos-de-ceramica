/*
 * Diagnostic_SD_Final_V13.ino
 * Pantalla garantizada + Detección de SD
 */

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define TFT_LED 12
#define SD_CS   33
#define TFT_CS  15

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  
  // 1. Backlight ON inmediato
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  
  // 2. Inicializar Pantalla
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("DIAGNOSTICO SD V13.0");
  tft.drawLine(0, 30, 320, 30, TFT_BLUE);

  // 3. Inicializar SD
  tft.setCursor(10, 50);
  tft.print("Buscando tarjeta...");
  delay(1000);

  // Forzar pines CS
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH); // Desactivar TFT para el bus

  if (!SD.begin(SD_CS)) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 80);
    tft.print("ERROR: No detectada");
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 120);
    tft.print("- Revisa cable MISO (GPIO 19)");
    tft.setCursor(10, 140);
    tft.print("- Revisa cable MOSI (GPIO 23)");
    tft.setCursor(10, 160);
    tft.print("- Revisa cable SCK  (GPIO 18)");
    tft.setCursor(10, 180);
    tft.print("- Formato FAT32 (<32GB)");
    Serial.println("SD Falló");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 80);
    tft.print("SD MONTADA: OK!");
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 120);
    tft.printf("Capacidad: %llu MB", cardSize);
    
    // Prueba de archivo
    File f = SD.open("/dac_lab.txt", FILE_WRITE);
    if(f) {
      f.println("Test V13 OK");
      f.close();
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(10, 150);
      tft.print("Escritura: OK");
    }
    Serial.println("SD Funcionando perfectamente");
  }
}

void loop() {}
