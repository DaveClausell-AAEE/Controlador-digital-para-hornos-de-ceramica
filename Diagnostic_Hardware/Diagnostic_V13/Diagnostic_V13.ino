/*
 * Diagnostic_V13.ino
 * Herramienta de validación para la arquitectura de Backup y nuevos pines de botones.
 * 
 * OBJETIVOS:
 * 1. Verificar lectura de botones en pines Input-Only (34, 35, 36, 39).
 * 2. Verificar montaje de SD compartiendo VSPI con la pantalla.
 * 3. Verificar que el relé se mantenga en estado seguro (HIGH).
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>

// --- NUEVO PINOUT ---
#define NEW_BTN_UP    36
#define NEW_BTN_DOWN  39
#define NEW_BTN_OK    34
#define NEW_BTN_EXIT  35
#define SD_CS         33 // Pin liberado
#define RELAY_PIN     17
#define TFT_LED_PIN   12 

TFT_eSPI tft = TFT_eSPI();

void drawButtonState(const char* label, bool pressed, int y) {
  tft.setCursor(10, y);
  tft.print(label);
  if (pressed) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(" PRESIONADO ");
  } else {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.print(" LIBERADO    ");
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  
  // Seguridad: Relé Apagado
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Backlight: Encender
  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, HIGH); // Brillo máximo por defecto

  // Configuración de Botones (Requieren Pull-up externo a 3.3V)
  pinMode(NEW_BTN_UP, INPUT);
  pinMode(NEW_BTN_DOWN, INPUT);
  pinMode(NEW_BTN_OK, INPUT);
  pinMode(NEW_BTN_EXIT, INPUT);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("DIAGNOSTICO V13 - SD/BOTONES", 10, 10, 2);

  Serial.println("Iniciando prueba de SD...");
  if (!SD.begin(SD_CS)) {
    tft.setTextColor(TFT_RED);
    tft.drawString("SD: ERROR de Montaje", 10, 40, 2);
    Serial.println("Error: No se pudo montar la SD");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.drawString("SD: OK (Montada)", 10, 40, 2);
    Serial.print("Tipo SD: "); Serial.println(SD.cardType());
  }

  tft.setTextColor(TFT_WHITE);
  tft.drawString("Pruebe los botones ahora:", 10, 70, 2);
}

void loop() {
  bool u = digitalRead(NEW_BTN_UP) == LOW;
  bool d = digitalRead(NEW_BTN_DOWN) == LOW;
  bool o = digitalRead(NEW_BTN_OK) == LOW;
  bool e = digitalRead(NEW_BTN_EXIT) == LOW;

  // Visualización en Pantalla
  drawButtonState("UP (SVP):", u, 100);
  drawButtonState("DN (SVN):", d, 120);
  drawButtonState("OK (34):", o, 140);
  drawButtonState("EX (35):", e, 160);

  delay(50);
}
