
// =================================================================
// ==           PRUEBAS DE LÓGICA DE CONTROL - V1.0              ==
// =================================================================

#include <Arduino.h>

// Mock de estructuras para testing
struct Etapa { int rampa, temperatura, tiempo; };
#define GRAPH_POINTS 120
float tempHistory[GRAPH_POINTS];
int graphIdx = 0;
float currentTemperature = 25.0;
float temperaturaCrudaSimulada = 27.2;
float calibracionOffset = 1.5;
unsigned long ultimoUpdateTempSimulada = 0;
#define RELAY_PIN_MOCK 17

// --- Funciones a probar (copiadas de la lógica principal) ---

void leerSensores_Mock(bool relayOn) {
  // Simulamos el comportamiento de leerSensores() cuando no hay termocupla (NaN)
  // En el original usa digitalRead(RELAY_PIN)
  if (relayOn) temperaturaCrudaSimulada += 1.0;
  else if (temperaturaCrudaSimulada > 27.2) temperaturaCrudaSimulada -= 0.2;
  
  currentTemperature = temperaturaCrudaSimulada - calibracionOffset;
}

float getMaxTempHistory() {
  float maxT = 100.0;
  for(int i=0; i<GRAPH_POINTS; i++) if(tempHistory[i] > maxT) maxT = tempHistory[i];
  return maxT * 1.1;
}

// --- Framework de Pruebas Simple ---

int testsRun = 0;
int testsPassed = 0;

void assert_float(const char* name, float actual, float expected, float delta = 0.1) {
  testsRun++;
  if (abs(actual - expected) <= delta) {
    Serial.print("[PASS] "); Serial.println(name);
    testsPassed++;
  } else {
    Serial.print("[FAIL] "); Serial.print(name);
    Serial.print(" - Esperado: "); Serial.print(expected);
    Serial.print(", Obtenido: "); Serial.println(actual);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- INICIANDO PRUEBAS DE LOGICA ---");

  // Prueba 1: Simulación de incremento de temperatura
  temperaturaCrudaSimulada = 100.0;
  leerSensores_Mock(true); // Relay ON
  assert_float("Incremento Temp Sim", temperaturaCrudaSimulada, 101.0);
  assert_float("Offset Aplicado", currentTemperature, 99.5);

  // Prueba 2: Simulación de enfriamiento
  leerSensores_Mock(false); // Relay OFF
  assert_float("Decremento Temp Sim", temperaturaCrudaSimulada, 100.8);

  // Prueba 3: Escalado dinámico de gráfica
  for(int i=0; i<GRAPH_POINTS; i++) tempHistory[i] = 0;
  tempHistory[10] = 500.0;
  tempHistory[50] = 1000.0;
  float maxScale = getMaxTempHistory();
  assert_float("Escalado Maximo (1000 * 1.1)", maxScale, 1100.0);

  // Prueba 4: Escalado mínimo de gráfica
  for(int i=0; i<GRAPH_POINTS; i++) tempHistory[i] = 10.0;
  maxScale = getMaxTempHistory();
  assert_float("Escalado Minimo (100 default * 1.1)", maxScale, 110.0);

  Serial.println("-----------------------------------");
  Serial.print("Resultado: "); Serial.print(testsPassed);
  Serial.print("/"); Serial.print(testsRun); Serial.println(" pasados.");
  
  if (testsPassed == testsRun) Serial.println(">>> TODO OK <<<");
  else Serial.println(">>> ERRORES DETECTADOS <<<");
}

void loop() {}
