# Controlador Digital para Horno de Cerámica (ESP32)

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. Permite gestionar programas de horneado por etapas con rampas de temperatura, tiempos de mantenimiento y un sistema de control PID.

## Características actuales
- **Interfaz Gráfica:** Pantalla TFT ILI9341 con menús fluidos y gráfica en tiempo real.
- **Gestión de Programas:** Soporte para hasta 5 programas con 10 etapas cada uno (Rampa, Temperatura Meta, Tiempo).
- **Persistencia de Datos:** Los programas y calibración se guardan automáticamente en la memoria flash mediante `LittleFS`.
- **Control PID:** Lógica de control de temperatura precisa para rampas y mantenimientos.
- **Menú de Calibración:** Sistema para ajustar el offset de la termocupla en tiempo real.
- **Simulación Acelerada:** Modo x60 para verificar el funcionamiento de ciclos largos en minutos.

## Hardware
- **ESP32** (Dev Module o WROOM).
- **Pantalla TFT ILI9341** (320x240).
- **Módulo de Relé** (Configurado como Active-LOW).
- **Termocupla Tipo K** con módulo **MAX31855** (Lectura de alta precisión).
- **4 Pulsadores:** Arriba, Abajo, OK, Salir (EXIT).

## Estructura del Proyecto
- **/CC_V10.6/**: Código fuente de la versión actual.
- **/versiones/**: Historial de versiones anteriores del firmware.
- **/Diagnostic_Hardware/**: Scripts de prueba para los componentes.
- **FSD.md**: Especificaciones funcionales detalladas.
- **CHANGELOG.md**: Registro de cambios por versión.

## Pinout (Conexiones)
| Componente | Pin ESP32 | Función |
|------------|-----------|---------|
| **Relé** | GPIO 17   | Control de Resistencia |
| **Botón UP**| GPIO 26   | Navegación / Aumento |
| **Botón DOWN**| GPIO 25 | Navegación / Disminución |
| **Botón OK** | GPIO 33   | Confirmar / Editar |
| **Botón EXIT**| GPIO 32  | Volver / Cancelar |
| **TFT LED** | GPIO 12   | Retroiluminación |

*Nota: Los pines de la pantalla (CS, DC, RST, etc.) se configuran en el archivo `User_Setup.h` de la librería TFT_eSPI.*

## Librerías necesarias
Es necesario instalar las siguientes librerías desde el Gestor de Librerías de Arduino:
1. `TFT_eSPI` (Configurar `User_Setup.h` para ILI9341 y pines correspondientes).
2. `PID_v1` (Por Brett Beauregard).
3. `Adafruit MAX31855` (Por Adafruit).

## Sensor de Temperatura
El sistema utiliza la librería `Adafruit_MAX31855` para leer la temperatura real. Si el sensor no está conectado o falla (devuelve NaN), el sistema entrará automáticamente en **Modo Simulación**, utilizando una física simplificada para simular el calentamiento y permitir pruebas de interfaz.

## Simulación
El código incluye un multiplicador de tiempo (`SIM_MULTIPLIER = 60`) que permite que 1 hora de horneado transcurra en 1 minuto de tiempo real, ideal para pruebas de lógica sin esperar horas.

---
Desarrollado con fines educativos y de mejora para talleres de cerámica.
