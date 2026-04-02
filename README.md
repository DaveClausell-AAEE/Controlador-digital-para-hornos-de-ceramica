# Controlador Digital para Horno de Cerámica (ESP32) V12.2

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. Permite gestionar programas de horneado por etapas con rampas de temperatura, tiempos de mantenimiento y un sistema de control PID avanzado con monitoreo remoto.

## 🚀 Características Principales (V12.2)
- **WiFi Manager & Portal Cautivo:** Configuración de red inalámbrica sin tocar el código. Si no hay red, el horno crea el punto de acceso `HORNO-CONFIG`.
- **Servidor Web de Monitoreo:** Visualiza la temperatura y el estado del proceso en tiempo real desde cualquier navegador en la red local.
- **Interfaz Gráfica Pro:** Pantalla TFT ILI9341 con menús fluidos, gráfica con escalado dinámico y barra de estado informativa.
- **Feedback Sonoro:** Sonidos diferenciados para navegación (clicks), inicio, fin de ciclo y alarmas de fallo.
- **Gestión de Programas:** Soporte para 5 programas de hasta 10 etapas cada uno (Rampa, Temp. Meta, Mantenimiento).
- **Seguridad Industrial:** 
  - Watchdog térmico (detecta si la resistencia no calienta).
  - Autorecovery (retoma el horneado tras un corte de luz).
  - Alarmas por sobretemperatura o fallo de sensor.
- **Personalización:** Ajuste de brillo PWM, calibración de offset y edición de PID desde el menú.

## 📶 Configuración WiFi (Primera vez o cambio de red)
1. Conecte el equipo. Si la pantalla indica `CONFIG AP`, busque la red WiFi `HORNO-CONFIG` en su móvil.
2. Conéctese y abra en el navegador la IP `192.168.4.1`.
3. Introduzca el nombre de su red y la contraseña. El equipo se reiniciará y se conectará automáticamente.
4. Para resetear el WiFi, vaya a **Ajustes > Reset WiFi**.

## 🛠️ Hardware y Pinout
| Componente | Pin ESP32 | Función |
|------------|-----------|---------|
| **Relé** | GPIO 17 | Control de Resistencia (Active LOW) |
| **Buzzer** | GPIO 27 | Alarma y Feedback |
| **Botón UP**| GPIO 26 | Navegación / Aumento |
| **Botón DOWN**| GPIO 25 | Navegación / Disminución |
| **Botón OK** | GPIO 33 | Confirmar / Editar |
| **Botón EXIT**| GPIO 32 | Volver / Cancelar |
| **TFT LED** | GPIO 12 | Brillo Pantalla (PWM) |
| **Sensor** | SPI (Shared) | MAX31855 (CS: GPIO 5) |

## 🧪 Metodología de Pruebas (Pre-Instalación)
Antes de conectar a un horno real, se recomienda realizar las pruebas detalladas en el manual de validación técnica (ver sección de testing).

---
Desarrollado por **DAC LAB**
