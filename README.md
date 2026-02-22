# Controlador Digital para Horno de Cerámica (ESP32)

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. Permite gestionar programas de horneado por etapas con rampas de temperatura, tiempos de mantenimiento y un sistema de control PID.

## Características actuales
- **Interfaz Gráfica:** Pantalla TFT ILI9341 con menús fluidos.
- **Gestión de Programas:** Soporte para múltiples programas con hasta 10 etapas cada uno (Rampa, Temperatura Meta, Tiempo).
- **Control PID:** Lógica de control de temperatura precisa (actualmente en modo simulación).
- **Menú de Calibración:** Sistema para ajustar el offset de la termocupla en un punto (ej. 0°C).
- **Simulación Acelerada:** Modo x60 para verificar el funcionamiento de ciclos largos en minutos.

## Hardware
- **ESP32** (Dev Module o WROOM).
- **Pantalla TFT ILI9341** (320x240).
- **Módulo de Relé** (Configurado como Active-LOW).
- **Termocupla Tipo K** con módulo **MAX31855** (Próximamente en código).
- **4 Pulsadores:** Arriba, Abajo, OK, Salir (EXIT).

## Pinout (Conexiones)
| Componente | Pin ESP32 | Función |
|------------|-----------|---------|
| **Relé** | GPIO 17   | Control de Resistencia |
| **Botón UP**| GPIO 26   | Navegación / Aumento |
| **Botón DOWN**| GPIO 25 | Navegación / Disminución |
| **Botón OK** | GPIO 33   | Confirmar / Editar |
| **Botón EXIT**| GPIO 32  | Volver / Cancelar |
| **TFT LED** | GPIO 27   | Retroiluminación |

*Nota: Los pines de la pantalla (CS, DC, RST, etc.) se configuran en el archivo `User_Setup.h` de la librería TFT_eSPI.*

## Librerías necesarias
Es necesario instalar las siguientes librerías desde el Gestor de Librerías de Arduino:
1. `TFT_eSPI` (Configurar `User_Setup.h` para ILI9341 y pines correspondientes).
2. `PID_v1` (Por Brett Beauregard).

## Simulación
El código incluye un multiplicador de tiempo (`SIM_MULTIPLIER = 60`) que permite que 1 hora de horneado transcurra en 1 minuto de tiempo real, ideal para pruebas de lógica sin esperar horas.

---
Desarrollado con fines educativos y de mejora para talleres de cerámica.
