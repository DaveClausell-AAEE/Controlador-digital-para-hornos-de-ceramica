/*
 * Pin_Continuity_Test_V13.ino
 * Fuerza todos los pines de la SD a 3.3V para probar con multimetro.
 */

#define SD_CS   33
#define SD_MOSI 23
#define SD_SCK  18
#define SD_MISO 19
#define TFT_LED 12

void setup() {
  Serial.begin(115200);
  
  // Encendemos el backlight para saber que el ESP32 esta vivo
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  // Configuramos todos los pines de la SD como salida para la prueba
  pinMode(SD_CS, OUTPUT);
  pinMode(SD_MOSI, OUTPUT);
  pinMode(SD_SCK, OUTPUT);
  pinMode(SD_MISO, OUTPUT);

  Serial.println("--- PRUEBA DE CONTINUIDAD SPI ---");
  Serial.println("Poniendo todos los pines en ALTO (3.3V)...");
}

void loop() {
  // Forzamos 3.3V en todos los pines
  digitalWrite(SD_CS, HIGH);
  digitalWrite(SD_MOSI, HIGH);
  digitalWrite(SD_SCK, HIGH);
  digitalWrite(SD_MISO, HIGH);
  
  delay(1000);
  Serial.println("Pines en ALTO. Mida en el lector SD.");
}
