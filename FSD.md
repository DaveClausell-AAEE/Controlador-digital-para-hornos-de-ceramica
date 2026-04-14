# Documento de Especificaciones Funcionales (FSD) - Controlador de Horno de Cerámica

## 1. Descripción General
Este proyecto consiste en un sistema de control de temperatura de alta precisión basado en el microcontrolador **ESP32**, diseñado específicamente para hornos de cerámica que requieren curvas de temperatura complejas (rampas y mantenimientos). El sistema utiliza un algoritmo **PID** para gestionar la potencia del horno a través de un relé de estado sólido o electromecánico.

## 2. Hardware del Sistema
- **Microcontrolador:** ESP32 (38-pin Dev Module).
- **Sensor de Temperatura:** Módulo MAX31855 con Termocupla Tipo K (Rango hasta 1350°C).
- **Actuador:** Relé de potencia (Configuración Active-LOW).
- **Interfaz Visual:** Pantalla TFT ILI9341 (320x240 px) con controlador SPI.
- **Entradas de Usuario:** 4 Pulsadores (UP, DOWN, OK, EXIT).

## 3. Pinout Detallado (Arquitectura Industrial V13.0)

```text
       ESP32 (38-pin Dev Module) - Conexión Directa a TFT
      +-------------------------------------------+
      | [GND] [5V] [3.3V] ...                     |
(D12) | TFT_LED (PWM)      GPIO 17 [EXT] -> RELAY |
(D14) | SENSOR_CLK (HSPI)  GPIO 27 [EXT] -> BUZZER|
(D13) | SENSOR_MISO(HSPI)  GPIO 25 [IN]  <- BTN_DN|
(D5 ) | SENSOR_CS  (HSPI)  GPIO 26 [IN]  <- BTN_UP|
      |                    GPIO 33 [IN]  <- BTN_OK|
(D16) | LED_RED            GPIO 32 [IN]  <- BTN_EX|
(D21) | LED_GREEN                                 |
(D22) | LED_BLUE           SPI (VSPI para TFT)    |
      |                    SCK: 18, MISO: 19 (NC) |
(D2 ) | TFT_DC             MOSI: 23, CS: 15       |
(D4 ) | TFT_RST                                   |
      +-------------------------------------------+

Nota: La placa se monta en la parte posterior de la pantalla.
```

| Componente | Pin ESP32 | Función | Bus / Tipo |
|------------|-----------|---------|------------|
| **Relé (Externo)** | GPIO 17 | Control de Resistencia | Digital |
| **Buzzer (Ext)** | GPIO 27 | Alarma Sonora | Digital |
| **LED RGB (R)** | GPIO 16 | Estado (Rojo) | Digital |
| **LED RGB (G)** | GPIO 21 | Estado (Verde) | Digital |
| **LED RGB (B)** | GPIO 22 | Estado (Azul) | Digital |
| **Botón UP** | GPIO 26 | Navegación | Digital (PULLUP) |
| **Botón DOWN** | GPIO 25 | Navegación | Digital (PULLUP) |
| **Botón OK** | GPIO 33 | Confirmar | Digital (PULLUP) |
| **Botón EXIT** | GPIO 32 | Volver | Digital (PULLUP) |
| **TFT LED** | GPIO 12 | Brillo Pantalla | PWM |
| **TFT CS** | GPIO 15 | Selección TFT | **VSPI** |
| **TFT DC** | GPIO 2 | Datos/Comando | **VSPI** |
| **TFT RST** | GPIO 4 | Reset Pantalla | **VSPI** |
| **MAX31855 CLK**| GPIO 14 | Reloj Sensor | **HSPI** |
| **MAX31855 MISO**| GPIO 13 | Datos Sensor | **HSPI** |
| **MAX31855 CS** | GPIO 5 | Selección Sensor | **HSPI** |

## 4. Recomendaciones de Ingeniería Industrial (V13.0)
### 4.1 Integridad de Señal y Ruido
- **Separación de Buses SPI:** Se utiliza HSPI para el sensor de temperatura y VSPI para la pantalla TFT. Esto evita interferencias electromagnéticas (EMI) y cuellos de botella en el bus compartidos que podrían corromper las lecturas críticas del MAX31855.
- **Filtrado de Alimentación:** Implementar un filtro pasa-banda en la entrada de 3.3V del ESP32 con un capacitor electrolítico de 10uF (baja frecuencia) y uno cerámico de 100nF (alta frecuencia) lo más cerca posible de los pines de alimentación.
- **Aislamiento de Pines:** Todos los pines GPIO no utilizados deben configurarse explícitamente como `INPUT_PULLUP` o `OUTPUT` en estado bajo para evitar que actúen como antenas de ruido.

### 4.2 Diseño Físico y Modularidad
- **Modularidad de Potencia:** La fuente de alimentación y el relé deben ubicarse en un módulo externo separado. La placa de control solo recibe alimentación DC limpia y envía señales de control de bajo nivel.
- **Factor de Forma:** Placa compacta diseñada para encajar directamente en los pines traseros de una pantalla ILI9341, optimizando el espacio y reduciendo el cableado.

### 4.3 Interfaz de Usuario Remota (LED de Estado)
- **AZUL:** Conectado a WiFi, en espera (Stand-by).
- **NARANJA/AMARILLO:** Horneado en curso (Calentando).
- **VERDE:** Ciclo terminado / Manteniendo.
- **ROJO:** Error crítico / Fallo de sensor.
- **CIAN PARPADEANTE:** Modo Configuración (Punto de Acceso).

### 4.4 Funciones de Recuperación
- **Hard Reset:** Presionar simultáneamente **OK + EXIT** durante 3 segundos provocará un reinicio de software (`ESP.restart()`).

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

### 4.4 Personalización y UI Avanzada
- **Control de Brillo:** Ajuste de intensidad lumínica desde el menú de ajustes (PWM).
- **Splash Screen:** Pantalla de bienvenida `"Controlador by DAC LAB"` con animación de carga al iniciar.
- **Barra de Estado Proactiva:** Muestra la temperatura y el estado actual del horno (LISTO, CALENT., etc.) de forma persistente.
- **Info Sistema:** Medidor visual del porcentaje de ocupación de la memoria LittleFS.

### 4.5 Ajustes PID Dinámicos
- Menú interactivo para modificar los parámetros Kp, Ki y Kd sin necesidad de modificar el código fuente.

## 5. Casos de Prueba y Manejo de Errores

| Caso de Prueba | Acción del Sistema | Estado de Seguridad |
|----------------|-------------------|---------------------|
| **Falla del Sensor (MAX31855)** | Si el sensor devuelve `NaN` o error de conexión, se detiene el ciclo de horneado inmediatamente. | **Relé OFF (HIGH)** |
| **Sobre-temperatura** | Si la temperatura supera el límite de seguridad (ej. 1200°C), se activa la alarma y se corta el relé. | **Relé OFF (HIGH)** |
| **Desconexión de Relé** | El sistema detecta si no hay cambio de temperatura tras 10 min de activación (Watchdog térmico). | **Estado de FALLO** |
| **Pérdida de Energía** | Al reiniciar, el horno consulta `/recovery.bin` para intentar retomar el ciclo donde quedó. | **AUTORECOVERY** |
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
- Persistencia de datos mediante `LittleFS` para guardar programas, calibración, PID y brillo.
- Sistema de edición visual completo para programas y etapas.
- Gráfica de temperatura en tiempo real con escalado dinámico y etiquetas de tiempo.
- **Auto-recovery:** Guardado de estado dinámico en LittleFS.
- **Alarmas Sonoras:** Integración completa de buzzer con sonidos diferenciados.
- **Conectividad WiFi:** Servidor web integrado para monitoreo remoto y pantalla de info sistema con IP.

### 7.2 Próximos Pasos 🚀
1. **Seguridad Avanzada:** Implementar un sistema de notificaciones por email/telegram en caso de fallo.
2. **Histórico de Horneados:** Guardar un log de los últimos horneados en LittleFS para descargar vía web.
