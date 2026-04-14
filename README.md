# Controlador Digital para Horno de Cerámica (ESP32) V13.0 Industrial

Este proyecto es un controlador de temperatura profesional para hornos de cerámica basado en el microcontrolador **ESP32**. La versión 13.0 introduce una arquitectura de grado industrial con buses de datos separados para máxima estabilidad y feedback visual remoto.

## 🚀 Características Principales (V13.0 Industrial)
- **Arquitectura SPI Dual:** Separación física de buses (HSPI para sensor, VSPI para pantalla) para eliminar ruidos e interferencias en lecturas críticas.
- **LED Status RGB:** Visualización rápida del estado desde lejos (Azul: Online, Amarillo: Calentando, Verde: Listo, Rojo: Fallo, Cian: Config).
- **Hard Reset por Software:** Reinicio seguro del equipo mediante combinación de botones (OK + EXIT por 3s).
- **Inmunidad al Ruido:** Configuración de pines para entornos industriales hostiles y recomendaciones de filtrado bypass.
- **WiFi Manager & Portal Cautivo:** Configuración de red inalámbrica sin tocar el código.
- **Servidor Web de Monitoreo:** Visualiza la temperatura y el estado en tiempo real.
- **Seguridad Industrial:** Watchdog térmico, Autorecovery y alarmas sonoras/visuales.

## 🛠️ Hardware y Pinout (V13.0)
| Componente | Pin ESP32 | Función | Bus / Tipo |
|------------|-----------|---------|------------|
| **Relé (Ext)** | GPIO 17 | Control Resistencia (Active LOW) | Digital |
| **Buzzer (Ext)**| GPIO 27 | Alarma y Feedback | Digital |
| **LED RGB (R)** | GPIO 16 | Estado (Rojo) | Digital |
| **LED RGB (G)** | GPIO 21 | Estado (Verde) | Digital |
| **LED RGB (B)** | GPIO 22 | Estado (Azul) | Digital |
| **Botón UP**| GPIO 26 | Navegación | Digital (PULLUP) |
| **Botón DOWN**| GPIO 25 | Navegación | Digital (PULLUP) |
| **Botón OK** | GPIO 33 | Confirmar / Editar | Digital (PULLUP) |
| **Botón EXIT**| GPIO 32 | Volver / Reset (Combo) | Digital (PULLUP) |
| **TFT LED** | GPIO 12 | Brillo Pantalla | PWM |
| **Sensor (K)** | SPI (HSPI) | MAX31855 (CLK: 14, MISO: 13, CS: 5) | **HSPI** |
| **Pantalla** | SPI (VSPI) | ILI9341 (SCK: 18, MISO: 19, MOSI: 23, CS: 15) | **VSPI** |

## 📶 Configuración WiFi
1. Si no hay red, busque la WiFi `HORNO-CONFIG` en su móvil.
2. IP de configuración: `192.168.4.1`.
3. Para borrar el WiFi: **Ajustes > Reset WiFi** o use el Hard Reset (OK+EXIT).

## 🛡️ Notas de Montaje Industrial
- **Filtrado:** Colocar un capacitor de 10uF y uno de 100nF entre VCC y GND de la ESP32.
- **Modularidad:** Mantener la fuente de 220V/5V y el relé de potencia fuera de la placa de control para minimizar interferencias.
- **Hard Reset:** Mantenga presionados OK y EXIT por 3 segundos para forzar un reinicio del sistema.

## 🧪 Metodología de Pruebas (Pre-Instalación)
Antes de conectar a un horno real, se recomienda realizar las pruebas detalladas en el manual de validación técnica (ver sección de testing).

---
Desarrollado por **DAC LAB**
