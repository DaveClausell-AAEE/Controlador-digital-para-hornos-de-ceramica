// =================================================================
// ==      CONTROLADOR DIGITAL PARA HORNO DE CERÁMICA V10.5       ==
// =================================================================
//      (Edición de Nombres, Gestión de Etapas y Watchdog Térmico)
//

#include <SPI.h>
#include <TFT_eSPI.h> 
#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#include <LittleFS.h>

// --- PINOUT ---
#define RELAY_PIN 17
#define BTN_UP_PIN    26
#define BTN_DOWN_PIN  25
#define BTN_OK_PIN    33
#define BTN_EXIT_PIN  32
#define TFT_LED       27 

// Pinout MAX31855
#define MAXCS   5
Adafruit_MAX31855 thermocouple(MAXCS);

// --- ESTRUCTURAS ---
#define MAX_ETAPAS 10
#define MAX_LARGO_NOMBRE 16
#define MAX_PROGRAMAS 5
#define STATUS_BAR_HEIGHT 30

struct Etapa { int rampa, temperatura, tiempo; };
struct Programa { char nombre[MAX_LARGO_NOMBRE]; Etapa etapas[MAX_ETAPAS]; int numEtapas; };

TFT_eSPI tft = TFT_eSPI();

// --- VARIABLES DE ESTADO ---
Programa programas[MAX_PROGRAMAS];
int numProgramasGuardados = 0;
int programaActivoIndex = 0;
float currentTemperature = 25.0;
float prevTemperature = 0.0;
float temperaturaCrudaSimulada = 27.2; 
float calibracionOffset = 0.0;

enum EstadoHorno {
  STAND_BY, CALENTANDO, MANTENIENDO, ENFRIANDO, FALLO, 
  MENU_PRINCIPAL, MENU_SELECCION_PROG, MENU_CONFIG_PROG,
  MENU_AJUSTES, MENU_CALIBRACION, EDITANDO_NOMBRE
};
EstadoHorno estadoActual = STAND_BY;
EstadoHorno estadoPrevio = STAND_BY;

bool enModoEdicionHorizontal = false;
int menuPrincipalCursor = 0;
int menuAjustesCursor = 0;
int menuSeleccionProgCursor = 0;
int programaEnEdicionIndex = 0;
int configProgCursorVertical = 0;
int configProgCursorHorizontal = 0;
int charIndexEdicion = 0;
float valorCalibracionEditado = 0.0;

// Watchdog Térmico
unsigned long ultimoCheckWatchdog = 0;
float tempEnUltimoCheck = 0;
const unsigned long INTERVALO_WATCHDOG = 600000; // 10 Minutos
const float DELTA_MIN_TEMP = 2.0; 

unsigned long ultimoPulsoBoton = 0;
const long intervaloDebounce = 200;
bool necesitaRefresco = true;
bool parpadeoEstado = false;
unsigned long ultimoParpadeo = 0;

const char* menuPrincipalItems[] = {"Iniciar Programa", "Sel. Programa", "Ajustes"};
int numMenuPrincipalItems = 3;
const char* menuAjustesItems[] = {"WiFi", "Calibracion"};
int numMenuAjustesItems = 2;

// --- PID ---
double pidSetpoint, pidInput, pidOutput;
double Kp=10, Ki=0.2, Kd=1; 
PID hornoPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);
unsigned long windowSize = 5000;
unsigned long windowStartTime;
int etapaActualIndex = 0;
unsigned long tiempoInicioEtapa = 0;
float tempInicioEtapa = 0;
unsigned long ultimoUpdateTempSimulada = 0;

const int SIM_MULTIPLIER = 60; 

// --- PROTOTIPOS ---
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
void dibujarPantallaFallo();
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
  digitalWrite(RELAY_PIN, HIGH); 
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); 

  tft.init();
  tft.setRotation(3);
  
  if (!thermocouple.begin()) Serial.println("MAX31855 error");

  if(!LittleFS.begin(true)) Serial.println("LittleFS error");
  cargarConfiguracion();
  
  hornoPID.SetOutputLimits(0, windowSize);
  hornoPID.SetMode(AUTOMATIC);
  
  tft.fillScreen(TFT_BLACK);
  necesitaRefresco = true;
  actualizarPantalla();
}

void loop() {
  leerSensores();
  manejarPulsadores();
  ejecutarCicloDeHorneado();
  
  // Manejo de parpadeo del cursor
  if (millis() - ultimoParpadeo > 400) {
    ultimoParpadeo = millis();
    parpadeoEstado = !parpadeoEstado;
    if (estadoActual == EDITANDO_NOMBRE || enModoEdicionHorizontal) necesitaRefresco = true;
  }
  
  actualizarPantalla();
}

void guardarConfiguracion() {
  File f = LittleFS.open("/config.bin", "w");
  if (f) {
    f.write((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.write((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.write((uint8_t*)programas, sizeof(programas));
    f.close();
  }
}

void cargarConfiguracion() {
  if (!LittleFS.exists("/config.bin")) {
    cargarProgramaDePrueba();
    guardarConfiguracion();
    return;
  }
  File f = LittleFS.open("/config.bin", "r");
  if (f) {
    f.read((uint8_t*)&calibracionOffset, sizeof(calibracionOffset));
    f.read((uint8_t*)&numProgramasGuardados, sizeof(numProgramasGuardados));
    f.read((uint8_t*)programas, sizeof(programas));
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
    if (abs(currentTemperature - prevTemperature) >= 0.5) {
      prevTemperature = currentTemperature;
      necesitaRefresco = true;
    }
  }
}

void manejarPulsadores() {
  if (millis() - ultimoPulsoBoton < intervaloDebounce) return;
  bool u = digitalRead(BTN_UP_PIN) == LOW, d = digitalRead(BTN_DOWN_PIN) == LOW;
  bool o = digitalRead(BTN_OK_PIN) == LOW, e = digitalRead(BTN_EXIT_PIN) == LOW;
  if (u || d || o || e) {
    ultimoPulsoBoton = millis();
    procesarEntrada(u, d, o, e);
    necesitaRefresco = true;
  }
}

void procesarEntrada(bool subiendo, bool bajando, bool ok, bool exit) {
    estadoPrevio = estadoActual;
    switch (estadoActual) {
      case STAND_BY: if (ok) estadoActual = MENU_PRINCIPAL; break;
      case MENU_PRINCIPAL:
        if (subiendo) menuPrincipalCursor = (menuPrincipalCursor - 1 + numMenuPrincipalItems) % numMenuPrincipalItems;
        if (bajando) menuPrincipalCursor = (menuPrincipalCursor + 1) % numMenuPrincipalItems;
        if (exit) estadoActual = STAND_BY;
        if (ok) {
          if (menuPrincipalCursor == 0) {
             etapaActualIndex = 0; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature;
             pidSetpoint = currentTemperature; windowStartTime = millis();
             ultimoCheckWatchdog = millis(); tempEnUltimoCheck = currentTemperature;
             estadoActual = CALENTANDO;
          }
          if (menuPrincipalCursor == 1) estadoActual = MENU_SELECCION_PROG;
          if (menuPrincipalCursor == 2) { estadoActual = MENU_AJUSTES; menuAjustesCursor = 0; }
        }
        break;
      case MENU_AJUSTES:
        if (subiendo) menuAjustesCursor = (menuAjustesCursor - 1 + numMenuAjustesItems) % numMenuAjustesItems;
        if (bajando) menuAjustesCursor = (menuAjustesCursor + 1) % numMenuAjustesItems;
        if (exit) estadoActual = MENU_PRINCIPAL;
        if (ok && menuAjustesCursor == 1) { valorCalibracionEditado = currentTemperature; estadoActual = MENU_CALIBRACION; }
        break;
      case MENU_CALIBRACION:
        if (subiendo) valorCalibracionEditado += 0.1;
        if (bajando) valorCalibracionEditado -= 0.1;
        if (exit) estadoActual = MENU_AJUSTES;
        if (ok) { calibracionOffset = temperaturaCrudaSimulada - valorCalibracionEditado; guardarConfiguracion(); estadoActual = MENU_AJUSTES; }
        break;
      case MENU_SELECCION_PROG:
        if (subiendo) menuSeleccionProgCursor = (menuSeleccionProgCursor - 1 + (numProgramasGuardados + 1)) % (numProgramasGuardados + 1);
        if (bajando) menuSeleccionProgCursor = (menuSeleccionProgCursor + 1) % (numProgramasGuardados + 1);
        if (exit) estadoActual = MENU_PRINCIPAL;
        if (ok) {
            if (menuSeleccionProgCursor < numProgramasGuardados) {
                programaEnEdicionIndex = menuSeleccionProgCursor;
                configProgCursorVertical = 0; enModoEdicionHorizontal = false;
                estadoActual = MENU_CONFIG_PROG;
            } else if (numProgramasGuardados < MAX_PROGRAMAS) {
                int newIdx = numProgramasGuardados;
                sprintf(programas[newIdx].nombre, "NUEVO %d", newIdx+1);
                programas[newIdx].numEtapas = 1;
                programas[newIdx].etapas[0] = {100, 100, 0};
                numProgramasGuardados++;
                guardarConfiguracion();
                programaEnEdicionIndex = newIdx;
                configProgCursorVertical = 0; enModoEdicionHorizontal = false;
                estadoActual = MENU_CONFIG_PROG;
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
          int numOpciones = programas[programaEnEdicionIndex].numEtapas + 3; // Nombre + Etapas + Add + Delete
          if (subiendo) configProgCursorVertical = (configProgCursorVertical - 1 + numOpciones) % numOpciones;
          if (bajando) configProgCursorVertical = (configProgCursorVertical + 1) % numOpciones;
          if (exit) { guardarConfiguracion(); estadoActual = MENU_SELECCION_PROG; }
          if (ok) {
            if (configProgCursorVertical == 0) { charIndexEdicion = 0; estadoActual = EDITANDO_NOMBRE; }
            else if (configProgCursorVertical <= programas[programaEnEdicionIndex].numEtapas) { enModoEdicionHorizontal = true; configProgCursorHorizontal = 0; }
            else if (configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 1) { // Añadir Etapa
              if (programas[programaEnEdicionIndex].numEtapas < MAX_ETAPAS) {
                int idx = programas[programaEnEdicionIndex].numEtapas;
                programas[programaEnEdicionIndex].etapas[idx] = {100, 100, 0};
                programas[programaEnEdicionIndex].numEtapas++;
                guardarConfiguracion();
              }
            }
            else if (configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 2) { // Borrar Etapa
              if (programas[programaEnEdicionIndex].numEtapas > 1) {
                programas[programaEnEdicionIndex].numEtapas--;
                guardarConfiguracion();
                if (configProgCursorVertical > programas[programaEnEdicionIndex].numEtapas) configProgCursorVertical = programas[programaEnEdicionIndex].numEtapas;
              }
            }
          }
        }
        break;
      case EDITANDO_NOMBRE:
        if (subiendo || bajando) {
          char c = programas[programaEnEdicionIndex].nombre[charIndexEdicion];
          if (subiendo) { c++; if (c > 'Z') c = ' '; if (c == ' '+1) c = 'A'; }
          else { c--; if (c < ' ') c = 'Z'; if (c == 'A'-1) c = ' '; }
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
  
  // Watchdog Térmico
  if (millis() - ultimoCheckWatchdog > INTERVALO_WATCHDOG) {
    if (digitalRead(RELAY_PIN) == LOW && (currentTemperature < tempEnUltimoCheck + DELTA_MIN_TEMP)) {
       estadoActual = FALLO; digitalWrite(RELAY_PIN, HIGH); return;
    }
    ultimoCheckWatchdog = millis();
    tempEnUltimoCheck = currentTemperature;
  }

  pidInput = currentTemperature;
  Etapa etapa = programas[programaActivoIndex].etapas[etapaActualIndex];
  unsigned long tSim = (millis() - tiempoInicioEtapa) * SIM_MULTIPLIER;
  if (estadoActual == CALENTANDO) {
    pidSetpoint = tempInicioEtapa + (etapa.rampa * (tSim / 3600000.0));
    if (pidSetpoint > etapa.temperatura) pidSetpoint = etapa.temperatura;
    if (currentTemperature >= etapa.temperatura) { estadoActual = MANTENIENDO; tiempoInicioEtapa = millis(); pidSetpoint = etapa.temperatura; }
  } else if (estadoActual == MANTENIENDO) {
    if ((millis() - tiempoInicioEtapa) >= ((unsigned long)etapa.tiempo * 60000 / SIM_MULTIPLIER)) {
      etapaActualIndex++;
      if (etapaActualIndex >= programas[programaActivoIndex].numEtapas) estadoActual = ENFRIANDO;
      else { estadoActual = CALENTANDO; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature; }
    }
  }
  hornoPID.Compute();
  if (millis() - windowStartTime > windowSize) windowStartTime += windowSize;
  digitalWrite(RELAY_PIN, (pidOutput > (millis() - windowStartTime)) ? LOW : HIGH);
}

void actualizarPantalla() {
  if (!necesitaRefresco) return;
  if (estadoActual != estadoPrevio) {
    tft.fillRect(0, STATUS_BAR_HEIGHT, tft.width(), tft.height() - STATUS_BAR_HEIGHT, TFT_BLACK);
    dibujarBarraDeEstado();
    switch (estadoActual) {
      case STAND_BY: dibujarPantallaStandBy(true); break;
      case MENU_PRINCIPAL: dibujarMenuPrincipal(true); break;
      case MENU_AJUSTES: dibujarMenuAjustes(true); break; 
      case MENU_CALIBRACION: dibujarPantallaCalibracion(); break;
      case MENU_SELECCION_PROG: dibujarMenuSeleccionProg(true); break;
      case MENU_CONFIG_PROG: case EDITANDO_NOMBRE: dibujarMenuConfigProg(true); break;
      case CALENTANDO: case MANTENIENDO: dibujarPantallaCalentando(true); break;
      case ENFRIANDO: dibujarPantallaEnfriando(true); break;
      case FALLO: dibujarPantallaFallo(); break;
    }
  } else {
    actualizarTemperaturaEnBarra();
    if (estadoActual == STAND_BY) dibujarPantallaStandBy(false);
    else if (estadoActual == MENU_PRINCIPAL) dibujarMenuPrincipal(false);
    else if (estadoActual == MENU_AJUSTES) dibujarMenuAjustes(false);
    else if (estadoActual == MENU_CONFIG_PROG || estadoActual == EDITANDO_NOMBRE) dibujarMenuConfigProg(false);
    else if (estadoActual == CALENTANDO || estadoActual == MANTENIENDO) dibujarPantallaCalentando(false);
    else if (estadoActual == MENU_CALIBRACION) dibujarPantallaCalibracion();
  }
  estadoPrevio = estadoActual; necesitaRefresco = false;
}

void dibujarBarraDeEstado() { tft.fillRect(0, 0, tft.width(), STATUS_BAR_HEIGHT, TFT_DARKGREY); actualizarTemperaturaEnBarra(); }
void actualizarTemperaturaEnBarra() { tft.fillRect(0, 0, 100, STATUS_BAR_HEIGHT, TFT_DARKGREY); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(5, 7); tft.print(currentTemperature, 0); tft.print("c"); }
void dibujarPantallaStandBy(bool r) {
  if (r) { tft.setTextColor(TFT_WHITE); tft.setCursor(50, 60); tft.print("Temp. Actual:"); tft.setCursor(50, 150); tft.print("Programa Listo:"); tft.setTextColor(TFT_CYAN); tft.setCursor(60, 180); tft.println(programas[programaActivoIndex].nombre); }
  tft.fillRect(80, 90, 100, 40, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(80, 90); tft.print(currentTemperature, 0); tft.print("c");
}
void dibujarItemMenu(int i, bool s, const char* t, int y) { int yp = y + (i * 35); tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); tft.setTextColor(s ? TFT_BLACK : TFT_WHITE); tft.setTextSize(2); tft.setCursor(10, yp + 7); tft.println(t); }
void dibujarMenuPrincipal(bool r) { for (int i = 0; i < numMenuPrincipalItems; i++) dibujarItemMenu(i, i == menuPrincipalCursor, menuPrincipalItems[i], 90); }
void dibujarMenuAjustes(bool r) { for (int i = 0; i < numMenuAjustesItems; i++) dibujarItemMenu(i, i == menuAjustesCursor, menuAjustesItems[i], 90); }
void dibujarItemMenuSeleccion(int i, bool s) { int yp = 40 + (i * 35); tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK); tft.setTextColor(s ? TFT_BLACK : (i < numProgramasGuardados ? TFT_WHITE : TFT_GREEN)); tft.setCursor(10, yp + 7); tft.println(i < numProgramasGuardados ? programas[i].nombre : "+ Crear Programa"); }
void dibujarMenuSeleccionProg(bool r) { for (int i = 0; i <= numProgramasGuardados; i++) dibujarItemMenuSeleccion(i, i == menuSeleccionProgCursor); }
void dibujarMenuConfigProg(bool r) { if (r) tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK); dibujarLineaDeEtapa(-1); for (int i = 0; i < programas[programaEnEdicionIndex].numEtapas; i++) dibujarLineaDeEtapa(i); 
  // Opciones de gestión al final
  int baseYP = 80 + programas[programaEnEdicionIndex].numEtapas * 25;
  dibujarItemMenu(0, configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 1, "+ Añadir Etapa", baseYP);
  dibujarItemMenu(1, configProgCursorVertical == programas[programaEnEdicionIndex].numEtapas + 2, "- Borrar Etapa", baseYP);
}
void dibujarLineaDeEtapa(int idx) {
  int yp = (idx == -1) ? 40 : 80 + idx * 25;
  bool sel = (idx + 1 == configProgCursorVertical);
  if (idx == -1) { 
    tft.fillRect(0, yp, tft.width(), 30, (configProgCursorVertical == 0) ? TFT_CYAN : TFT_BLACK); 
    tft.setTextColor((configProgCursorVertical == 0) ? (parpadeoEstado && estadoActual == EDITANDO_NOMBRE ? TFT_YELLOW : TFT_BLACK) : TFT_CYAN); 
    tft.setCursor(10, yp + 5); tft.setTextSize(3); 
    if (estadoActual == EDITANDO_NOMBRE) {
      for(int i=0; i<strlen(programas[programaEnEdicionIndex].nombre); i++) {
        if (i == charIndexEdicion && parpadeoEstado) tft.setTextColor(TFT_WHITE, TFT_BLACK);
        else tft.setTextColor(TFT_BLACK, TFT_CYAN);
        tft.print(programas[programaEnEdicionIndex].nombre[i]);
      }
    } else tft.print(programas[programaEnEdicionIndex].nombre);
  } 
  else {
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
void dibujarPantallaCalentando(bool r) {
  if (r) { tft.setTextColor(TFT_ORANGE); tft.setTextSize(3); tft.setCursor(80, 50); tft.print("HORNEANDO"); tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(30, 90); tft.print("Actual:"); tft.setCursor(30, 130); tft.print("Meta:"); }
  tft.fillRect(150, 85, 100, 80, TFT_BLACK); tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(150, 85); tft.print(currentTemperature, 0); tft.print("c");
  tft.setTextColor(TFT_GREEN); tft.setCursor(150, 125); tft.print(programas[programaActivoIndex].etapas[etapaActualIndex].temperatura); tft.print("c");
}
void dibujarPantallaEnfriando(bool r) { tft.setTextColor(TFT_GREEN); tft.setTextSize(3); tft.setCursor(80, 70); tft.print("FINALIZADO"); }
void dibujarPantallaCalibracion() { tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(20, 100); tft.print("Lectura Cruda:"); tft.setCursor(200, 100); tft.setTextColor(TFT_YELLOW); tft.print(temperaturaCrudaSimulada, 1); tft.setTextColor(TFT_WHITE); tft.setCursor(20, 140); tft.print("Temp. Real:"); tft.fillRect(190, 138, 80, 20, TFT_CYAN); tft.setTextColor(TFT_BLACK); tft.setCursor(200, 140); tft.print(valorCalibracionEditado, 1); }
void dibujarPantallaFallo() { tft.fillScreen(TFT_RED); tft.setTextColor(TFT_WHITE); tft.setTextSize(3); tft.setCursor(40, 80); tft.print("ERROR TERMICO"); tft.setTextSize(2); tft.setCursor(20, 140); tft.print("El horno no calienta."); tft.setCursor(20, 180); tft.print("Revise resistencias."); }
void cargarProgramaDePrueba() { strcpy(programas[0].nombre, "CERAMICA 1"); programas[0].numEtapas = 2; programas[0].etapas[0] = {300, 400, 10}; programas[0].etapas[1] = {200, 600, 5}; numProgramasGuardados = 1; }
