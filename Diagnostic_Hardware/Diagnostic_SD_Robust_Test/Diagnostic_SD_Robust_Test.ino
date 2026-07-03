/*
 * Diagnostic_SD_Robust_V13.ino
 * Versión de alta compatibilidad para detectar problemas de bus SPI.
 */

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// --- PINOUT ---
#define SD_CS         33 
#define TFT_CS        15
#define RELAY_PIN     17
#define TFT_LED_PIN   12

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- DIAGNÓSTICO ROBUSTO SD ---");

  // 1. Forzar Relé a apagado (Seguridad)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Backlight: Encender
  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, HIGH); 

  // 2. Gestionar pines CS para evitar conflictos en el bus compartido
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH); // Desactivar Pantalla
  digitalWrite(SD_CS, HIGH);  // Desactivar SD

  // 3. Iniciar Pantalla
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("DEBUG SD V13", 10, 10, 2);

  // 4. Intento de inicio de SD con parámetros específicos
  Serial.println("Probando SD a 4MHz...");
  tft.drawString("Iniciando SD...", 10, 40, 2);
  
  // Esperar un poco para que el voltaje se estabilice
  delay(200);

  // Probamos inicializar el bus SPI manualmente antes de la SD
  // VSPI: SCK=18, MISO=19, MOSI=23
  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS, SPI, 4000000)) { // 4MHz es más estable para cables soldados
    Serial.println("Fallo inicial a 4MHz. Reintentando a 1MHz...");
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("Reintentando 1MHz...", 10, 70, 2);
    
    delay(500);
    
    if (!SD.begin(SD_CS, SPI, 1000000)) {
      Serial.println("ERROR CRÍTICO: No se detecta la SD.");
      tft.setTextColor(TFT_RED);
      tft.fillScreen(TFT_BLACK);
      tft.drawString("ERROR: SD NO DETECTADA", 10, 100, 2);
      tft.drawString("1. Revise VCC (5V recomendado)", 10, 130);
      tft.drawString("2. Revise cables MISO/MOSI", 10, 150);
      tft.drawString("3. Formato FAT32 (<32GB)", 10, 170);
    } else {
      showSDSuccess();
    }
  } else {
    showSDSuccess();
  }
}

void showSDSuccess() {
  Serial.println("¡SD DETECTADA CON ÉXITO!");
  tft.setTextColor(TFT_GREEN);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("SD DETECTADA: OK", 10, 40, 4);
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("Tamaño: %llu MB\n", cardSize);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 80);
  tft.printf("Size: %llu MB", cardSize);
  
  // Prueba de escritura
  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Prueba de escritura DAC LAB OK");
    testFile.close();
    tft.setTextColor(TFT_CYAN);
    tft.drawString("Escritura: EXITOSA", 10, 110, 2);
    Serial.println("Archivo de prueba escrito correctamente.");
  } else {
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("Escritura: FALLIDA", 10, 110, 2);
    Serial.println("Error al abrir archivo para escribir.");
  }
}

void loop() {
  // Nada aquí
}
