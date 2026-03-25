# Historial de Versiones - Controlador de Horno

Este archivo registra la evolución del software, las nuevas funcionalidades y las correcciones implementadas en cada versión.

---

## [V10.6] - 2026-03-24 (Actual)
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
