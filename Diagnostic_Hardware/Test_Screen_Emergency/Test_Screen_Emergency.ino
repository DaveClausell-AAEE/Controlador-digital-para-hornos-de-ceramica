/*
 * Test_Screen_Emergency.ino
 * Forza el encendido de la pantalla y el backlight para descartar fallos de hardware.
 */

#include <TFT_eSPI.h>

#define TFT_LED 12 // Pin del Backlight

TFT_eSPI tft = TFT_eSPI();

void setup() {
  // 1. FORZAR BACKLIGHT (Muy importante para ver algo)
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); // Encendido total
  
  // 2. Inicializar Pantalla
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_RED); // Fondo rojo para que sea muy visible
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 100);
  tft.print("PANTALLA OK");
  
  tft.setTextSize(2);
  tft.setCursor(20, 150);
  tft.print("Si ves esto, el");
  tft.setCursor(20, 180);
  tft.print("hardware esta bien.");
}

void loop() {
  // Parpadeo del fondo para confirmar que el código corre
  delay(1000);
  tft.fillScreen(TFT_BLUE);
  tft.setCursor(20, 100);
  tft.print("PANTALLA OK");
  delay(1000);
  tft.fillScreen(TFT_RED);
  tft.setCursor(20, 100);
  tft.print("PANTALLA OK");
}
