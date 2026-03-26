// =================================================================
// ==      TOOL: HERRAMIENTA DE DIAGNOSTICO DE HARDWARE V1.1      ==
// =================================================================
//      (Pruebas de TFT, Sensores, Botones, Relé y Buzzer)
//

#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Adafruit_MAX31855.h>

// --- PINOUT (Igual que en el proyecto V10.8+) ---
#define RELAY_PIN 17
#define BUZZER_PIN 27
#define BTN_UP_PIN    26
#define BTN_DOWN_PIN  25
#define BTN_OK_PIN    33
#define BTN_EXIT_PIN  32
#define TFT_LED       12 
#define MAXCS         5

TFT_eSPI tft = TFT_eSPI();
Adafruit_MAX31855 thermocouple(MAXCS);

bool relayStatus = false;
unsigned long lastSerialPrint = 0;

void suenaBuzzer(int duracion, int veces = 1) {
  for(int i=0; i<veces; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duracion);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < veces - 1) delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- DIAGNOSTICO DE HARDWARE ESP32 V1.1 ---");

  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_EXIT_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // APAGADO (Active Low)
  digitalWrite(BUZZER_PIN, LOW);
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); // Encender backlight

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  if (!thermocouple.begin()) {
    Serial.println("FALLO: Sensor MAX31855 no detectado.");
  } else {
    Serial.println("OK: Sensor MAX31855 detectado.");
  }
  
  // Sonido de inicio (igual que maestro)
  suenaBuzzer(100, 2);
}

void loop() {
  // 1. Leer Botones
  bool up = digitalRead(BTN_UP_PIN) == LOW;
  bool down = digitalRead(BTN_DOWN_PIN) == LOW;
  bool ok = digitalRead(BTN_OK_PIN) == LOW;
  bool exit = digitalRead(BTN_EXIT_PIN) == LOW;

  // 2. Leer Sensor
  double temp = thermocouple.readCelsius();
  double internal = thermocouple.readInternal();

  // 3. Control de Relé manual (OK para toggle)
  static bool prevOk = false;
  if (ok && !prevOk) {
    relayStatus = !relayStatus;
    digitalWrite(RELAY_PIN, relayStatus ? LOW : HIGH);
    Serial.print("RELE: "); Serial.println(relayStatus ? "ENCENDIDO" : "APAGADO");
  }
  prevOk = ok;

  // 4. Pruebas de Buzzer (UP y DOWN)
  static bool prevUp = false;
  if (up && !prevUp) {
    Serial.println("BUZZER: Sonido FIN (1000ms)");
    suenaBuzzer(1000, 1);
  }
  prevUp = up;

  static bool prevDown = false;
  if (down && !prevDown) {
    Serial.println("BUZZER: Sonido FALLO (500ms x 3)");
    suenaBuzzer(500, 3);
  }
  prevDown = down;

  // 5. Salida por Serial (cada 1s)
  if (millis() - lastSerialPrint > 1000) {
    lastSerialPrint = millis();
    Serial.printf("{\"temp\":%.2f, \"int\":%.2f, \"up\":%d, \"down\":%d, \"ok\":%d, \"exit\":%d, \"relay\":%d}\n", 
                  temp, internal, up, down, ok, exit, relayStatus);
  }

  // 6. Dibujar UI de Diagnóstico
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10); tft.print("DIAGNOSTICO DE HARDWARE V1.1");
  tft.drawLine(0, 35, 320, 35, TFT_DARKGREY);

  // Botones (Visual)
  tft.setCursor(10, 50); tft.print("BOTONES:");
  drawButtonIndicator(110, 60, "UP", up);
  drawButtonIndicator(160, 60, "DN", down);
  drawButtonIndicator(210, 60, "OK", ok);
  drawButtonIndicator(260, 60, "EX", exit);

  // Sensor
  tft.setCursor(10, 100); tft.print("SENSOR:");
  if (isnan(temp)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(110, 100); tft.print("ERROR / NAN");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(110, 100); tft.print(temp, 1); tft.print(" C");
    tft.setTextSize(1);
    tft.setCursor(110, 125); tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Int: "); tft.print(internal, 1); tft.print(" C");
    tft.setTextSize(2);
  }

  // Relé
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 150); tft.print("RELE:");
  tft.fillRect(110, 145, 80, 30, relayStatus ? TFT_RED : TFT_DARKGREY);
  tft.setTextColor(relayStatus ? TFT_WHITE : TFT_LIGHTGREY, relayStatus ? TFT_RED : TFT_DARKGREY);
  tft.setCursor(120, 152); tft.print(relayStatus ? "ON" : "OFF");

  // Buzzer
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 185); tft.print("BUZZER:");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(110, 185); tft.setTextSize(1);
  tft.print("UP: Fin | DN: Fallo");
  tft.setTextSize(2);

  // Barra de instrucciones
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 215); tft.print("OK: Alternar Rele. EXIT: Sin funcion.");
  tft.setCursor(10, 230); tft.print("SOPLO: Caliente la termocupla para ver variacion.");

  delay(50);
}

void drawButtonIndicator(int x, int y, const char* lbl, bool pressed) {
  tft.drawCircle(x, y, 20, pressed ? TFT_GREEN : TFT_WHITE);
  if (pressed) tft.fillCircle(x, y, 18, TFT_GREEN);
  else tft.fillCircle(x, y, 18, TFT_BLACK);
  
  tft.setTextColor(pressed ? TFT_BLACK : TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(x - 6, y - 4);
  tft.print(lbl);
}
