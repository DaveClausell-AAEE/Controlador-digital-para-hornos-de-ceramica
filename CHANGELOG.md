# Historial de Versiones - Controlador de Horno

Este archivo registra la evolución del software, las nuevas funcionalidades y las correcciones implementadas en cada versión.

---

## [V13.0] - 2026-04-14 (Actual)
### 🏗️ Arquitectura de Grado Industrial
- **Dual SPI Bus:**
    - Implementada separación física de buses: **HSPI** exclusivo para el sensor MAX31855 y **VSPI** para la pantalla TFT.
    - Reduce drásticamente el ruido electromagnético en las lecturas de temperatura.
- **Feedback Visual de Estado (LED RGB):**
    - Añadida lógica para LED RGB en GPIOs 16, 21 y 22.
    - Colores: **Azul** (Online), **Amarillo** (Horneando), **Verde** (Listo/Mantenimiento), **Rojo** (Fallo), **Cian parpadeante** (Modo AP).
- **Hard Reset por Software:**
    - Nueva función de reinicio de emergencia: Presionando **OK + EXIT por 3 segundos** se fuerza un `ESP.restart()`.
- **Integridad y Seguridad:**
    - Configuración de pines no usados en modo `INPUT_PULLUP` para evitar ruido tipo "antena".
    - Recomendaciones de filtrado (10uF + 100nF) añadidas a la documentación.
- **Diseño Modular:**
    - El código ahora asume relé y fuente externos para mayor limpieza eléctrica.

---

## [V12.2] - 2026-04-01
### 🐛 Corrección de Interfaz de Inicio
- **Fix Pantalla Standby:**
    - Corregido error por el cual las etiquetas de "Temp. Actual" y "Programa Listo" no aparecían inmediatamente tras la pantalla de bienvenida.
    - Se forzó el dibujado completo del layout al iniciar el sistema.
- **Estabilidad Visual:**
    - Mejorada la gestión de estados para garantizar refrescos completos en el primer ciclo.

---

## [V12.1] - 2026-04-01
### 🎨 UI Optimizada y Guía de Configuración
- **Mejora en Modo AP:**
    - Ahora la pantalla TFT muestra instrucciones claras cuando el horno está en modo configuración.
    - Se indica la red WiFi (`HORNO-CONFIG`) y la dirección IP (`192.168.4.1`) directamente en el display.
- **Ajuste de Gráfica:**
    - La base de la gráfica se ha subido 10 píxeles para garantizar que las etiquetas de tiempo ("-10m" y "Ahora") sean completamente visibles.
- **Estabilidad de WiFi:**
    - Corregida lógica de reconexión en el loop principal.

---

## [V12.0] - 2026-04-01
### 📶 WiFi Manager y Configuración Dinámica
- **Modo Punto de Acceso (AP):**
    - Si no hay WiFi configurado o falla la conexión, el horno crea su propia red `HORNO-CONFIG`.
    - Permite configurar el SSID y Password desde un portal cautivo en el móvil (`192.168.4.1`).
- **Persistencia de Red:**
    - Las credenciales se guardan de forma segura en `/wifi.bin` dentro de LittleFS.
- **Opción de Reset WiFi:**
    - Añadida función en **Ajustes > Reset WiFi** para borrar las credenciales y forzar el modo configuración.
- **Interfaz Informativa:**
    - La pantalla de Info WiFi ahora indica explícitamente cuando el dispositivo está en modo `CONFIG AP`.

---

## [V11.1] - 2026-04-01
### 🔊 Feedback Sonoro y Ajustes de Usuario
- **Sonidos de Interfaz:**
    - Implementado un sonido de "click" (20ms) al navegar por los menús y pulsar botones.
    - Mejora la experiencia táctil y confirma las acciones del usuario.
- **Gestión de Sonido:**
    - Nueva opción en el menú de **Ajustes > Sonido** para habilitar o deshabilitar los pitidos.
    - El estado del sonido se guarda de forma persistente en LittleFS.
- **Correcciones Técnicas:**
    - Optimización del orden de declaración de variables globales.

---

## [V11.0] - 2026-04-01
### 🌐 Conectividad y Monitoreo Remoto
- **Implementación de WiFi:**
    - Conexión automática a red WiFi configurada.
    - Sistema de reconexión automática en el loop.
- **Servidor Web Básico:**
    - Servidor HTTP integrado que permite ver el estado y la temperatura en tiempo real desde cualquier dispositivo en la red.
    - Refresco automático de la página cada 5 segundos.
- **Mejora en Info Sistema:**
    - La pantalla de información ahora muestra la dirección IP local si está conectado, o "DESC." si no lo está.
    - Reorganización visual de los datos de memoria y programas.
- **Optimización de Gráfica en Tiempo Real:**
    - Implementado escalado dinámico del eje Y según la temperatura máxima registrada (con margen del 10%).
    - Añadidas etiquetas de escala de temperatura (0 a Max).
    - Añadidas etiquetas de tiempo en el eje X ("-10m" y "Ahora") para mejor legibilidad.

---

## [V10.9] - 2026-03-26
### 🚀 Nuevas Funcionalidades y Personalización
- **Personalización de Bienvenida:** Nueva Splash Screen con el texto `"Controlador by DAC LAB"` y animación de carga.
- **Control de Brillo por Software:**
    - Añadida opción en el menú de Ajustes para regular la intensidad de la pantalla (PWM en GPIO 12).
    - El nivel de brillo se persiste automáticamente en LittleFS.
- **Gestión de Memoria Visual:** Nuevo apartado en **Info Sistema** que muestra el porcentaje de uso de LittleFS.
- **Ajustes PID en Vivo:** Menú dedicado para modificar Kp, Ki y Kd desde la interfaz sin necesidad de reprogramar.
- **Status Bar Informativo:** La barra superior ahora muestra el estado del horno (LISTO, CALENT., MANTEN., etc.) de forma persistente.
- **Traducción de Estados:** Cambio de "IDLE" a "LISTO" para una interfaz más intuitiva en español.

---

## [V10.8] - 2026-03-25
### 🛡️ Seguridad y Sonido
- **Sistema de Auto-Recuperación:** Implementado guardado de estado en `/recovery.bin` para retomar el horneado tras un fallo de energía.
- **Integración de Buzzer:**
    - Sonidos diferenciados para: Inicio de sistema (doble pitido corto), Fin de ciclo (pitido largo) y Fallo (triple pitido de alerta).
- **Herramienta de Diagnóstico V1.1:** Actualizada para incluir pruebas de sonido y corregir el pinout del buzzer (GPIO 27).

---

## [V10.7] - 2026-03-25
### 🎨 Mejoras de Interfaz y Correcciones
- **Corrección de Menú de Calibración:** Se ajustó la lógica para evitar actualizaciones si el sensor no está disponible y mejorar la experiencia de edición del offset.
- **Corrección de Tipografía:** Reemplazo de "ñ" por "n" en etiquetas de menú para garantizar compatibilidad con el set de caracteres del display.
- **Optimización de Refresco:** Se mejoró el refresco de pantalla en los menús de configuración para eliminar artefactos visuales y texto remanente.
- **Layout de Pantalla:** Se ajustó el layout de la pantalla de "Calentando/Manteniendo" para evitar la superposición de texto y números de temperatura.
- **Estabilidad de Lectura:** Se optimizó el refresco de la temperatura en la barra superior para evitar parpadeos innecesarios.

---

## [V10.6] - 2026-03-24
### 📊 Visualización Avanzada y UX
- **Gráfica de Temperatura en Tiempo Real:**
    - Se añadió un área de visualización gráfica durante el horneado.
    - Muestra los últimos 10 minutos de historia térmica (muestreo cada 5s).
- **Pantalla de Confirmación Pre-Vuelo:**
    - Nuevo estado de seguridad que muestra un resumen del programa (Nombre, Etapas, Temp. Máxima) antes de iniciar el calentamiento.
- **Diagnóstico de Errores Mejorado:**
    - El Watchdog Térmico ahora muestra mensajes específicos como: `"SIN AUMENTO DE TEMP: REVISAR RESISTENCIA"`.
- **Menú de Información WiFi:**
    - Nueva pantalla en Ajustes que muestra la dirección MAC del dispositivo, facilitando su identificación para futuras funciones de red.
- **Optimización de Interfaz:**
    - Ajuste de diseño en la pantalla de horneado para dar espacio a la gráfica sin perder legibilidad en los datos numéricos.

---

## [V10.5] - 2026-03-24
### 🛠️ Herramientas de Edición y Seguridad
- **Edición de Nombres "In-Situ":**
    - Implementado sistema de edición de caracteres letra por letra con parpadeo visual.
    - Navegación circular entre caracteres (A-Z, espacio, 0-9).
- **Gestión Dinámica de Etapas:**
    - Añadidas funciones para **Añadir Etapa** (hasta 10) y **Borrar Etapa** (mínimo 1) desde el menú de configuración.
    - Los cambios se guardan automáticamente en LittleFS.
- **Seguridad: Watchdog Térmico:**
    - Nuevo sistema de seguridad que monitorea la eficacia del calentamiento.
    - Si el relé está activo pero la temperatura no sube al menos 2°C en 10 minutos, el sistema corta la energía y muestra una pantalla de **FALLO** roja.
- **Mejoras Visuales:**
    - Implementado parpadeo (blink) en cursores de edición y selección de parámetros para mejorar el feedback al usuario.

---

## [V10.4] - 2026-03-24
### 💾 Persistencia y Gestión de Memoria
- **Implementación de LittleFS:**
    - Se añadió soporte para el sistema de archivos de memoria flash del ESP32.
    - Los programas y el ajuste de calibración ahora se guardan en un archivo binario `/config.bin`.
- **Persistencia Automática:**
    - Los datos se guardan automáticamente al salir del menú de edición de programas o al confirmar una calibración.
- **Mejora en Gestión de Programas:**
    - Se habilitó la funcionalidad de **"+ Crear Programa"**, permitiendo añadir nuevos programas vacíos directamente desde la interfaz.
    - Se cargan automáticamente datos de prueba en el primer inicio si no existe configuración previa.

---

## [V10.3] - 2026-03-24
### ✨ Mejoras en Hardware e Interfaz
- **Integración del Sensor MAX31855:**
    - Soporte nativo para lectura de Termocupla Tipo K mediante SPI (Pines compartidos con TFT).
    - Implementación de **Modo Híbrido**: Si el sensor falla o devuelve `NaN`, el sistema cambia automáticamente a **Modo Simulación** para permitir pruebas de UI sin hardware.
- **Interfaz de Configuración de Programas (UI):**
    - Se completó la visualización de parámetros por etapa (Rampa, Temperatura Meta, Tiempo).
    - Implementado sistema de resaltado visual (Amarillo) para indicar qué parámetro se está editando.
    - Navegación horizontal mejorada para ajustar valores con los botones UP/DOWN.
- **Barra de Estado Proactiva:**
    - La temperatura en la barra superior ahora se refresca en tiempo real en todos los menús, no solo en la pantalla principal.

---

## [V10.2] - Versión Base
### 📦 Funcionalidades Iniciales
- **Núcleo del Sistema:** Lógica de estados (Stand-by, Menú, Horneado).
- **Control PID:** Algoritmo básico de control de temperatura (simulado).
- **Menú de Calibración:** Ajuste de offset para la termocupla.
- **Interfaz Gráfica:** Menús básicos utilizando la librería `TFT_eSPI`.
- **Estructura de Datos:** Soporte para programas con múltiples etapas.

---

*Nota: Para volver a una versión anterior, simplemente carga el archivo `.ino` correspondiente a la versión deseada.*
