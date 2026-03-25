# Controlador Digital para Horno de Cerámica (ESP32)

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. Permite gestionar programas de horneado por etapas con rampas de temperatura, tiempos de mantenimiento y un sistema de control PID.

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
