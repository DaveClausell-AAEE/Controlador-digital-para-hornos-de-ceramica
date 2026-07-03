/*
 * Diagnostic_V13_Full.ino
 * Herramienta de validación completa para el Controlador de Horno V13.0.
 * 
 * PRUEBAS:
 * 1. Pantalla TFT (ILI9341) y Backlight (PWM).
 * 2. Tarjeta SD (Montaje en VSPI).
 * 3. Botones Industriales (Pines 34, 35, 36, 39).
 * 4. Relay (GPIO 17 - Active LOW).
 * 5. Buzzer (GPIO 27).
 * 6. LED RGB (GPIO 16, 21, 22).
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>

// --- PINOUT V13.0 ---
#define RELAY_PIN     17
#define BUZZER_PIN    27
#define LED_R_PIN     16
#define LED_G_PIN     21
#define LED_B_PIN     22
#define BTN_UP_PIN    36
#define BTN_DOWN_PIN  39
#define BTN_OK_PIN    34
#define BTN_EXIT_PIN  35
#define SD_CS         33 
#define TFT_LED_PIN   12 

TFT_eSPI tft = TFT_eSPI();
bool relayActive = false;
int currentRGB = 0; // 0:Off, 1:Red, 2:Green, 3:Blue

// --- PROTOTIPOS ---
void dibujarInterfazBase();
void actualizarBotonesUI(bool u, bool d, bool o, bool e);
void drawIndicator(const char* label, bool active, int x, int y);
void actualizarRelayUI();
void testSD();

void setup() {
  Serial.begin(115200);
  
  // 1. Configuración de Salidas
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Apagado por defecto (Active LOW)
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  digitalWrite(LED_R_PIN, LOW);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_B_PIN, LOW);

  // 2. Configuración de Entradas (Requieren Pull-up externo)
  pinMode(BTN_UP_PIN, INPUT);
  pinMode(BTN_DOWN_PIN, INPUT);
  pinMode(BTN_OK_PIN, INPUT);
  pinMode(BTN_EXIT_PIN, INPUT);

  // 3. Inicializar Pantalla
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  // PWM para el Backlight (Compatible con Core 3.x)
  ledcAttach(TFT_LED_PIN, 5000, 8); 
  ledcWrite(TFT_LED_PIN, 255); // Brillo máximo

  dibujarInterfazBase();
}

void loop() {
  static unsigned long lastUpdate = 0;
  
  // Lectura de botones (LOW = Presionado)
  bool up = digitalRead(BTN_UP_PIN) == LOW;
  bool down = digitalRead(BTN_DOWN_PIN) == LOW;
  bool ok = digitalRead(BTN_OK_PIN) == LOW;
  bool exit = digitalRead(BTN_EXIT_PIN) == LOW;

  // Lógica de Pruebas
  
  // OK: Toggle Relay + Beep corto
  static bool lastOk = false;
  if (ok && !lastOk) {
    relayActive = !relayActive;
    digitalWrite(RELAY_PIN, relayActive ? LOW : HIGH);
    tone(BUZZER_PIN, 2000, 100);
    actualizarRelayUI();
  }
  lastOk = ok;

  // UP: Cambiar Color RGB
  static bool lastUp = false;
  if (up && !lastUp) {
    currentRGB = (currentRGB + 1) % 4;
    digitalWrite(LED_R_PIN, currentRGB == 1);
    digitalWrite(LED_G_PIN, currentRGB == 2);
    digitalWrite(LED_B_PIN, currentRGB == 3);
    tone(BUZZER_PIN, 3000, 50);
  }
  lastUp = up;

  // DOWN: Test de Montaje SD
  static bool lastDown = false;
  if (down && !lastDown) {
    testSD();
  }
  lastDown = down;

  // EXIT: Beep largo (Alarma)
  static bool lastExit = false;
  if (exit && !lastExit) {
    tone(BUZZER_PIN, 1000, 500);
  }
  lastExit = exit;

  // Actualizar visualización de botones cada 50ms
  if (millis() - lastUpdate > 50) {
    actualizarBotonesUI(up, down, ok, exit);
    lastUpdate = millis();
  }
}

void dibujarInterfazBase() {
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("DIAGNOSTICO INTEGRAL V13.0", 10, 10, 4);
  tft.drawLine(0, 40, 320, 40, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("BOTONES:", 10, 60, 2);
  tft.drawString("RELAY:", 10, 130, 2);
  tft.drawString("RGB LED:", 10, 160, 2);
  tft.drawString("SD CARD:", 10, 190, 2);

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("OK: Toggle Relay | UP: Ciclo RGB", 10, 220);
  tft.drawString("DN: Test SD | EXIT: Beep Alarma", 10, 230);
}

void actualizarBotonesUI(bool u, bool d, bool o, bool e) {
  drawIndicator("UP", u, 100, 60);
  drawIndicator("DN", d, 150, 60);
  drawIndicator("OK", o, 200, 60);
  drawIndicator("EX", e, 250, 60);
}

void drawIndicator(const char* label, bool active, int x, int y) {
  tft.fillRoundRect(x, y, 40, 25, 4, active ? TFT_GREEN : TFT_DARKGREY);
  tft.setTextColor(active ? TFT_BLACK : TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + 20, y + 13);
  tft.setTextDatum(TL_DATUM);
}

void actualizarRelayUI() {
  tft.fillRect(100, 125, 100, 30, TFT_BLACK);
  if (relayActive) {
    tft.setTextColor(TFT_RED);
    tft.drawString("ACTIVO (ON)", 100, 130, 2);
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.drawString("SEGURO (OFF)", 100, 130, 2);
  }
}

void testSD() {
  tft.fillRect(100, 185, 200, 30, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Probando...", 100, 190, 2);
  
  // Robust SD Init
  digitalWrite(15, HIGH); // TFT_CS High
  SPI.begin(18, 19, 23, SD_CS);
  
  if (SD.begin(SD_CS, SPI, 4000000)) {
    tft.fillRect(100, 185, 200, 30, TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("OK! Montada", 100, 190, 2);
    SD.end();
  } else {
    tft.fillRect(100, 185, 200, 30, TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.drawString("ERROR: No detectada", 100, 190, 2);
  }
}
