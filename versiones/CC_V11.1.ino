// =================================================================
// ==      CONTROLADOR DIGITAL PARA HORNO DE CERÁMICA V11.1       ==
// =================================================================
//      (WiFi, Servidor Web, Sonidos de Menú y Status Bar)
//

#include <SPI.h>
#include <TFT_eSPI.h> 
#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#include <LittleFS.h>
using namespace fs;
#include <WiFi.h>
#include <WebServer.h>

// --- WiFi CREDENTIALS ---
const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";
WebServer server(80);
unsigned long lastWiFiCheck = 0;
bool wifiConectado = false;

// --- PINOUT ---
#define RELAY_PIN 17
#define BUZZER_PIN 27
#define BTN_UP_PIN    26
#define BTN_DOWN_PIN  25
#define BTN_OK_PIN    33
#define BTN_EXIT_PIN  32
#define TFT_LED       12 

// ...

bool sonidoHabilitado = true;

void suenaBuzzer(int duracion, int veces = 1) {
  for(int i=0; i<veces; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duracion);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < veces - 1) delay(100);
  }
}

void suenaClick() {
  if (sonidoHabilitado) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(20); 
    digitalWrite(BUZZER_PIN, LOW);
  }
}
#define MAXCS   5
Adafruit_MAX31855 thermocouple(MAXCS);

// --- ESTRUCTURAS ---
#define MAX_ETAPAS 10
#define MAX_LARGO_NOMBRE 16
#define MAX_PROGRAMAS 5
#define STATUS_BAR_HEIGHT 30
#define GRAPH_POINTS 120 // 10 min (puntos cada 5s)

struct Etapa { int rampa, temperatura, tiempo; };
struct Programa { char nombre[MAX_LARGO_NOMBRE]; Etapa etapas[MAX_ETAPAS]; int numEtapas; };

TFT_eSPI tft = TFT_eSPI();

// --- VARIABLES DE ESTADO ---
struct RecoveryData {
  int estadoActual;
  int etapaActualIndex;
  unsigned long tiempoInicioEtapa;
};

Programa programas[MAX_PROGRAMAS];
int numProgramasGuardados = 0;
int programaActivoIndex = 0;
float currentTemperature = 25.0;
float prevTemperature = 0.0;
float temperaturaCrudaSimulada = 27.2; 
float calibracionOffset = 0.0;

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
const long intervaloDebounce = 200;
bool necesitaRefresco = true;
bool parpadeoEstado = false;
unsigned long ultimoParpadeo = 0;

const char* menuPrincipalItems[] = {"Iniciar Programa", "Sel. Programa", "Ajustes"};
const char* menuAjustesItems[] = {"Info Sistema", "Calibracion", "Ajustes PID", "Brillo", "Sonido"};
int brilloPantalla = 255;
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
void manejarRaizWeb();
void cargarProgramaDePrueba();
void guardarConfiguracion();
void cargarConfiguracion();
void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_EXIT_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(BUZZER_PIN, LOW);
  
  if(!LittleFS.begin(true)) Serial.println("LittleFS error");

  // Brillo Pantalla (PWM)
  ledcAttach(TFT_LED, 1000, 8); // 1kHz, 8 bits res
  cargarConfiguracion();
  ledcWrite(TFT_LED, brilloPantalla);
  
  tft.init(); tft.setRotation(3);
  dibujarPantallaBienvenida();
  if (!thermocouple.begin()) Serial.println("MAX31855 error");
  cargarEstadoRecuperacion(); 

  hornoPID.SetOutputLimits(0, windowSize);
  hornoPID.SetMode(AUTOMATIC);
  for(int i=0; i<GRAPH_POINTS; i++) tempHistory[i] = 0;

  tft.fillScreen(TFT_BLACK);
  necesitaRefresco = true;
  actualizarPantalla();

  suenaBuzzer(100, 2);
  
  iniciarWiFi();
  server.on("/", manejarRaizWeb);
  server.begin();
}

void loop() {
  server.handleClient();
  
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiConectado) { Serial.println("WiFi desconectado. Reconectando..."); wifiConectado = false; WiFi.begin(ssid, password); }
    } else {
      if (!wifiConectado) { Serial.print("WiFi Conectado! IP: "); Serial.println(WiFi.localIP()); wifiConectado = true; }
    }
  }

  leerSensores();
  manejarPulsadores();
  ejecutarCicloDeHorneado();
  
  if (millis() - ultimoParpadeo > 400) {
    ultimoParpadeo = millis(); parpadeoEstado = !parpadeoEstado;
    if (estadoActual == EDITANDO_NOMBRE || enModoEdicionHorizontal) necesitaRefresco = true;
  }
  
  if ((estadoActual == CALENTANDO || estadoActual == MANTENIENDO) && (millis() - ultimoPuntoGrafo > 5000)) {
    ultimoPuntoGrafo = millis();
    tempHistory[graphIdx] = currentTemperature;
    graphIdx = (graphIdx + 1) % GRAPH_POINTS;
    necesitaRefresco = true;
  }
  
  actualizarPantalla();
}

void cargarProgramaDePrueba() { strcpy(programas[0].nombre, "CERAMICA 1"); programas[0].numEtapas = 2; programas[0].etapas[0] = {300, 400, 10}; programas[0].etapas[1] = {200, 600, 5}; numProgramasGuardados = 1; }

void iniciarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi: "); Serial.println(ssid);
  // No esperamos indefinidamente para no bloquear el inicio del horno.
  // El loop se encargará de reintentar si no conecta.
}

void manejarRaizWeb() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Controlador Horno by DAC LAB</title><style>";
  html += "body { font-family: Arial; text-align: center; background-color: #111; color: white; }";
  html += ".box { border: 2px solid cyan; padding: 20px; display: inline-block; border-radius: 10px; margin-top: 50px; }";
  html += ".temp { font-size: 3em; color: yellow; }";
  html += ".state { font-size: 1.5em; color: #00ff00; text-transform: uppercase; }";
  html += "</style><meta http-equiv='refresh' content='5'></head><body>";
  html += "<div class='box'><h1>DAC LAB - OVEN CONTROL</h1>";
  html += "<div class='state'>" + String(getEstadoStr(estadoActual)) + "</div>";
  html += "<div class='temp'>" + String(currentTemperature, 1) + " &deg;C</div>";
  if (estadoActual == CALENTANDO || estadoActual == MANTENIENDO) {
    html += "<p>Etapa: " + String(etapaActualIndex + 1) + " de " + String(programas[programaActivoIndex].numEtapas) + "</p>";
    html += "<p>Setpoint: " + String(pidSetpoint, 1) + " &deg;C</p>";
  }
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void guardarConfiguracion() {
  fs::File f = LittleFS.open("/config.bin", "w");
  if (f) {
    f.write((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.write((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.write((uint8_t*)programas, sizeof(programas));
    f.write((uint8_t*)&Kp, sizeof(Kp));
    f.write((uint8_t*)&Ki, sizeof(Ki));
    f.write((uint8_t*)&Kd, sizeof(Kd));
    f.write((uint8_t*)&brilloPantalla, sizeof(brilloPantalla));
    f.write((uint8_t*)&sonidoHabilitado, sizeof(sonidoHabilitado));
    f.close();
  }
}

void cargarConfiguracion() {
  if (!LittleFS.exists("/config.bin")) { cargarProgramaDePrueba(); guardarConfiguracion(); return; }
  fs::File f = LittleFS.open("/config.bin", "r");
  if (f) {
    f.read((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.read((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.read((uint8_t*)programas, sizeof(programas));
    if (f.available() >= sizeof(double)*3) {
      f.read((uint8_t*)&Kp, sizeof(Kp));
      f.read((uint8_t*)&Ki, sizeof(Ki));
      f.read((uint8_t*)&Kd, sizeof(Kd));
      hornoPID.SetTunings(Kp, Ki, Kd);
    }
    if (f.available() >= sizeof(int)) {
      f.read((uint8_t*)&brilloPantalla, sizeof(brilloPantalla));
    }
    if (f.available() >= sizeof(bool)) {
      f.read((uint8_t*)&sonidoHabilitado, sizeof(sonidoHabilitado));
    }
    f.close();
  }
}

void leerSensores() {
  double c = thermocouple.readCelsius();
  if (isnan(c)) {
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
  if (millis() - ultimoPulsoBoton < intervaloDebounce) return;
  bool u = digitalRead(BTN_UP_PIN) == LOW, d = digitalRead(BTN_DOWN_PIN) == LOW;
  bool o = digitalRead(BTN_OK_PIN) == LOW, e = digitalRead(BTN_EXIT_PIN) == LOW;
  if (u || d || o || e) { ultimoPulsoBoton = millis(); procesarEntrada(u, d, o, e); necesitaRefresco = true; }
}

void procesarEntrada(bool subiendo, bool bajando, bool ok, bool exit) {
    if (subiendo || bajando || ok || exit) suenaClick();
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
           estadoActual = CALENTANDO;
        }
        if (exit) estadoActual = MENU_PRINCIPAL;
        break;
      case MENU_AJUSTES:
        if (subiendo) menuAjustesCursor = (menuAjustesCursor - 1 + 5) % 5;
        if (bajando) menuAjustesCursor = (menuAjustesCursor + 1) % 5;
        if (exit) estadoActual = MENU_PRINCIPAL;
        if (ok) {
          if (menuAjustesCursor == 0) estadoActual = INFO_WIFI;
          if (menuAjustesCursor == 1) { valorCalibracionEditado = currentTemperature; estadoActual = MENU_CALIBRACION; }
          if (menuAjustesCursor == 2) { menuPIDCursor = 0; estadoActual = MENU_AJUSTES_PID; }
          if (menuAjustesCursor == 3) { estadoActual = MENU_AJUSTES_BRILLO; }
          if (menuAjustesCursor == 4) { sonidoHabilitado = !sonidoHabilitado; guardarConfiguracion(); }
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
           if (!isnan(currentTemperature)) {
               calibracionOffset = valorCalibracionEditado - currentTemperature; 
               guardarConfiguracion(); 
           }
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
            if (configProgCursorHorizontal == 0) programas[programaEnEdicionIndex].etapas[etIdx].rampa += (subiendo ? inc : -inc);
            else if (configProgCursorHorizontal == 1) programas[programaEnEdicionIndex].etapas[etIdx].temperatura += (subiendo ? inc : -inc);
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
              if (programas[programaEnEdicionIndex].numEtapas < MAX_ETAPAS) { int i = programas[programaEnEdicionIndex].numEtapas; programas[programaEnEdicionIndex].etapas[i] = {100, 100, 0}; programas[programaEnEdicionIndex].numEtapas++; guardarConfiguracion(); }
            }
            else if (configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 2) {
              if (programas[programaEnEdicionIndex].numEtapas > 1) { programas[programaEnEdicionIndex].numEtapas--; guardarConfiguracion(); if (configProgCursorVertical > programas[programaEnEdicionIndex].numEtapas) configProgCursorVertical = programas[programaEnEdicionIndex].numEtapas; }
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
      case FALLO: if (ok || exit) estadoActual = STAND_BY; break;
      default: if (exit) estadoActual = STAND_BY; break;
    }
}

void ejecutarCicloDeHorneado() {
  if (estadoActual != CALENTANDO && estadoActual != MANTENIENDO) { digitalWrite(RELAY_PIN, HIGH); return; }
  if (millis() - ultimoCheckWatchdog > INTERVALO_WATCHDOG) {
    if (digitalRead(RELAY_PIN) == LOW && (currentTemperature < tempEnUltimoCheck + 2.0)) {
       errorMsgStr = "SIN AUMENTO DE TEMP: REVISAR RESISTENCIA"; estadoActual = FALLO; digitalWrite(RELAY_PIN, HIGH); suenaBuzzer(500, 3); return;
    }
    ultimoCheckWatchdog = millis(); tempEnUltimoCheck = currentTemperature;
  }
  pidInput = currentTemperature;
  Etapa etapa = programas[programaActivoIndex].etapas[etapaActualIndex];
  unsigned long tSim = (millis() - tiempoInicioEtapa) * SIM_MULTIPLIER;
  if (estadoActual == CALENTANDO) {
    pidSetpoint = tempInicioEtapa + (etapa.rampa * (tSim / 3600000.0));
    if (pidSetpoint > etapa.temperatura) pidSetpoint = etapa.temperatura;
    if (currentTemperature >= etapa.temperatura) { estadoActual = MANTENIENDO; tiempoInicioEtapa = millis(); pidSetpoint = etapa.temperatura; guardarEstadoRecuperacion(); }
  } else if (estadoActual == MANTENIENDO) {
    if ((millis() - tiempoInicioEtapa) >= ((unsigned long)etapa.tiempo * 60000 / SIM_MULTIPLIER)) {
      etapaActualIndex++;
      if (etapaActualIndex >= programas[programaActivoIndex].numEtapas) { estadoActual = ENFRIANDO; LittleFS.remove("/recovery.bin"); suenaBuzzer(1000, 1); }
      else { estadoActual = CALENTANDO; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature; guardarEstadoRecuperacion(); }
    }
  }
  hornoPID.Compute();
  if (millis() - windowStartTime > windowSize) windowStartTime += windowSize;
  digitalWrite(RELAY_PIN, (pidOutput > (millis() - windowStartTime)) ? LOW : HIGH);
}

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
  } else {
    actualizarTemperaturaEnBarra();
    if (necesitaRefresco) {
        if (estadoActual == STAND_BY) dibujarPantallaStandBy(false);
        else if (estadoActual == MENU_PRINCIPAL) dibujarMenuPrincipal(false);
        else if (estadoActual == MENU_AJUSTES) dibujarMenuAjustes(false);
        else if (estadoActual == MENU_AJUSTES_PID) dibujarMenuPID(false);
        else if (estadoActual == MENU_AJUSTES_BRILLO) dibujarMenuBrillo();
        else if (estadoActual == MENU_SELECCION_PROG) dibujarMenuSeleccionProg(false);
        else if (estadoActual == MENU_CONFIG_PROG || estadoActual == EDITANDO_NOMBRE) dibujarMenuConfigProg(false);
        else if (estadoActual == CALENTANDO || estadoActual == MANTENIENDO) dibujarPantallaCalentando(false);
    }
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
    case MENU_CALIBRACION: return "CALIB.";
    case EDITANDO_NOMBRE: return "NOMBRE";
    case CONFIRMACION_INICIO: return "INICIO?";
    case INFO_WIFI: return "INFO";
    case MENU_AJUSTES_PID: return "PID";
    default: return "";
  }
}
void actualizarTemperaturaEnBarra() {
  static float lastPrintedTemp = -999.0;
  static EstadoHorno lastPrintedEstado = (EstadoHorno)-1;
  if (abs(currentTemperature - lastPrintedTemp) < 0.5 && estadoActual == lastPrintedEstado) return;
  lastPrintedTemp = currentTemperature; lastPrintedEstado = estadoActual;
  tft.fillRect(0, 0, tft.width(), STATUS_BAR_HEIGHT, TFT_DARKGREY); 
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); 
  tft.setCursor(5, 7); tft.print(currentTemperature, 0); tft.print("c"); 
  tft.setCursor(100, 7); tft.setTextColor(TFT_YELLOW); tft.print(getEstadoStr(estadoActual));
}
void dibujarPantallaStandBy(bool r) {
  if (r) { tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(50, 60); tft.print("Temp. Actual:"); tft.setCursor(50, 150); tft.print("Programa Listo:"); tft.setTextColor(TFT_CYAN); tft.setCursor(60, 180); tft.println(programas[programaActivoIndex].nombre); }
  tft.fillRect(80, 90, 100, 40, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(80, 90); tft.print(currentTemperature, 0); tft.print("c");
}
void dibujarItemMenu(int i, bool s, const char* t, int y) { 
  int yp = y + (i * 35); 
  tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); 
  tft.setTextColor(s ? TFT_BLACK : TFT_WHITE); tft.setTextSize(2); 
  tft.setCursor(10, yp + 7); 
  tft.print(t);
  if (strcmp(t, "Sonido") == 0) {
    tft.setCursor(200, yp + 7);
    tft.print(sonidoHabilitado ? ": SI" : ": NO");
  }
}
void dibujarMenuPrincipal(bool r) { for (int i = 0; i < 3; i++) dibujarItemMenu(i, i == menuPrincipalCursor, menuPrincipalItems[i], 90); }
void dibujarMenuAjustes(bool r) { for (int i = 0; i < 5; i++) dibujarItemMenu(i, i == menuAjustesCursor, menuAjustesItems[i], 90); }
void dibujarItemMenuSeleccion(int i, bool s) { int yp = 40 + (i * 35); tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); tft.setTextColor(s ? TFT_BLACK : (i < numProgramasGuardados ? TFT_WHITE : TFT_GREEN)); tft.setCursor(10, yp + 7); tft.println(i < numProgramasGuardados ? programas[i].nombre : "+ Crear Programa"); }
void dibujarMenuSeleccionProg(bool r) { for (int i = 0; i <= numProgramasGuardados; i++) dibujarItemMenuSeleccion(i, i == menuSeleccionProgCursor); }
void dibujarMenuConfigProg(bool r) { 
  tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK); 
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
    tft.setCursor(10, yp + 5); tft.setTextSize(3); 
    if (estadoActual == EDITANDO_NOMBRE) { for(int i=0; i<strlen(programas[programaEnEdicionIndex].nombre); i++) { if (i == charIndexEdicion && parpadeoEstado) tft.setTextColor(TFT_WHITE, TFT_BLACK); else tft.setTextColor(TFT_BLACK, TFT_CYAN); tft.print(programas[programaEnEdicionIndex].nombre[i]); } } else tft.print(programas[programaEnEdicionIndex].nombre);
  } else {
    uint16_t bg = (sel && !enModoEdicionHorizontal) ? TFT_CYAN : TFT_BLACK, tx = (sel && !enModoEdicionHorizontal) ? TFT_BLACK : TFT_WHITE;
    tft.fillRect(0, yp, tft.width(), 22, bg); tft.setTextColor(tx, bg); tft.setTextSize(2); tft.setCursor(5, yp+3); tft.print("E"); tft.print(idx+1);
    Etapa e = programas[programaEnEdicionIndex].etapas[idx];
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 0) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, TFT_BLACK); else tft.setTextColor(tx, bg);
    tft.setCursor(50, yp+3); tft.print("R:"); tft.print(e.rampa);
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 1) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, TFT_BLACK); else tft.setTextColor(tx, bg);
    tft.setCursor(130, yp+3); tft.print("T:"); tft.print(e.temperatura);
    if (sel && enModoEdicionHorizontal && configProgCursorHorizontal == 2) tft.setTextColor(parpadeoEstado ? TFT_YELLOW : TFT_WHITE, TFT_BLACK); else tft.setTextColor(tx, bg);
    tft.setCursor(220, yp+3); tft.print("m:"); tft.print(e.tiempo);
  }
}
void dibujarPantallaConfirmacion() {
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(20, 50); tft.print("INICIAR HORNEADO?");
  tft.setTextColor(TFT_WHITE); tft.setCursor(20, 90); tft.print("Prog: "); tft.print(programas[programaActivoIndex].nombre);
  tft.setCursor(20, 120); tft.print("Etapas: "); tft.print(programas[programaActivoIndex].numEtapas);
  tft.setCursor(20, 150); tft.print("T. Final: "); tft.print(programas[programaActivoIndex].etapas[programas[programaActivoIndex].numEtapas-1].temperatura); tft.print("c");
  tft.setTextColor(TFT_GREEN); tft.setCursor(50, 200); tft.print("OK: INICIAR  EXIT: NO");
}
void dibujarPantallaInfoWiFi() {
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2); tft.setCursor(20, 40); tft.print("INFO SISTEMA");
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);

  tft.setCursor(20, 75); tft.print("IP: "); 
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN); tft.print(WiFi.localIP());
  } else {
    tft.setTextColor(TFT_RED); tft.print("DESC.");
  }

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 110); tft.print("MAC: "); tft.print(WiFi.macAddress());
  tft.setCursor(20, 140);
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  float pct = (total > 0) ? ((float)used / total * 100.0) : 0;
  tft.print("LittleFS: "); tft.print(pct, 1); tft.print("%");
  tft.setCursor(20, 170); tft.print("Progs: "); tft.print(numProgramasGuardados); tft.print("/"); tft.print(MAX_PROGRAMAS);
  tft.setTextColor(TFT_YELLOW); tft.setCursor(20, 210); tft.print("Presione OK para volver");
}

void dibujarMenuPID(bool r) {
  if (r) tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2); tft.setCursor(20, 50); tft.print("AJUSTES PID");
  
  auto drawP = [&](int i, const char* label, double val, int y) {
    bool sel = (menuPIDCursor == i);
    tft.fillRect(10, y-5, 300, 30, sel ? TFT_WHITE : TFT_BLACK);
    tft.setTextColor(sel ? TFT_BLACK : TFT_WHITE);
    tft.setCursor(20, y); tft.print(label);
    tft.setCursor(180, y); tft.print(val, (i == 1) ? 2 : 1);
  };
  
  drawP(0, "Kp:", Kp, 90);
  drawP(1, "Ki:", Ki, 130);
  drawP(2, "Kd:", Kd, 170);
  
  bool selExit = (menuPIDCursor == 3);
  tft.fillRect(10, 205, 300, 30, selExit ? TFT_WHITE : TFT_BLACK);
  tft.setTextColor(selExit ? TFT_BLACK : TFT_YELLOW);
  tft.setCursor(20, 210); tft.print("GUARDAR Y VOLVER");
}
void dibujarMenuBrillo() {
  tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2); tft.setCursor(20, 50); tft.print("BRILLO PANTALLA");
  
  int x0 = 40, y0 = 120, w = 240, h = 30;
  tft.drawRect(x0, y0, w, h, TFT_WHITE);
  int fillW = map(brilloPantalla, 5, 255, 0, w-4);
  tft.fillRect(x0+2, y0+2, fillW, h-4, TFT_YELLOW);
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 180); tft.print("UP/DN: Ajustar");
  tft.setCursor(20, 210); tft.print("OK: Guardar y Salir");
}
void dibujarPantallaCalentando(bool r) {
  if (r) { tft.setTextColor(TFT_ORANGE); tft.setTextSize(2); tft.setCursor(20, 45); tft.print(estadoActual == CALENTANDO ? "CALENTANDO" : "MANTENIENDO"); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(20, 85); tft.print("Meta:"); }
  tft.fillRect(150, 45, 120, 30, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(3); tft.setCursor(180, 45); tft.print(currentTemperature, 0); tft.print("c");
  tft.setTextColor(TFT_GREEN); tft.setTextSize(2); tft.setCursor(90, 85); tft.print(programas[programaActivoIndex].etapas[etapaActualIndex].temperatura); tft.print("c");
  dibujarGrafica();
}
void dibujarPantallaBienvenida() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_ORANGE);
  tft.setTextSize(3);
  tft.setCursor(40, 50);
  tft.print("Controlador by");
  tft.setTextSize(4);
  tft.setCursor(60, 90);
  tft.print("DAC LAB");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(80, 150);
  tft.print("Iniciando...");
  
  // Animación de barra
  int x0 = 60, y0 = 190, w = 200, h = 15;
  tft.drawRect(x0, y0, w, h, TFT_WHITE);
  for(int i=0; i<w-4; i++) {
    tft.fillRect(x0+2, y0+2, i, h-4, TFT_ORANGE);
    delay(10); 
  }
  delay(500);
}
void dibujarGrafica() {
  int x0 = 35, y0 = 230, w = 275, h = 80;
  tft.drawRect(x0, y0-h, w, h, TFT_DARKGREY);
  
  // Encontrar Máximo para escalado
  float maxT = 100.0;
  for(int i=0; i<GRAPH_POINTS; i++) if(tempHistory[i] > maxT) maxT = tempHistory[i];
  maxT *= 1.1; // Margen del 10%
  
  // Etiquetas Y
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY);
  tft.setCursor(5, y0-h); tft.print((int)maxT);
  tft.setCursor(5, y0-10); tft.print("0");
  
  // Etiquetas X
  tft.setCursor(x0, y0+5); tft.print("-10m");
  tft.setCursor(x0+w-30, y0+5); tft.print("Ahora");

  for(int i=0; i<GRAPH_POINTS-1; i++) {
    int p1 = (graphIdx + i) % GRAPH_POINTS;
    int p2 = (graphIdx + i + 1) % GRAPH_POINTS;
    
    if (tempHistory[p1] > 0 && tempHistory[p2] > 0) {
      int x1 = x0 + (i * w / GRAPH_POINTS);
      int x2 = x0 + ((i+1) * w / GRAPH_POINTS);
      int y1 = y0 - (tempHistory[p1] * h / maxT);
      int y2 = y0 - (tempHistory[p2] * h / maxT);
      tft.drawLine(x1, y1, x2, y2, TFT_CYAN);
    }
  }
}
void dibujarPantallaEnfriando(bool r) { tft.setTextColor(TFT_GREEN); tft.setTextSize(3); tft.setCursor(80, 70); tft.print("FINALIZADO"); }
void dibujarPantallaCalibracion() { tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(20, 100); tft.print("Lectura Cruda:"); tft.setCursor(200, 100); tft.setTextColor(TFT_YELLOW); tft.print(temperaturaCrudaSimulada, 1); tft.setTextColor(TFT_WHITE); tft.setCursor(20, 140); tft.print("Temp. Real:"); tft.fillRect(190, 138, 80, 20, TFT_CYAN); tft.setTextColor(TFT_BLACK); tft.setCursor(200, 140); tft.print(valorCalibracionEditado, 1); }
void dibujarPantallaFallo(const char* msg) { tft.fillScreen(TFT_RED); tft.setTextColor(TFT_WHITE); tft.setTextSize(3); tft.setCursor(40, 50); tft.print("FALLO!"); tft.setTextSize(2); tft.setCursor(10, 100); tft.print(msg); tft.setCursor(10, 180); tft.print("Presione OK para reset"); }
void guardarEstadoRecuperacion() {
  fs::File f = LittleFS.open("/recovery.bin", "w");
  if (f) {
    RecoveryData data = {(int)estadoActual, etapaActualIndex, tiempoInicioEtapa};
    f.write((uint8_t*)&data, sizeof(data));
    f.close();
  }
}

void cargarEstadoRecuperacion() {
  if (LittleFS.exists("/recovery.bin")) {
    fs::File f = LittleFS.open("/recovery.bin", "r");
    if (f) {
      RecoveryData data;
      f.read((uint8_t*)&data, sizeof(data));
      f.close();
      if (data.estadoActual != STAND_BY) {
        estadoActual = (EstadoHorno)data.estadoActual;
        etapaActualIndex = data.etapaActualIndex;
        tiempoInicioEtapa = data.tiempoInicioEtapa;
      }
      LittleFS.remove("/recovery.bin"); 
    }
  }
}
