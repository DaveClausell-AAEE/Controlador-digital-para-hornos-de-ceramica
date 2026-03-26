# Controlador Digital para Horno de Cerámica (ESP32)

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. Permite gestionar programas de horneado por etapas con rampas de temperatura, tiempos de mantenimiento y un sistema de control PID.

## Características Principales (V10.9)
- **Interfaz Gráfica:** Pantalla TFT ILI9341 con menús fluidos y gráfica en tiempo real.
- **Splash Screen:** Pantalla de bienvenida personalizada `"Controlador by DAC LAB"`.
- **Gestión de Programas:** Soporte para hasta 5 programas con 10 etapas cada uno.
- **Control PID Editable:** Ajuste de Kp, Ki y Kd directamente desde el menú.
- **Control de Brillo:** Ajuste por software mediante PWM (GPIO 12).
- **Seguridad y Recuperación:** 
  - Watchdog térmico.
  - Autorecovery tras fallo de alimentación.
  - Alarma sonora (Buzzer).
- **Persistencia:** Guardado automático en LittleFS de todos los parámetros.

## Hardware y Pinout (Conexiones)

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

| Componente | Pin ESP32 | Función |
|------------|-----------|---------|
| **Relé** | GPIO 17 | Control de Resistencia |
| **Buzzer** | GPIO 27 | Alarma Sonora |
| **Botón UP**| GPIO 26 | Navegación / Aumento |
| **Botón DOWN**| GPIO 25 | Navegación / Disminución |
| **Botón OK** | GPIO 33 | Confirmar / Editar |
| **Botón EXIT**| GPIO 32 | Volver / Cancelar |
| **TFT LED** | GPIO 12 | Retroiluminación |

## Estructura del Proyecto
- **/versiones/**: Código fuente histórico.
- **/Diagnostic_Hardware/**: Scripts de prueba para componentes.
- **FSD.md**: Especificaciones funcionales detalladas.
- **CHANGELOG.md**: Registro de cambios por versión.

## Librerías necesarias
1. `TFT_eSPI` (Configurar `User_Setup.h` para ILI9341).
2. `PID_v1`
3. `Adafruit MAX31855`
