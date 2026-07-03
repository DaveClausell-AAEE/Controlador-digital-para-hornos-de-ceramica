// =================================================================
// ==      CONTROLADOR DIGITAL PARA HORNO DE CERÁMICA V13.0       ==
// =================================================================
//      (Arquitectura Industrial, SPI Dual, LED Status, Hard Reset)
//

#include <SPI.h>
#include <TFT_eSPI.h> 
#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#include <LittleFS.h>
#include <SD.h>
using namespace fs;
#include <WiFi.h>
#include <WebServer.h>
#include <esp_mac.h>  // Para leer MAC desde eFuse directamente (ESP-IDF v5+)

// --- CONFIGURACIÓN SPI DUAL ---
// VSPI: SCK 18, MISO 19, MOSI 23, SS 15 (TFT) - Manejado por SPI global
// HSPI: SCK 14, MISO 13, MOSI 12 (NC), SS 5 (SENSOR)
SPIClass hspi(HSPI);

// --- PINOUT V13.0 ---
#define RELAY_PIN     17
#define BUZZER_PIN    27
#define LED_R_PIN     16
#define LED_G_PIN     21
#define LED_B_PIN     22
#define BTN_UP_PIN    36 // SVP
#define BTN_DOWN_PIN  39 // SVN
#define BTN_OK_PIN    34
#define BTN_EXIT_PIN  35
#define SD_CS         33 
#define TFT_LED       12 
#define MAXCS         5

// --- WiFi DYNAMICS ---
struct WiFiConfig { char ssid[32]; char pass[64]; bool configurado; };
WiFiConfig wConfig = {"", "", false};
WebServer server(80);
unsigned long lastWiFiCheck = 0;
bool wifiConectado = false;
bool modoAP = false;
bool sdInicializada = false;
const char* apSSID = "HORNO-CONFIG";

// --- SENSOR ---
Adafruit_MAX31855 thermocouple(MAXCS, &hspi);

// --- ESTRUCTURAS ---
#define MAX_ETAPAS 10
#define MAX_LARGO_NOMBRE 16
#define MAX_PROGRAMAS 5
#define STATUS_BAR_HEIGHT 30
#define GRAPH_POINTS 120 

struct Etapa { int rampa, temperatura, tiempo; };
struct Programa { char nombre[MAX_LARGO_NOMBRE]; Etapa etapas[MAX_ETAPAS]; int numEtapas; };

TFT_eSPI tft = TFT_eSPI(); 

// --- VARIABLES DE ESTADO ---
struct RecoveryData { int estadoActual; int etapaActualIndex; unsigned long tiempoInicioEtapa; };
Programa programas[MAX_PROGRAMAS];
int numProgramasGuardados = 0;
int programaActivoIndex = 0;
float currentTemperature = 25.0;
float prevTemperature = 0.0;
float temperaturaCrudaSimulada = 27.2; 
float calibracionOffset = 0.0;
bool sonidoHabilitado = true;

// Gráfica
float tempHistory[GRAPH_POINTS];
int graphIdx = 0;
unsigned long ultimoPuntoGrafo = 0;

enum EstadoHorno {
  STAND_BY, CALENTANDO, MANTENIENDO, ENFRIANDO, FALLO, 
  MENU_PRINCIPAL, MENU_SELECCION_PROG, MENU_CONFIG_PROG,
  MENU_AJUSTES, MENU_CALIBRACION, EDITANDO_NOMBRE,
  CONFIRMACION_INICIO, INFO_WIFI, MENU_AJUSTES_PID, MENU_AJUSTES_BRILLO
};
EstadoHorno estadoActual = STAND_BY;
EstadoHorno estadoPrevio = STAND_BY;

const char* errorMsgStr = "";
bool enModoEdicionHorizontal = false;
int menuPrincipalCursor = 0, menuAjustesCursor = 0, menuSeleccionProgCursor = 0;
int programaEnEdicionIndex = 0, configProgCursorVertical = 0, configProgCursorHorizontal = 0, charIndexEdicion = 0;
float valorCalibracionEditado = 0.0;

// Watchdog
unsigned long ultimoCheckWatchdog = 0;
float tempEnUltimoCheck = 0;
const unsigned long INTERVALO_WATCHDOG = 600000; 

unsigned long ultimoPulsoBoton = 0;
unsigned long comboResetStart = 0;
const long intervaloDebounce = 200;
bool necesitaRefresco = true;
bool parpadeoEstado = false;
unsigned long ultimoParpadeo = 0;

const char* menuPrincipalItems[] = {"Iniciar Programa", "Sel. Programa", "Ajustes"};
const char* menuAjustesItems[] = {"Info Sistema", "Calibracion", "Ajustes PID", "Brillo", "Sonido", "Reset WiFi"};
int brilloPantalla = 255;
int brilloRGB = 25; // Brillo por defecto de los LEDs RGB (10% de 255)
int menuPIDCursor = 0;

// --- PID ---
double pidSetpoint, pidInput, pidOutput;
double Kp=10, Ki=0.2, Kd=1; 
PID hornoPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);
unsigned long windowSize = 5000, windowStartTime;
int etapaActualIndex = 0;
unsigned long tiempoInicioEtapa = 0;
float tempInicioEtapa = 0;
unsigned long ultimoUpdateTempSimulada = 0;
const int SIM_MULTIPLIER = 60; 

// --- PROTOTIPOS ---
const char* getEstadoStr(EstadoHorno e);
void leerSensores();
void manejarPulsadores();
void procesarEntrada(bool, bool, bool, bool);
void ejecutarCicloDeHorneado();
void actualizarPantalla();
void dibujarBarraDeEstado();
void actualizarTemperaturaEnBarra();
void dibujarPantallaStandBy(bool);
void dibujarItemMenu(int, bool, const char*, int);
void dibujarMenuPrincipal(bool);
void dibujarMenuAjustes(bool);
void dibujarItemMenuSeleccion(int, bool);
void dibujarMenuSeleccionProg(bool);
void dibujarMenuConfigProg(bool);
void dibujarLineaDeEtapa(int);
void dibujarPantallaCalentando(bool);
void dibujarPantallaEnfriando(bool);
void dibujarPantallaCalibracion();
void dibujarPantallaFallo(const char*);
void dibujarPantallaConfirmacion();
void dibujarPantallaInfoWiFi();
void dibujarMenuPID(bool);
void dibujarMenuBrillo();
void dibujarPantallaBienvenida();
void dibujarGrafica();
void iniciarWiFi();
void iniciarModoAP();
void manejarRaizWeb();
void manejarConfigWiFi();
void guardarWiFiConfig();
void cargarWiFiConfig();
void borrarWiFiConfig();
String obtenerMACAddress(); // Lee MAC desde eFuse, sin depender del estado WiFi
void cargarProgramaDePrueba();
void guardarConfiguracion();
void cargarConfiguracion();
void actualizarLEDStatus();
void sonarClick();
void sonarBuzzer(int duracion, int veces = 1);

void setup() {
  Serial.begin(115200);
  
  // Configuración de Botones (Requieren Pull-up externo a 3.3V)
  pinMode(BTN_UP_PIN, INPUT);
  pinMode(BTN_DOWN_PIN, INPUT);
  pinMode(BTN_OK_PIN, INPUT);
  pinMode(BTN_EXIT_PIN, INPUT);
  
  // Pines de Salida
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  // Configuración de LED RGB con PWM para control de brillo
  ledcAttach(LED_R_PIN, 1000, 8);
  ledcAttach(LED_G_PIN, 1000, 8);
  ledcAttach(LED_B_PIN, 1000, 8);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  
  digitalWrite(RELAY_PIN, HIGH); // Off (Active Low)
  digitalWrite(BUZZER_PIN, LOW);
  
  // Aislamiento de pines no usados (Recomendación Industrial)
  int pinsNoUsados[] = {0, 25, 26, 32}; // Pines liberados en V14.0
  for(int p : pinsNoUsados) pinMode(p, INPUT_PULLUP);

  // Iniciar bus SPI global (VSPI) con pines explícitos para pantalla y SD
  SPI.begin(18, 19, 23, 15); // SCK, MISO, MOSI, SS (VSPI)

  // Iniciar bus HSPI para el sensor de temperatura (sin MOSI en pin 12 para evitar conflictos con PWM de pantalla)
  hspi.begin(14, 13, -1, 5); // SCK, MISO, MOSI(NC), SS (HSPI)

  if(!LittleFS.begin(true)) Serial.println("LittleFS error");

  // Iniciar pantalla TFT primero (inicializa el bus SPI global por defecto)
  tft.init(); 
  tft.setRotation(3);

  // Brillo Pantalla (PWM)
  ledcAttach(TFT_LED, 1000, 8); 
  cargarConfiguracion();
  ledcWrite(TFT_LED, brilloPantalla);
  
  dibujarPantallaBienvenida();

  // --- PERSONALITY MODULE ---
  // Iniciar SD en el bus SPI global compartido
  if(SD.begin(SD_CS)) {
    sdInicializada = true;
    Serial.println("SD Detectada y Montada. Verificando integridad...");
    verificarYClonarConfiguracionSD();
  } else {
    Serial.println("SD No Presente o Error al montar.");
  }
  
  if (!thermocouple.begin()) Serial.println("MAX31855 error");
  
  cargarEstadoRecuperacion(); 
  hornoPID.SetOutputLimits(0, windowSize);
  hornoPID.SetMode(AUTOMATIC);
  for(int i=0; i<GRAPH_POINTS; i++) tempHistory[i] = 0;

  tft.fillScreen(TFT_BLACK);
  estadoPrevio = (EstadoHorno)-1; 
  necesitaRefresco = true;
  
  iniciarWiFi();
  server.on("/", manejarRaizWeb);
  server.on("/save", manejarConfigWiFi);
  server.begin();
  
  actualizarLEDStatus();
  sonarBuzzer(100, 2);
}

void loop() {
  server.handleClient();
  
  // Gestión de WiFi y Reconexión
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED && !modoAP) {
      if (wifiConectado) { wifiConectado = false; WiFi.begin(wConfig.ssid, wConfig.pass); }
    } else if (WiFi.status() == WL_CONNECTED) {
      if (!wifiConectado) { wifiConectado = true; actualizarLEDStatus(); }
    }
  }

  leerSensores();
  manejarPulsadores();
  ejecutarCicloDeHorneado();
  
  // Parpadeo para UI y LED (si es necesario)
  if (millis() - ultimoParpadeo > 400) {
    ultimoParpadeo = millis(); 
    parpadeoEstado = !parpadeoEstado;
    if (estadoActual == EDITANDO_NOMBRE || enModoEdicionHorizontal || modoAP) necesitaRefresco = true;
    if (modoAP) actualizarLEDStatus(); // Hacer parpadear el LED en modo AP
  }
  
  // Actualizar historial de gráfica
  if ((estadoActual == CALENTANDO || estadoActual == MANTENIENDO) && (millis() - ultimoPuntoGrafo > 5000)) {
    ultimoPuntoGrafo = millis();
    tempHistory[graphIdx] = currentTemperature;
    graphIdx = (graphIdx + 1) % GRAPH_POINTS;
    necesitaRefresco = true;
  }
  
  actualizarPantalla();
}

// --- LÓGICA DE LED STATUS RGB ---
void setLEDColor(bool r, bool g, bool b) {
  ledcWrite(LED_R_PIN, r ? brilloRGB : 0);
  ledcWrite(LED_G_PIN, g ? brilloRGB : 0);
  ledcWrite(LED_B_PIN, b ? brilloRGB : 0);
}

void actualizarLEDStatus() {
  if (estadoActual == FALLO) { setLEDColor(1, 0, 0); return; } // ROJO
  if (modoAP) { setLEDColor(0, parpadeoEstado, parpadeoEstado); return; } // CIAN PARPADEO
  
  switch(estadoActual) {
    case CALENTANDO: setLEDColor(1, 1, 0); break; // AMARILLO/NARANJA
    case MANTENIENDO: setLEDColor(0, 1, 0); break; // VERDE
    case ENFRIANDO: setLEDColor(0, 1, 0); break; // VERDE
    case STAND_BY: setLEDColor(0, 0, 1); break; // AZUL (Standby)
    default: setLEDColor(0, 0, 1); break;
  }
}

void leerSensores() {
  double c = thermocouple.readCelsius();
  if (isnan(c)) {
    // Modo Simulación si falla sensor
    if (millis() - ultimoUpdateTempSimulada > 500) {
      ultimoUpdateTempSimulada = millis();
      if (digitalRead(RELAY_PIN) == LOW) temperaturaCrudaSimulada += 1.0;
      else if (temperaturaCrudaSimulada > 27.2) temperaturaCrudaSimulada -= 0.2;
      necesitaRefresco = true;
    }
    currentTemperature = temperaturaCrudaSimulada - calibracionOffset;
  } else {
    currentTemperature = c - calibracionOffset;
    temperaturaCrudaSimulada = c;
    if (abs(currentTemperature - prevTemperature) >= 0.5) { prevTemperature = currentTemperature; necesitaRefresco = true; }
  }
}

void manejarPulsadores() {
  bool u = digitalRead(BTN_UP_PIN) == LOW;
  bool d = digitalRead(BTN_DOWN_PIN) == LOW;
  bool o = digitalRead(BTN_OK_PIN) == LOW;
  bool e = digitalRead(BTN_EXIT_PIN) == LOW;

  // --- LÓGICA HARD RESET (OK + EXIT por 3s) ---
  if (o && e) {
    if (comboResetStart == 0) comboResetStart = millis();
    if (millis() - comboResetStart > 3000) {
      sonarBuzzer(100, 3);
      delay(500);
      ESP.restart();
    }
    return; // No procesar otras teclas si se mantiene el combo
  } else {
    comboResetStart = 0;
  }

  if (millis() - ultimoPulsoBoton < intervaloDebounce) return;
  if (u || d || o || e) { 
    ultimoPulsoBoton = millis(); 
    procesarEntrada(u, d, o, e); 
    necesitaRefresco = true; 
    actualizarLEDStatus();
  }
}

void procesarEntrada(bool subiendo, bool bajando, bool ok, bool exit) {
    if (subiendo || bajando || ok || exit) sonarClick();
    estadoPrevio = estadoActual;
    switch (estadoActual) {
      case STAND_BY: if (ok) estadoActual = MENU_PRINCIPAL; break;
      case MENU_PRINCIPAL:
        if (subiendo) menuPrincipalCursor = (menuPrincipalCursor - 1 + 3) % 3;
        if (bajando) menuPrincipalCursor = (menuPrincipalCursor + 1) % 3;
        if (exit) estadoActual = STAND_BY;
        if (ok) {
          if (menuPrincipalCursor == 0) estadoActual = CONFIRMACION_INICIO;
          if (menuPrincipalCursor == 1) estadoActual = MENU_SELECCION_PROG;
          if (menuPrincipalCursor == 2) { estadoActual = MENU_AJUSTES; menuAjustesCursor = 0; }
        }
        break;
      case CONFIRMACION_INICIO:
        if (ok) {
           etapaActualIndex = 0; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature;
           pidSetpoint = currentTemperature; windowStartTime = millis();
           ultimoCheckWatchdog = millis(); tempEnUltimoCheck = currentTemperature;
           for(int i=0; i<GRAPH_POINTS; i++) tempHistory[i] = 0; graphIdx = 0;
           programaActivoIndex = menuSeleccionProgCursor; // Asegurar que usa el seleccionado
           estadoActual = CALENTANDO;
        }
        if (exit) estadoActual = MENU_PRINCIPAL;
        break;
      case MENU_AJUSTES:
        if (subiendo) menuAjustesCursor = (menuAjustesCursor - 1 + 6) % 6;
        if (bajando) menuAjustesCursor = (menuAjustesCursor + 1) % 6;
        if (exit) estadoActual = MENU_PRINCIPAL;
        if (ok) {
          if (menuAjustesCursor == 0) estadoActual = INFO_WIFI;
          if (menuAjustesCursor == 1) { valorCalibracionEditado = currentTemperature; estadoActual = MENU_CALIBRACION; }
          if (menuAjustesCursor == 2) { menuPIDCursor = 0; estadoActual = MENU_AJUSTES_PID; }
          if (menuAjustesCursor == 3) { estadoActual = MENU_AJUSTES_BRILLO; }
          if (menuAjustesCursor == 4) { sonidoHabilitado = !sonidoHabilitado; guardarConfiguracion(); }
          if (menuAjustesCursor == 5) { borrarWiFiConfig(); }
        }
        break;
      case MENU_AJUSTES_BRILLO:
        if (subiendo) { brilloPantalla += 25; if (brilloPantalla > 255) brilloPantalla = 255; ledcWrite(TFT_LED, brilloPantalla); }
        if (bajando) { brilloPantalla -= 25; if (brilloPantalla < 5) brilloPantalla = 5; ledcWrite(TFT_LED, brilloPantalla); }
        if (ok || exit) { guardarConfiguracion(); estadoActual = MENU_AJUSTES; }
        break;
      case MENU_AJUSTES_PID:
        if (subiendo || bajando) {
          double inc = (menuPIDCursor == 0) ? 1.0 : ((menuPIDCursor == 1) ? 0.05 : 0.1);
          if (menuPIDCursor == 0) Kp += (subiendo ? inc : -inc);
          else if (menuPIDCursor == 1) Ki += (subiendo ? inc : -inc);
          else if (menuPIDCursor == 2) Kd += (subiendo ? inc : -inc);
          if (Kp < 0) Kp = 0; if (Ki < 0) Ki = 0; if (Kd < 0) Kd = 0;
          hornoPID.SetTunings(Kp, Ki, Kd);
        }
        if (ok) { menuPIDCursor = (menuPIDCursor + 1) % 4; if (menuPIDCursor == 3) { guardarConfiguracion(); estadoActual = MENU_AJUSTES; } }
        if (exit) { guardarConfiguracion(); estadoActual = MENU_AJUSTES; }
        break;
      case INFO_WIFI: if (ok || exit) estadoActual = MENU_AJUSTES; break;
      case MENU_CALIBRACION:
        if (subiendo) valorCalibracionEditado += 1.0;
        if (bajando) valorCalibracionEditado -= 1.0;
        if (exit) estadoActual = MENU_AJUSTES;
        if (ok) { 
           if (!isnan(currentTemperature)) { calibracionOffset = valorCalibracionEditado - temperaturaCrudaSimulada; guardarConfiguracion(); }
           estadoActual = MENU_AJUSTES; 
        }
        break;
      case MENU_SELECCION_PROG:
        if (subiendo) menuSeleccionProgCursor = (menuSeleccionProgCursor - 1 + (numProgramasGuardados + 1)) % (numProgramasGuardados + 1);
        if (bajando) menuSeleccionProgCursor = (menuSeleccionProgCursor + 1) % (numProgramasGuardados + 1);
        if (exit) estadoActual = MENU_PRINCIPAL;
        if (ok) {
            if (menuSeleccionProgCursor < numProgramasGuardados) { programaEnEdicionIndex = menuSeleccionProgCursor; configProgCursorVertical = 0; enModoEdicionHorizontal = false; estadoActual = MENU_CONFIG_PROG; }
            else if (numProgramasGuardados < MAX_PROGRAMAS) {
                int n = numProgramasGuardados; sprintf(programas[n].nombre, "NUEVO %d", n+1);
                programas[n].numEtapas = 1; programas[n].etapas[0] = {100, 100, 0};
                numProgramasGuardados++; guardarConfiguracion(); programaEnEdicionIndex = n;
                configProgCursorVertical = 0; enModoEdicionHorizontal = false; estadoActual = MENU_CONFIG_PROG;
            }
        }
        break;
      case MENU_CONFIG_PROG:
        if (enModoEdicionHorizontal) {
          if (ok) configProgCursorHorizontal = (configProgCursorHorizontal + 1) % 3;
          if (exit) enModoEdicionHorizontal = false;
          if (subiendo || bajando) {
            int etIdx = configProgCursorVertical - 1;
            int inc = (configProgCursorHorizontal == 1) ? 5 : ((configProgCursorHorizontal == 0) ? 10 : 1);
            if (configProgCursorHorizontal == 0) { programas[programaEnEdicionIndex].etapas[etIdx].rampa += (subiendo ? inc : -inc); if (programas[programaEnEdicionIndex].etapas[etIdx].rampa < 1) programas[programaEnEdicionIndex].etapas[etIdx].rampa = 1; }
            else if (configProgCursorHorizontal == 1) { programas[programaEnEdicionIndex].etapas[etIdx].temperatura += (subiendo ? inc : -inc); if (programas[programaEnEdicionIndex].etapas[etIdx].temperatura < 25) programas[programaEnEdicionIndex].etapas[etIdx].temperatura = 25; }
            else { programas[programaEnEdicionIndex].etapas[etIdx].tiempo += (subiendo ? inc : -inc); if (programas[programaEnEdicionIndex].etapas[etIdx].tiempo < 0) programas[programaEnEdicionIndex].etapas[etIdx].tiempo = 0; }
          }
        } else {
          int numOpciones = programas[programaEnEdicionIndex].numEtapas + 3;
          if (subiendo) configProgCursorVertical = (configProgCursorVertical - 1 + numOpciones) % numOpciones;
          if (bajando) configProgCursorVertical = (configProgCursorVertical + 1) % numOpciones;
          if (exit) { guardarConfiguracion(); estadoActual = MENU_SELECCION_PROG; }
          if (ok) {
            if (configProgCursorVertical == 0) { charIndexEdicion = 0; estadoActual = EDITANDO_NOMBRE; }
            else if (configProgCursorVertical <= programas[programaEnEdicionIndex].numEtapas) { enModoEdicionHorizontal = true; configProgCursorHorizontal = 0; }
            else if (configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 1) {
              if (programas[programaEnEdicionIndex].numEtapas < MAX_ETAPAS) { int i = programas[programaEnEdicionIndex].numEtapas; programas[programaEnEdicionIndex].etapas[i] = {100, 100, 0}; programas[programaEnEdicionIndex].numEtapas++; }
            }
            else if (configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 2) {
              if (programas[programaEnEdicionIndex].numEtapas > 1) { programas[programaEnEdicionIndex].numEtapas--; if (configProgCursorVertical > programas[programaEnEdicionIndex].numEtapas) configProgCursorVertical = programas[programaEnEdicionIndex].numEtapas; }
            }
          }
        }
        break;
      case EDITANDO_NOMBRE:
        if (subiendo || bajando) {
          char c = programas[programaEnEdicionIndex].nombre[charIndexEdicion];
          if (subiendo) { c++; if (c > 'Z') c = ' '; if (c == ' '+1) c = 'A'; } else { c--; if (c < ' ') c = 'Z'; if (c == 'A'-1) c = ' '; }
          programas[programaEnEdicionIndex].nombre[charIndexEdicion] = c;
        }
        if (ok) { charIndexEdicion++; if (charIndexEdicion >= MAX_LARGO_NOMBRE - 1) { charIndexEdicion = 0; estadoActual = MENU_CONFIG_PROG; } }
        if (exit) { charIndexEdicion = 0; estadoActual = MENU_CONFIG_PROG; }
        break;
      case FALLO: if (ok || exit) { estadoActual = STAND_BY; actualizarLEDStatus(); } break;
      default: if (exit) estadoActual = STAND_BY; break;
    }
}

void ejecutarCicloDeHorneado() {
  if (estadoActual != CALENTANDO && estadoActual != MANTENIENDO) { digitalWrite(RELAY_PIN, HIGH); return; }
  
  // Watchdog térmico industrial
  if (millis() - ultimoCheckWatchdog > INTERVALO_WATCHDOG) {
    if (digitalRead(RELAY_PIN) == LOW && (currentTemperature < tempEnUltimoCheck + 2.0)) {
       errorMsgStr = "SIN AUMENTO DE TEMP: REVISAR RESISTENCIA"; estadoActual = FALLO; digitalWrite(RELAY_PIN, HIGH); sonarBuzzer(500, 3); actualizarLEDStatus(); return;
    }
    ultimoCheckWatchdog = millis(); tempEnUltimoCheck = currentTemperature;
  }
  
  pidInput = currentTemperature;
  Etapa etapa = programas[programaActivoIndex].etapas[etapaActualIndex];
  unsigned long tSim = (millis() - tiempoInicioEtapa) * SIM_MULTIPLIER;
  
  if (estadoActual == CALENTANDO) {
    pidSetpoint = tempInicioEtapa + (etapa.rampa * (tSim / 3600000.0));
    if (pidSetpoint > etapa.temperatura) pidSetpoint = etapa.temperatura;
    if (currentTemperature >= etapa.temperatura) { estadoActual = MANTENIENDO; tiempoInicioEtapa = millis(); pidSetpoint = etapa.temperatura; guardarEstadoRecuperacion(); actualizarLEDStatus(); }
  } else if (estadoActual == MANTENIENDO) {
    if ((millis() - tiempoInicioEtapa) >= ((unsigned long)etapa.tiempo * 60000 / SIM_MULTIPLIER)) {
      etapaActualIndex++;
      if (etapaActualIndex >= programas[programaActivoIndex].numEtapas) { estadoActual = ENFRIANDO; LittleFS.remove("/recovery.bin"); sonarBuzzer(1000, 1); actualizarLEDStatus(); }
      else { estadoActual = CALENTANDO; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature; guardarEstadoRecuperacion(); actualizarLEDStatus(); }
    }
  }
  
  hornoPID.Compute();
  if (millis() - windowStartTime > windowSize) windowStartTime += windowSize;
  digitalWrite(RELAY_PIN, (pidOutput > (millis() - windowStartTime)) ? LOW : HIGH);
}

void sonarBuzzer(int duracion, int veces) {
  for(int i=0; i<veces; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(duracion); digitalWrite(BUZZER_PIN, LOW); if (i < veces - 1) delay(100); }
}

void sonarClick() { if (sonidoHabilitado) { digitalWrite(BUZZER_PIN, HIGH); delay(20); digitalWrite(BUZZER_PIN, LOW); } }

// --- UI Y PANTALLAS (Mantenidas de V12.2 con mejoras) ---
void actualizarPantalla() {
  if (estadoActual != estadoPrevio) {
    tft.fillRect(0, STATUS_BAR_HEIGHT, tft.width(), tft.height() - STATUS_BAR_HEIGHT, TFT_BLACK);
    dibujarBarraDeEstado();
    switch (estadoActual) {
      case STAND_BY: dibujarPantallaStandBy(true); break;
      case MENU_PRINCIPAL: dibujarMenuPrincipal(true); break;
      case CONFIRMACION_INICIO: dibujarPantallaConfirmacion(); break;
      case MENU_AJUSTES: dibujarMenuAjustes(true); break; 
      case INFO_WIFI: dibujarPantallaInfoWiFi(); break;
      case MENU_CALIBRACION: dibujarPantallaCalibracion(); break;
      case MENU_AJUSTES_PID: dibujarMenuPID(true); break;
      case MENU_AJUSTES_BRILLO: dibujarMenuBrillo(); break;
      case MENU_SELECCION_PROG: dibujarMenuSeleccionProg(true); break;
      case MENU_CONFIG_PROG: case EDITANDO_NOMBRE: dibujarMenuConfigProg(true); break;
      case CALENTANDO: case MANTENIENDO: dibujarPantallaCalentando(true); break;
      case ENFRIANDO: dibujarPantallaEnfriando(true); break;
      case FALLO: dibujarPantallaFallo(errorMsgStr); break;
    }
  } else if (necesitaRefresco) {
        actualizarTemperaturaEnBarra();
        if (estadoActual == STAND_BY) dibujarPantallaStandBy(false);
        else if (estadoActual == MENU_PRINCIPAL) dibujarMenuPrincipal(false);
        else if (estadoActual == MENU_AJUSTES) dibujarMenuAjustes(false);
        else if (estadoActual == MENU_AJUSTES_PID) dibujarMenuPID(false);
        else if (estadoActual == MENU_AJUSTES_BRILLO) dibujarMenuBrillo();
        else if (estadoActual == MENU_SELECCION_PROG) dibujarMenuSeleccionProg(false);
        else if (estadoActual == MENU_CONFIG_PROG || estadoActual == EDITANDO_NOMBRE) dibujarMenuConfigProg(false);
        else if (estadoActual == CALENTANDO || estadoActual == MANTENIENDO) dibujarPantallaCalentando(false);
  }
  estadoPrevio = estadoActual; necesitaRefresco = false;
}

void dibujarBarraDeEstado() { tft.fillRect(0, 0, tft.width(), STATUS_BAR_HEIGHT, TFT_DARKGREY); actualizarTemperaturaEnBarra(); }

const char* getEstadoStr(EstadoHorno e) {
  switch(e) {
    case STAND_BY: return "LISTO";
    case CALENTANDO: return "CALENT.";
    case MANTENIENDO: return "MANTEN.";
    case ENFRIANDO: return "FIN";
    case FALLO: return "FALLO";
    case MENU_PRINCIPAL: return "MENU";
    case MENU_SELECCION_PROG: return "SEL.PROG";
    case MENU_CONFIG_PROG: return "CONFIG";
    case MENU_AJUSTES: return "AJUSTES";
    case INFO_WIFI: return "INFO";
    default: return "";
  }
}

void actualizarTemperaturaEnBarra() {
  tft.fillRect(0, 0, tft.width(), STATUS_BAR_HEIGHT, TFT_DARKGREY); 
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); 
  tft.setCursor(5, 7); tft.print(currentTemperature, 0); tft.print("c"); 
  tft.setCursor(100, 7); tft.setTextColor(TFT_YELLOW); tft.print(getEstadoStr(estadoActual));
  
  // Indicador de SD en la barra de estado
  if (sdInicializada) {
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(tft.width() - 65, 7);
    tft.print("SD");
  } else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(tft.width() - 85, 7);
    tft.print("NO SD");
  }

  if (WiFi.status() == WL_CONNECTED) { tft.fillCircle(tft.width()-15, 15, 5, TFT_GREEN); }
}

void dibujarPantallaStandBy(bool r) {
  if (modoAP) {
    if (r) {
      tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(20, 45); tft.print("MODO CONFIGURACION");
      tft.setTextColor(TFT_WHITE); tft.setCursor(20, 80); tft.print("Red WiFi: "); tft.print(apSSID);
      tft.setCursor(20, 105); tft.print("IP: 192.168.4.1");
      // --- MAC address en caja resaltada para whitelist de red ---
      tft.setTextColor(TFT_CYAN); tft.setCursor(20, 135); tft.print("MAC del dispositivo:");
      String mac = obtenerMACAddress();
      tft.drawRect(15, 153, 290, 28, TFT_CYAN);
      tft.setTextColor(TFT_WHITE); tft.setCursor(22, 159); tft.print(mac);
      tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setCursor(20, 190); tft.print("Proporciona este dato a IT para");
      tft.setCursor(20, 200); tft.print("habilitar el acceso a la red.");
    }
    return;
  }
  if (r) { tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(50, 60); tft.print("Temp. Actual:"); tft.setCursor(50, 150); tft.print("Programa:"); tft.setTextColor(TFT_CYAN); tft.setCursor(60, 180); tft.println(programas[programaActivoIndex].nombre); }
  tft.fillRect(80, 90, 120, 40, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(80, 90); tft.print(currentTemperature, 0); tft.print("c");
}

void dibujarItemMenu(int i, bool s, const char* t, int y) { 
  int yp = y + (i * 35); 
  tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); 
  tft.setTextColor(s ? TFT_BLACK : TFT_WHITE); tft.setTextSize(2); 
  tft.setCursor(10, yp + 7); tft.print(t);
  if (strcmp(t, "Sonido") == 0) { tft.setCursor(220, yp + 7); tft.print(sonidoHabilitado ? "SI" : "NO"); }
}

void dibujarMenuPrincipal(bool r) { for (int i = 0; i < 3; i++) dibujarItemMenu(i, i == menuPrincipalCursor, menuPrincipalItems[i], 40); }
void dibujarMenuAjustes(bool r) { for (int i = 0; i < 6; i++) dibujarItemMenu(i, i == menuAjustesCursor, menuAjustesItems[i], 40); }
void dibujarItemMenuSeleccion(int i, bool s) { int yp = 40 + (i * 35); tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); tft.setTextColor(s ? TFT_BLACK : (i < numProgramasGuardados ? TFT_WHITE : TFT_GREEN)); tft.setCursor(10, yp + 7); tft.println(i < numProgramasGuardados ? programas[i].nombre : "+ Crear Programa"); }
void dibujarMenuSeleccionProg(bool r) { for (int i = 0; i <= numProgramasGuardados; i++) dibujarItemMenuSeleccion(i, i == menuSeleccionProgCursor); }

void dibujarMenuConfigProg(bool r) { 
  if (r) tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK); 
  dibujarLineaDeEtapa(-1); 
  for (int i = 0; i < programas[programaEnEdicionIndex].numEtapas; i++) dibujarLineaDeEtapa(i); 
  int baseYP = 80 + programas[programaEnEdicionIndex].numEtapas * 25;
  dibujarItemMenu(0, configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 1, "+ Anadir Etapa", baseYP);
  dibujarItemMenu(1, configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 2, "- Borrar Etapa", baseYP);
}

void dibujarLineaDeEtapa(int idx) {
  int yp = (idx == -1) ? 40 : 80 + idx * 25; bool sel = (idx + 1 == configProgCursorVertical);
  if (idx == -1) { 
    tft.fillRect(0, yp, tft.width(), 30, (configProgCursorVertical == 0) ? TFT_CYAN : TFT_BLACK); 
    tft.setTextColor((configProgCursorVertical == 0) ? (parpadeoEstado && estadoActual == EDITANDO_NOMBRE ? TFT_YELLOW : TFT_BLACK) : TFT_CYAN); 
    tft.setCursor(10, yp + 5); tft.setTextSize(3); tft.print(programas[programaEnEdicionIndex].nombre);
  } else {
    uint16_t bg = (sel && !enModoEdicionHorizontal) ? TFT_CYAN : TFT_BLACK, tx = (sel && !enModoEdicionHorizontal) ? TFT_BLACK : TFT_WHITE;
    tft.fillRect(0, yp, tft.width(), 22, bg); tft.setTextColor(tx, bg); tft.setTextSize(2); tft.setCursor(5, yp+3); tft.print("E"); tft.print(idx+1);
    Etapa e = programas[programaEnEdicionIndex].etapas[idx];
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 0) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, bg); else tft.setTextColor(tx, bg);
    tft.setCursor(50, yp+3); tft.print("R:"); tft.print(e.rampa);
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 1) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, bg); else tft.setTextColor(tx, bg);
    tft.setCursor(130, yp+3); tft.print("T:"); tft.print(e.temperatura);
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 2) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, bg); else tft.setTextColor(tx, bg);
    tft.setCursor(220, yp+3); tft.print("m:"); tft.print(e.tiempo);
  }
}

void dibujarPantallaConfirmacion() {
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(20, 50); tft.print("INICIAR HORNEADO?");
  tft.setTextColor(TFT_WHITE); tft.setCursor(20, 90); tft.print("Prog: "); tft.print(programas[menuSeleccionProgCursor].nombre);
  tft.setCursor(20, 120); tft.print("Etapas: "); tft.print(programas[menuSeleccionProgCursor].numEtapas);
  tft.setCursor(20, 150); tft.print("T. Final: "); tft.print(programas[menuSeleccionProgCursor].etapas[programas[menuSeleccionProgCursor].numEtapas-1].temperatura); tft.print("c");
  tft.setTextColor(TFT_GREEN); tft.setCursor(50, 200); tft.print("OK: INICIAR  EXIT: NO");
}

void dibujarPantallaInfoWiFi() {
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2); tft.setCursor(20, 40); tft.print("INFO SISTEMA");
  // --- IP ---
  tft.setTextColor(TFT_WHITE); tft.setCursor(20, 72); tft.print("IP: ");
  if (WiFi.status() == WL_CONNECTED) tft.setTextColor(TFT_GREEN); else tft.setTextColor(TFT_RED);
  tft.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : (modoAP ? "CONFIG AP" : "DESCONECTADO"));
  // --- MAC address en caja resaltada (util para whitelist IT) ---
  tft.setTextColor(TFT_CYAN); tft.setCursor(20, 100); tft.print("MAC (para IT/whitelist):");
  String macAddr = obtenerMACAddress();
  tft.drawRect(15, 117, 290, 28, TFT_CYAN);
  tft.fillRect(16, 118, 288, 26, 0x0821); // fondo azul muy oscuro para resaltar
  tft.setTextColor(TFT_WHITE); tft.setCursor(22, 124); tft.print(macAddr);
  // --- LittleFS ---
  tft.setTextColor(TFT_WHITE); tft.setCursor(20, 155);
  size_t total = LittleFS.totalBytes(); size_t used = LittleFS.usedBytes();
  tft.print("LittleFS: "); tft.print((float)used/total*100, 1); tft.print("%");
  // --- SD ---
  tft.setCursor(20, 178); tft.print("SD: ");
  tft.setTextColor(sdInicializada ? TFT_GREEN : TFT_RED);
  tft.print(sdInicializada ? "OK" : "No detectada");
  // --- Version ---
  tft.setTextColor(TFT_DARKGREY); tft.setCursor(20, 200); tft.print("V: 14.0 INDUSTRIAL");
  tft.setTextColor(TFT_YELLOW); tft.setCursor(20, 218); tft.print("OK para volver");
}

void dibujarMenuPID(bool r) {
  if (r) tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2); tft.setCursor(20, 50); tft.print("AJUSTES PID");
  auto drawP = [&](int i, const char* label, double val, int y) {
    bool sel = (menuPIDCursor == i);
    tft.fillRect(10, y-5, 300, 30, sel ? TFT_WHITE : TFT_BLACK);
    tft.setTextColor(sel ? TFT_BLACK : TFT_WHITE); tft.setCursor(20, y); tft.print(label); tft.setCursor(180, y); tft.print(val, (i==1)?2:1);
  };
  drawP(0, "Kp:", Kp, 90); drawP(1, "Ki:", Ki, 130); drawP(2, "Kd:", Kd, 170);
  tft.fillRect(10, 205, 300, 30, (menuPIDCursor == 3) ? TFT_WHITE : TFT_BLACK);
  tft.setTextColor((menuPIDCursor == 3) ? TFT_BLACK : TFT_YELLOW); tft.setCursor(20, 210); tft.print("GUARDAR Y VOLVER");
}

void dibujarMenuBrillo() {
  tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setCursor(20, 50); tft.print("BRILLO PANTALLA");
  tft.drawRect(40, 120, 240, 30, TFT_WHITE);
  tft.fillRect(42, 122, map(brilloPantalla, 5, 255, 0, 236), 26, TFT_YELLOW);
}

void dibujarPantallaCalentando(bool r) {
  if (r) { tft.setTextColor(TFT_ORANGE); tft.setCursor(20, 45); tft.print(estadoActual == CALENTANDO ? "CALENTANDO" : "MANTENIENDO"); }
  tft.fillRect(150, 45, 120, 30, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(3); tft.setCursor(180, 45); tft.print(currentTemperature, 0); tft.print("c");
  dibujarGrafica();
}

void dibujarGrafica() {
  int x0 = 35, y0 = 220, w = 275, h = 80;
  tft.drawRect(x0, y0-h, w, h, TFT_DARKGREY);
  float maxT = 100.0;
  for(int i=0; i<GRAPH_POINTS; i++) if(tempHistory[i] > maxT) maxT = tempHistory[i];
  maxT *= 1.1;
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY);
  tft.setCursor(5, y0-h); tft.print((int)maxT); tft.setCursor(5, y0-10); tft.print("0");
  for(int i=0; i<GRAPH_POINTS-1; i++) {
    int p1 = (graphIdx + i) % GRAPH_POINTS; int p2 = (graphIdx + i + 1) % GRAPH_POINTS;
    if (tempHistory[p1] > 0 && tempHistory[p2] > 0) {
      tft.drawLine(x0+(i*w/GRAPH_POINTS), y0-(tempHistory[p1]*h/maxT), x0+((i+1)*w/GRAPH_POINTS), y0-(tempHistory[p2]*h/maxT), TFT_CYAN);
    }
  }
}

void dibujarPantallaBienvenida() {
  tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_ORANGE); tft.setTextSize(3); tft.setCursor(40, 50); tft.print("Controlador by");
  tft.setTextSize(4); tft.setCursor(60, 90); tft.print("DAC LAB");
  tft.drawRect(60, 190, 200, 15, TFT_WHITE);
  for(int i=0; i<196; i++) { tft.fillRect(62, 192, i, 11, TFT_ORANGE); delay(5); }
}

void dibujarPantallaEnfriando(bool r) { tft.setTextColor(TFT_GREEN); tft.setTextSize(3); tft.setCursor(80, 70); tft.print("FINALIZADO"); }
void dibujarPantallaCalibracion() { tft.setTextColor(TFT_WHITE); tft.setCursor(20, 100); tft.print("Lectura Cruda: "); tft.print(temperaturaCrudaSimulada, 1); tft.setCursor(20, 140); tft.print("Temp. Real: "); tft.setTextColor(TFT_BLACK, TFT_CYAN); tft.print(valorCalibracionEditado, 1); }
void dibujarPantallaFallo(const char* msg) { tft.fillScreen(TFT_RED); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(10, 100); tft.print(msg); tft.setCursor(10, 180); tft.print("OK para reset"); }

// --- PERSISTENCIA ---
void guardarConfiguracion() {
  File f = LittleFS.open("/config.bin", "w");
  if (f) {
    f.write((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.write((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.write((uint8_t*)programas, sizeof(programas));
    f.write((uint8_t*)&Kp, sizeof(Kp)); f.write((uint8_t*)&Ki, sizeof(Ki)); f.write((uint8_t*)&Kd, sizeof(Kd));
    f.write((uint8_t*)&brilloPantalla, sizeof(brilloPantalla));
    f.write((uint8_t*)&sonidoHabilitado, sizeof(sonidoHabilitado));
    f.close();
    Serial.println("Configuración guardada en LittleFS.");
  }
  
  if (sdInicializada) {
    File f_sd = SD.open("/config.bin", "w");
    if (f_sd) {
      f_sd.write((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
      f_sd.write((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
      f_sd.write((uint8_t*)programas, sizeof(programas));
      f_sd.write((uint8_t*)&Kp, sizeof(Kp)); f_sd.write((uint8_t*)&Ki, sizeof(Ki)); f_sd.write((uint8_t*)&Kd, sizeof(Kd));
      f_sd.write((uint8_t*)&brilloPantalla, sizeof(brilloPantalla));
      f_sd.write((uint8_t*)&sonidoHabilitado, sizeof(sonidoHabilitado));
      f_sd.close();
      Serial.println("Configuración respaldada en SD (Mirror).");
    } else {
      Serial.println("Error al guardar backup en SD.");
    }
  }
}

// --- PERSONALITY MODULE ---
void verificarYClonarConfiguracionSD() {
  if (sdInicializada) {
    if (SD.exists("/config.bin")) {
      if (!LittleFS.exists("/config.bin")) {
        Serial.println("Clonando configuración desde SD a LittleFS...");
        File source = SD.open("/config.bin", "r");
        File target = LittleFS.open("/config.bin", "w");
        if (source && target) {
          while (source.available()) target.write(source.read());
          target.close();
          source.close();
          Serial.println("Clonación exitosa.");
        }
      }
    } else {
      if (LittleFS.exists("/config.bin")) {
        Serial.println("Creando backup de LittleFS a la SD...");
        File source = LittleFS.open("/config.bin", "r");
        File target = SD.open("/config.bin", "w");
        if (source && target) {
          while (source.available()) target.write(source.read());
          target.close();
          source.close();
          Serial.println("Backup en SD creado con éxito.");
        }
      }
    }
  }
}

void cargarConfiguracion() {
  if (!LittleFS.exists("/config.bin")) { cargarProgramaDePrueba(); guardarConfiguracion(); return; }
  File f = LittleFS.open("/config.bin", "r");
  if (f) {
    f.read((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.read((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.read((uint8_t*)programas, sizeof(programas));
    f.read((uint8_t*)&Kp, sizeof(Kp)); f.read((uint8_t*)&Ki, sizeof(Ki)); f.read((uint8_t*)&Kd, sizeof(Kd));
    f.read((uint8_t*)&brilloPantalla, sizeof(brilloPantalla));
    f.read((uint8_t*)&sonidoHabilitado, sizeof(sonidoHabilitado));
    f.close();
    hornoPID.SetTunings(Kp, Ki, Kd);
  }
}

void cargarProgramaDePrueba() { 
  // Programa 1: CERAMICA1
  strcpy(programas[0].nombre, "CERAMICA1"); 
  programas[0].numEtapas = 4; 
  programas[0].etapas[0] = {180, 400, 15};  // Rampa 3C/min (180C/h), T: 400C, Mant: 15min
  programas[0].etapas[1] = {180, 700, 10};  // Rampa 3C/min (180C/h), T: 700C, Mant: 10min
  programas[0].etapas[2] = {180, 940, 10};  // Rampa 3C/min (180C/h), T: 940C, Mant: 10min
  programas[0].etapas[3] = {180, 1040, 20}; // Rampa 3C/min (180C/h), T: 1040C, Mant: 20min

  // Programa 2: SECADO
  strcpy(programas[1].nombre, "SECADO"); 
  programas[1].numEtapas = 1; 
  programas[1].etapas[0] = {180, 200, 120}; // Rampa 3C/min (180C/h), T: 200C, Mant: 120min (2h)

  numProgramasGuardados = 2; 
}

// --- WiFi ---
void iniciarWiFi() {
  cargarWiFiConfig();
  if (wConfig.configurado) {
    // --- Pantalla de conexión con progreso ---
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2); tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 40); tft.print("Conectando WiFi...");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 75); tft.print(wConfig.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wConfig.ssid, wConfig.pass);
    unsigned long st = millis();
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - st < 20000) {
      delay(500);
      tft.fillRect(20, 110, 280, 25, TFT_BLACK);
      tft.setCursor(20, 110); tft.setTextColor(TFT_YELLOW);
      for (int i = 0; i <= dots % 6; i++) tft.print(".");
      dots++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifiConectado = true;
      tft.setTextColor(TFT_GREEN); tft.setCursor(20, 145); tft.print("Conectado!");
      tft.setCursor(20, 175); tft.setTextColor(TFT_WHITE); tft.print("IP: "); tft.print(WiFi.localIP().toString());
      delay(2500);
      necesitaRefresco = true;
      return;
    }
    // --- Fallo: mostrar motivo ---
    tft.setTextColor(TFT_RED); tft.setCursor(20, 145); tft.print("Sin conexion");
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setCursor(20, 175);
    wl_status_t st2 = WiFi.status();
    if (st2 == WL_NO_SSID_AVAIL)  tft.print("Red no encontrada. Verificar SSID.");
    else if (st2 == WL_CONNECT_FAILED) tft.print("Contrasena incorrecta.");
    else if (st2 == WL_CONNECTION_LOST) tft.print("Conexion perdida.");
    else { tft.print("Timeout. MAC puede no estar"); tft.setCursor(20, 188); tft.print("habilitada en la red corporativa."); }
    tft.setTextColor(TFT_YELLOW); tft.setCursor(20, 210); tft.setTextSize(1);
    tft.print("MAC: "); tft.print(obtenerMACAddress());
    delay(6000); // Dar tiempo para leer el mensaje
  }
  iniciarModoAP();
}

void iniciarModoAP() { WiFi.mode(WIFI_AP); WiFi.softAP(apSSID); modoAP = true; actualizarLEDStatus(); }

void manejarRaizWeb() {
  String h = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>HORNO V13</title></head><body>";
  if (modoAP) {
    h += "<h1>Config WiFi</h1><form action='/save' method='POST'>SSID: <input name='s'><br>Pass: <input name='p' type='password'><br><input type='submit' value='Guardar'></form>";
  } else {
    h += "<h1>Horno Industrial V13</h1><p>Temp: " + String(currentTemperature, 1) + " C</p><p>Estado: " + getEstadoStr(estadoActual) + "</p>";
    h += "<meta http-equiv='refresh' content='5'>";
  }
  h += "</body></html>";
  server.send(200, "text/html", h);
}

void manejarConfigWiFi() {
  if (server.hasArg("s")) {
    strcpy(wConfig.ssid, server.arg("s").c_str()); strcpy(wConfig.pass, server.arg("p").c_str());
    wConfig.configurado = true; guardarWiFiConfig();
    server.send(200, "text/html", "Reinicio..."); delay(2000); ESP.restart();
  }
}

void guardarWiFiConfig() { File f = LittleFS.open("/wifi.bin", "w"); if(f){ f.write((uint8_t*)&wConfig, sizeof(wConfig)); f.close(); } }
void cargarWiFiConfig() { if(LittleFS.exists("/wifi.bin")){ File f = LittleFS.open("/wifi.bin", "r"); if(f){ f.read((uint8_t*)&wConfig, sizeof(wConfig)); f.close(); } } }
void borrarWiFiConfig() { LittleFS.remove("/wifi.bin"); ESP.restart(); }

// Lee la MAC del ESP32 directamente desde el eFuse de hardware.
// Funciona siempre, independientemente del estado del WiFi.
String obtenerMACAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void guardarEstadoRecuperacion() {
  File f = LittleFS.open("/recovery.bin", "w");
  if (f) { RecoveryData d = {(int)estadoActual, etapaActualIndex, tiempoInicioEtapa}; f.write((uint8_t*)&d, sizeof(d)); f.close(); }
}

void cargarEstadoRecuperacion() {
  if (LittleFS.exists("/recovery.bin")) {
    File f = LittleFS.open("/recovery.bin", "r");
    if (f) { RecoveryData d; f.read((uint8_t*)&d, sizeof(d)); f.close(); 
      if (d.estadoActual != STAND_BY) { estadoActual = (EstadoHorno)d.estadoActual; etapaActualIndex = d.etapaActualIndex; tiempoInicioEtapa = d.tiempoInicioEtapa; }
    }
    LittleFS.remove("/recovery.bin");
  }
}
