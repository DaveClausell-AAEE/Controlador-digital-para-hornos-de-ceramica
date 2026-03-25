# Documento de Especificaciones Funcionales (FSD) - Controlador de Horno de Cerámica

## 1. Descripción General
Este proyecto consiste en un sistema de control de temperatura de alta precisión basado en el microcontrolador **ESP32**, diseñado específicamente para hornos de cerámica que requieren curvas de temperatura complejas (rampas y mantenimientos). El sistema utiliza un algoritmo **PID** para gestionar la potencia del horno a través de un relé de estado sólido o electromecánico.

## 2. Hardware del Sistema
- **Microcontrolador:** ESP32 (38-pin Dev Module).
- **Sensor de Temperatura:** Módulo MAX31855 con Termocupla Tipo K (Rango hasta 1350°C).
- **Actuador:** Relé de potencia (Configuración Active-LOW).
- **Interfaz Visual:** Pantalla TFT ILI9341 (320x240 px) con controlador SPI.
- **Entradas de Usuario:** 4 Pulsadores (UP, DOWN, OK, EXIT).

## 3. Pinout Detallado (Conexiones)

```text
       ESP32 (38-pin Dev Module)
      +------------------------+
      |        ...             |
(D12) | TFT_LED    GPIO 17 (R) | (Relay)
(D14) | [NC]       GPIO 27 (B) | (Buzzer)
(D26) | BTN_UP     GPIO 25 (D) | (BTN_DOWN)
(D33) | BTN_OK     GPIO 32 (E) | (BTN_EXIT)
      |                        |
      |   SPI (MAX31855/TFT)   |
(D18) | SCK  MISO 19  MOSI 23  |
(D5 ) | CS1  CS2  15           |
      +------------------------+

Conexiones:
- Relé (Resistencia): GPIO 17
- Buzzer (Alarma):    GPIO 27
- TFT Backlight (BL): GPIO 12
- Pulsadores (PULLUP):
  UP: 26, DOWN: 25, OK: 33, EXIT: 32
```

| Componente | Pin ESP32 | Función | Tipo de Señal |
|------------|-----------|---------|---------------|
| **Relé** | GPIO 17 | Control de Resistencia | Digital (Salida) |
| **Buzzer** | GPIO 27 | Alarma Sonora | Digital (Salida) |
| **Botón UP** | GPIO 26 | Navegación / Incremento | Digital (Entrada) |
| **Botón DOWN** | GPIO 25 | Navegación / Decremento | Digital (Entrada) |
| **Botón OK** | GPIO 33 | Confirmar / Editar | Digital (Entrada) |
| **Botón EXIT** | GPIO 32 | Volver / Cancelar | Digital (Entrada) |
| **TFT LED** | GPIO 12 | Retroiluminación | Digital (Salida) |
| **MAX31855 CLK**| GPIO 18 | Reloj SPI (Shared) | Digital (Salida) |
| **MAX31855 MISO**| GPIO 19 | Datos del Sensor | Digital (Entrada) |
| **MAX31855 CS** | GPIO 5 | Selección de Chip | Digital (Salida) |
| **TFT SCK** | GPIO 18 | Reloj SPI (Shared) | Digital (Salida) |
| **TFT MISO** | GPIO 19 | Datos TFT (Shared) | Digital (Entrada) |
| **TFT MOSI** | GPIO 23 | Datos TFT (Salida) | Digital (Salida) |
| **TFT CS** | GPIO 15 | Selección de Chip TFT | Digital (Salida) |
| **TFT DC/RS** | GPIO 2 | Datos/Comando TFT | Digital (Salida) |
| **TFT RST** | GPIO 4 | Reset Pantalla | Digital (Salida) |

## 4. Funcionalidades Principales
### 4.1 Gestión de Programas
- Soporte para hasta **5 programas** distintos en memoria.
- Cada programa consta de hasta **10 etapas** configurables.
- Parámetros por etapa:
  - **Rampa:** Velocidad de ascenso/descenso (°C por hora).
  - **Temperatura Meta:** Punto final de la etapa (°C).
  - **Tiempo de Mantenimiento:** Duración de la temperatura una vez alcanzada (minutos).

### 4.2 Control de Temperatura (PID)
- Algoritmo PID para evitar sobre-oscilaciones.
- Ventana de control de tiempo (Time Proportioning Control) de 5 segundos.
- Modo simulación acelerada (x60) para pruebas de lógica.

### 4.3 Menú de Calibración
- Ajuste de **Offset** en tiempo real para corregir desviaciones de la termocupla.
- Visualización de temperatura cruda vs temperatura corregida.

## 5. Casos de Prueba y Manejo de Errores

| Caso de Prueba | Acción del Sistema | Estado de Seguridad |
|----------------|-------------------|---------------------|
| **Falla del Sensor (MAX31855)** | Si el sensor devuelve `NaN` o error de conexión, se detiene el ciclo de horneado inmediatamente. | **Relé OFF (HIGH)** |
| **Sobre-temperatura** | Si la temperatura supera el límite de seguridad (ej. 1200°C), se activa la alarma y se corta el relé. | **Relé OFF (HIGH)** |
| **Desconexión de Relé** | El sistema detecta si no hay cambio de temperatura tras 10 min de activación (Watchdog térmico). | **Estado de FALLO** |
| **Pérdida de Energía** | Al reiniciar, el horno debe estar en STAND-BY por seguridad (Revisar para futura implementación de autorecovery). | **STAND-BY** |
| **Error de Etapa** | Si una rampa es imposible de alcanzar por el hardware, el sistema notificará pero mantendrá el control PID al máximo. | **ACTIVO** |

## 6. Estructura del Proyecto
- **/versiones/**: Repositorio histórico de las versiones del firmware (.ino).
- **/Diagnostic_Hardware/**: Scripts de prueba para verificar el funcionamiento de la pantalla, sensor y relé.
- **FSD.md**: Especificaciones funcionales y técnicas del sistema.
- **CHANGELOG.md**: Registro histórico de cambios y mejoras implementadas.
- **README.md**: Documentación de hardware y guía rápida de configuración.

## 7. Estado del Desarrollo y Próximos Pasos
### 7.1 Hitos Completados ✅
- Implementación de la librería `Adafruit_MAX31855` para lecturas reales del sensor.
- Persistencia de datos mediante `LittleFS` para guardar programas y calibración.
- Sistema de edición visual completo para programas y etapas.
- Gráfica de temperatura en tiempo real y barra de estado dinámica.

### 7.2 Próximos Pasos 🚀
1. **Optimización de Gráficos:** Mejorar el escalado de la gráfica y añadir etiquetas de tiempo.
2. **Implementación de WiFi:** Añadir un servidor web básico para monitoreo remoto desde el celular.
3. **Auto-recovery:** Guardar el estado del ciclo actual en LittleFS para retomar el horneado tras un corte de luz.
4. **Alarmas Sonoras:** Integrar un buzzer para notificar fallos o fin de ciclo.
