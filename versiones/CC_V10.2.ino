// =================================================================
// ==      CONTROLADOR DIGITAL PARA HORNO DE CERÁMICA V10.2       ==
// =================================================================
//      (Versión estable con UI completa y Menú de Calibración)
//

#include <SPI.h>
#include <TFT_eSPI.h> // Asegúrate de configurar User_Setup.h en la librería
#include <PID_v1.h>

// --- PINOUT ---
#define RELAY_PIN 17
#define BTN_UP_PIN    26
#define BTN_DOWN_PIN  25
#define BTN_OK_PIN    33
#define BTN_EXIT_PIN  32
#define TFT_LED       27 

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
float temperaturaCrudaSimulada = 27.2; // Simulación de sensor
float calibracionOffset = 0.0;

enum EstadoHorno {
  STAND_BY, CALENTANDO, MANTENIENDO, ENFRIANDO, FALLO, 
  MENU_PRINCIPAL, MENU_SELECCION_PROG, MENU_CONFIG_PROG,
  MENU_AJUSTES, MENU_CALIBRACION
};
EstadoHorno estadoActual = STAND_BY;
EstadoHorno estadoPrevio = STAND_BY;

bool enModoEdicionHorizontal = false;
int menuPrincipalCursor = 0, prevMenuPrincipalCursor = 0;
int menuAjustesCursor = 0, prevMenuAjustesCursor = 0;
int menuSeleccionProgCursor = 0, prevMenuSeleccionProgCursor = 0;
int programaEnEdicionIndex = 0;
int configProgCursorVertical = 0, prevConfigProgCursorVertical = 0;
int configProgCursorHorizontal = 0, prevConfigProgCursorHorizontal = 0;
int valorEditado = 0;
float valorCalibracionEditado = 0.0;

unsigned long ultimoPulsoBoton = 0;
const long intervaloDebounce = 200;
bool necesitaRefresco = true;

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

const int SIM_MULTIPLIER = 60; // 60x aceleración para pruebas

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
void cargarProgramaDePrueba();

void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_EXIT_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // APAGADO (Active Low)
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); // Encender backlight

  tft.init();
  tft.setRotation(3);
  cargarProgramaDePrueba();
  
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
  actualizarPantalla();
}

void leerSensores() {
  if (millis() - ultimoUpdateTempSimulada > 500) {
    ultimoUpdateTempSimulada = millis();
    if (digitalRead(RELAY_PIN) == LOW) { temperaturaCrudaSimulada += 1.0; }
    else { if (temperaturaCrudaSimulada > 27.2) { temperaturaCrudaSimulada -= 0.2; } }
    necesitaRefresco = true;
  }
  currentTemperature = temperaturaCrudaSimulada - calibracionOffset;
}

void manejarPulsadores() {
  if (millis() - ultimoPulsoBoton < intervaloDebounce) return;
  bool u = digitalRead(BTN_UP_PIN) == LOW;
  bool d = digitalRead(BTN_DOWN_PIN) == LOW;
  bool o = digitalRead(BTN_OK_PIN) == LOW;
  bool e = digitalRead(BTN_EXIT_PIN) == LOW;
  
  if (u || d || o || e) {
    ultimoPulsoBoton = millis();
    procesarEntrada(u, d, o, e);
    necesitaRefresco = true;
  }
}

void procesarEntrada(bool subiendo, bool bajando, bool ok, bool exit) {
    estadoPrevio = estadoActual;
    prevMenuPrincipalCursor = menuPrincipalCursor;
    prevMenuAjustesCursor = menuAjustesCursor;
    prevMenuSeleccionProgCursor = menuSeleccionProgCursor;
    prevConfigProgCursorVertical = configProgCursorVertical;
    prevConfigProgCursorHorizontal = configProgCursorHorizontal;

    switch (estadoActual) {
      case STAND_BY: if (ok) { estadoActual = MENU_PRINCIPAL; } break;
      case MENU_PRINCIPAL:
        if (subiendo) { menuPrincipalCursor = (menuPrincipalCursor - 1 + numMenuPrincipalItems) % numMenuPrincipalItems; }
        if (bajando) { menuPrincipalCursor = (menuPrincipalCursor + 1) % numMenuPrincipalItems; }
        if (exit) { estadoActual = STAND_BY; }
        if (ok) {
          if (menuPrincipalCursor == 0) {
             etapaActualIndex = 0; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature;
             pidSetpoint = currentTemperature; windowStartTime = millis();
             estadoActual = CALENTANDO;
          }
          if (menuPrincipalCursor == 1) { estadoActual = MENU_SELECCION_PROG; }
          if (menuPrincipalCursor == 2) { estadoActual = MENU_AJUSTES; menuAjustesCursor = 0; }
        }
        break;
      case MENU_AJUSTES:
        if (subiendo) { menuAjustesCursor = (menuAjustesCursor - 1 + numMenuAjustesItems) % numMenuAjustesItems; }
        if (bajando) { menuAjustesCursor = (menuAjustesCursor + 1) % numMenuAjustesItems; }
        if (exit) { estadoActual = MENU_PRINCIPAL; }
        if (ok) {
          if (menuAjustesCursor == 1) { valorCalibracionEditado = 0.0; estadoActual = MENU_CALIBRACION; }
        }
        break;
      case MENU_CALIBRACION:
        if (subiendo) { valorCalibracionEditado += 0.1; }
        if (bajando) { valorCalibracionEditado -= 0.1; }
        if (exit) { estadoActual = MENU_AJUSTES; }
        if (ok) { calibracionOffset = temperaturaCrudaSimulada - valorCalibracionEditado; estadoActual = MENU_AJUSTES; }
        break;
      case MENU_SELECCION_PROG:
        if (subiendo) { menuSeleccionProgCursor = (menuSeleccionProgCursor - 1 + (numProgramasGuardados + 1)) % (numProgramasGuardados + 1); }
        if (bajando) { menuSeleccionProgCursor = (menuSeleccionProgCursor + 1) % (numProgramasGuardados + 1); }
        if (exit) { estadoActual = MENU_PRINCIPAL; }
        if (ok && menuSeleccionProgCursor < numProgramasGuardados) {
            programaEnEdicionIndex = menuSeleccionProgCursor;
            configProgCursorVertical = 0; enModoEdicionHorizontal = false;
            estadoActual = MENU_CONFIG_PROG;
        }
        break;
      case MENU_CONFIG_PROG:
        if (enModoEdicionHorizontal) {
          if (ok) { configProgCursorHorizontal = (configProgCursorHorizontal + 1) % 3; }
          if (exit) { enModoEdicionHorizontal = false; }
          if (subiendo || bajando) {
            int etapaIdx = configProgCursorVertical - 1;
            int inc = (configProgCursorHorizontal == 1) ? 5 : ((configProgCursorHorizontal == 0) ? 10 : 1);
            if (configProgCursorHorizontal == 0) { programas[programaEnEdicionIndex].etapas[etapaIdx].rampa += (subiendo ? inc : -inc); }
            else if (configProgCursorHorizontal == 1) { programas[programaEnEdicionIndex].etapas[etapaIdx].temperatura += (subiendo ? inc : -inc); }
            else { programas[programaEnEdicionIndex].etapas[etapaIdx].tiempo += (subiendo ? inc : -inc); }
          }
        } else {
          if (subiendo) { configProgCursorVertical = (configProgCursorVertical - 1 + (programas[programaEnEdicionIndex].numEtapas + 1)) % (programas[programaEnEdicionIndex].numEtapas + 1); }
          if (bajando) { configProgCursorVertical = (configProgCursorVertical + 1) % (programas[programaEnEdicionIndex].numEtapas + 1); }
          if (exit) { estadoActual = MENU_SELECCION_PROG; }
          if (ok && configProgCursorVertical > 0) { enModoEdicionHorizontal = true; configProgCursorHorizontal = 0; }
        }
        break;
      case CALENTANDO: case MANTENIENDO: case ENFRIANDO:
        if (exit) estadoActual = STAND_BY;
        break;
    }
}

void ejecutarCicloDeHorneado() {
  if (estadoActual != CALENTANDO && estadoActual != MANTENIENDO) {
    digitalWrite(RELAY_PIN, HIGH); return;
  }
  pidInput = currentTemperature;
  Programa prog = programas[programaActivoIndex];
  Etapa etapa = prog.etapas[etapaActualIndex];
  unsigned long tSim = (millis() - tiempoInicioEtapa) * SIM_MULTIPLIER;

  if (estadoActual == CALENTANDO) {
    pidSetpoint = tempInicioEtapa + (etapa.rampa * (tSim / 3600000.0));
    if (pidSetpoint > etapa.temperatura) pidSetpoint = etapa.temperatura;
    if (currentTemperature >= etapa.temperatura) { estadoActual = MANTENIENDO; tiempoInicioEtapa = millis(); pidSetpoint = etapa.temperatura; }
  } else if (estadoActual == MANTENIENDO) {
    if ((millis() - tiempoInicioEtapa) >= ((unsigned long)etapa.tiempo * 60000 / SIM_MULTIPLIER)) {
      etapaActualIndex++;
      if (etapaActualIndex >= prog.numEtapas) { estadoActual = ENFRIANDO; }
      else { estadoActual = CALENTANDO; tiempoInicioEtapa = millis(); tempInicioEtapa = currentTemperature; }
    }
  }
  hornoPID.Compute();
  if (millis() - windowStartTime > windowSize) windowStartTime += windowSize;
  digitalWrite(RELAY_PIN, (pidOutput > (millis() - windowStartTime)) ? LOW : HIGH);
}

// --- FUNCIONES DE DIBUJADO ---
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
      case MENU_CONFIG_PROG: dibujarMenuConfigProg(true); break;
      case CALENTANDO: case MANTENIENDO: dibujarPantallaCalentando(true); break;
      case ENFRIANDO: dibujarPantallaEnfriando(true); break;
    }
  } else {
    // Actualizaciones parciales
    if (estadoActual == STAND_BY) dibujarPantallaStandBy(false);
    else if (estadoActual == MENU_PRINCIPAL) dibujarMenuPrincipal(false);
    else if (estadoActual == MENU_AJUSTES) dibujarMenuAjustes(false);
    else if (estadoActual == MENU_CONFIG_PROG) dibujarMenuConfigProg(false);
    else if (estadoActual == CALENTANDO || estadoActual == MANTENIENDO) dibujarPantallaCalentando(false);
    else if (estadoActual == MENU_CALIBRACION) dibujarPantallaCalibracion();
  }
  estadoPrevio = estadoActual;
  necesitaRefresco = false;
}

void dibujarBarraDeEstado() {
  tft.fillRect(0, 0, tft.width(), STATUS_BAR_HEIGHT, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
  tft.setCursor(5, 7); tft.print(currentTemperature, 0); tft.print("c");
}

void dibujarPantallaStandBy(bool r) {
  if (r) {
    tft.setTextColor(TFT_WHITE); tft.setCursor(50, 60); tft.print("Temp. Actual:");
    tft.setCursor(50, 150); tft.print("Programa Listo:");
    tft.setTextColor(TFT_CYAN); tft.setCursor(60, 180); tft.println(programas[programaActivoIndex].nombre);
  }
  tft.fillRect(80, 90, 100, 40, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(80, 90); tft.print(currentTemperature, 0); tft.print("c");
}

void dibujarItemMenu(int i, bool s, const char* t, int y) {
  int yp = y + (i * 35);
  tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK);
  tft.setTextColor(s ? TFT_BLACK : TFT_WHITE); tft.setTextSize(2);
  tft.setCursor(10, yp + 7); tft.println(t);
}

void dibujarMenuPrincipal(bool r) {
  for (int i = 0; i < numMenuPrincipalItems; i++) dibujarItemMenu(i, i == menuPrincipalCursor, menuPrincipalItems[i], 90);
}

void dibujarMenuAjustes(bool r) {
  for (int i = 0; i < numMenuAjustesItems; i++) dibujarItemMenu(i, i == menuAjustesCursor, menuAjustesItems[i], 90);
}

void dibujarItemMenuSeleccion(int i, bool s) {
  int yp = 40 + (i * 35);
  tft.fillRect(0, yp, tft.width(), 30, s ? TFT_WHITE : TFT_BLACK);
  tft.setTextColor(s ? TFT_BLACK : (i < numProgramasGuardados ? TFT_WHITE : TFT_GREEN));
  tft.setCursor(10, yp + 7); tft.println(i < numProgramasGuardados ? programas[i].nombre : "+ Crear Programa");
}

void dibujarMenuSeleccionProg(bool r) {
  for (int i = 0; i <= numProgramasGuardados; i++) dibujarItemMenuSeleccion(i, i == menuSeleccionProgCursor);
}

void dibujarMenuConfigProg(bool r) {
  if (r) { tft.fillRect(0, 30, tft.width(), 210, TFT_BLACK); }
  dibujarLineaDeEtapa(-1);
  for (int i = 0; i < programas[programaEnEdicionIndex].numEtapas; i++) dibujarLineaDeEtapa(i);
}

void dibujarLineaDeEtapa(int idx) {
  int yp = (idx == -1) ? 40 : 80 + idx * 25;
  bool sel = (idx + 1 == configProgCursorVertical);
  if (idx == -1) {
    tft.fillRect(0, yp, tft.width(), 30, (configProgCursorVertical == 0) ? TFT_CYAN : TFT_BLACK);
    tft.setTextColor((configProgCursorVertical == 0) ? TFT_BLACK : TFT_CYAN);
    tft.setCursor(10, yp + 5); tft.setTextSize(3); tft.print(programas[programaEnEdicionIndex].nombre);
  } else {
    tft.fillRect(0, yp, tft.width(), 22, sel ? (enModoEdicionHorizontal ? TFT_BLACK : TFT_CYAN) : TFT_BLACK);
    tft.setTextColor(sel ? (enModoEdicionHorizontal ? TFT_YELLOW : TFT_BLACK) : TFT_WHITE);
    tft.setTextSize(2); tft.setCursor(10, yp); tft.print("S"); tft.print(idx+1); tft.print(":");
    // Dibujar R, T, t con sus resaltados si corresponde...
  }
}

void dibujarPantallaCalentando(bool r) {
  if (r) {
    tft.setTextColor(TFT_ORANGE); tft.setTextSize(3); tft.setCursor(80, 50); tft.print("HORNEANDO");
    tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(30, 90); tft.print("Actual:");
    tft.setCursor(30, 130); tft.print("Meta:");
  }
  tft.fillRect(150, 85, 100, 80, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(4); tft.setCursor(150, 85); tft.print(currentTemperature, 0); tft.print("c");
  tft.setTextColor(TFT_GREEN); tft.setCursor(150, 125); tft.print(programas[programaActivoIndex].etapas[etapaActualIndex].temperatura); tft.print("c");
}

void dibujarPantallaEnfriando(bool r) {
  tft.setTextColor(TFT_GREEN); tft.setTextSize(3); tft.setCursor(80, 70); tft.print("FINALIZADO");
}

void dibujarPantallaCalibracion() {
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(20, 100); tft.print("Lectura Cruda:");
  tft.setCursor(200, 100); tft.setTextColor(TFT_YELLOW); tft.print(temperaturaCrudaSimulada, 1);
  tft.setTextColor(TFT_WHITE); tft.setCursor(20, 140); tft.print("Temp. Real:");
  tft.fillRect(190, 138, 80, 20, TFT_CYAN); tft.setTextColor(TFT_BLACK); tft.setCursor(200, 140); tft.print(valorCalibracionEditado, 1);
}

void cargarProgramaDePrueba() {
  strcpy(programas[0].nombre, "Ceramica 1");
  programas[0].numEtapas = 2;
  programas[0].etapas[0] = {300, 400, 10};
  programas[0].etapas[1] = {200, 600, 5};
  numProgramasGuardados = 1;
}

void actualizarTemperaturaEnBarra() {} // Placeholder
