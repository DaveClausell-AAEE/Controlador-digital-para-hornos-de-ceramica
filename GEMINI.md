# GEMINI.md - Controlador de Horno de Cerámica (ESP32)

Este archivo contiene las directivas y el contexto técnico esencial para el desarrollo y mantenimiento del Controlador de Horno V13.0 Industrial. Estas reglas son de cumplimiento obligatorio para garantizar la estabilidad y seguridad del sistema.

## 🚀 Mandatos Principales
- **Integridad del Bus SPI:** NUNCA compartas buses entre el sensor (MAX31855 en HSPI) y la pantalla (TFT en VSPI). La separación física es crítica para evitar ruidos en lecturas de temperatura.
- **Seguridad (Failsafe):** El relé opera en lógica **Active-LOW**. Asegúrate de que `digitalWrite(RELAY_PIN, HIGH)` sea siempre el estado por defecto para apagar la resistencia.
- **Persistencia:** Cualquier cambio en la configuración (PID, Brillo, Programas) debe persistirse en `LittleFS` usando la estructura `config.bin`.
- **UI Responsiva:** Mantén el sistema de refresco basado en la bandera `necesitaRefresco` para evitar parpadeos innecesarios en la pantalla TFT.

## 🛠️ Especificaciones de Hardware y Pinout
**Fuente de Verdad:** Consulta siempre el bloque de `#define` al inicio del archivo `.ino` más reciente en `/versiones/`.

### ⚖️ Leyes de Hardware (Inmutables)
Cualquier cambio físico o de hardware debe respetar estas reglas:
1. **Integridad del Bus SPI:** El sensor (MAX31855) **DEBE** permanecer en el bus HSPI y la pantalla (TFT) en el bus VSPI. No mezcles dispositivos en el mismo bus para evitar ruidos térmicos.
2. **Lógica de Relé (Failsafe):** El relé principal opera en **Active-LOW**. Al agregar o cambiar pines de control de potencia, asegura que el estado `HIGH` sea siempre el de "Apagado/Seguro".
3. **Pulsadores:** Todos los botones deben configurarse como `INPUT_PULLUP` y conectarse a GND.
4. **Nuevos Periféricos:** Si agregas hardware nuevo (sensores I2C, UART, etc.), actualiza la sección de "Arquitectura" en este archivo si afecta el flujo de la máquina de estados.
## 🏗️ Arquitectura de Software
### 💾 Sistema de Backup y Portabilidad (Personality Module)
Para permitir el reemplazo rápido de módulos ESP32 en campo, el sistema implementa un esquema de **Almacenamiento Dual**:
1. **Mirroring LittleFS <-> SD:** Los archivos de configuración (`config.bin`) y programas deben existir tanto en la memoria interna como en la SD.
2. **Prioridad de Restauración:** Al iniciar, si LittleFS está vacío o corrupto pero hay una SD presente, el sistema debe clonar los datos de la SD a la memoria interna automáticamente.
3. **Sincronización:** Cualquier cambio en ajustes, PID o programas debe intentar escribirse en ambos medios. Si la SD no está presente, el sistema funciona normalmente con LittleFS pero notifica la falta de backup.

### Máquina de Estados (`EstadoHorno`)
...

El flujo del programa se gestiona mediante un `enum EstadoHorno`. Al añadir nuevas funcionalidades:
1. Añade el estado al `enum`.
2. Actualiza `procesarEntrada()` para la navegación.
3. Actualiza `actualizarPantalla()` para el renderizado.
4. Actualiza `actualizarLEDStatus()` si requiere un feedback visual específico.

### Sistema de Archivos (LittleFS)
- `/config.bin`: Almacena configuraciones generales, programas y parámetros PID.
- `/recovery.bin`: Se crea al iniciar un horneado y se borra al finalizar. Permite el **Auto-recovery** en caso de corte de energía.

### Control PID
- Ventana de tiempo: 5 segundos (`windowSize = 5000`).
- Implementación: Librería `PID_v1`.
- Sintonización: Kp, Ki, Kd son ajustables desde el menú y se guardan en `/config.bin`.

## 🎨 Estándares de Código
- **Idioma:** Los comentarios y nombres de funciones de UI deben estar en **Español** (ej: `dibujarMenuPrincipal`), siguiendo la convención de DAC LAB.
- **Tipado:** Evita el uso de `String` de Arduino en el loop principal para prevenir fragmentación de memoria; usa `char[]` y `sprintf`.
- **Simulación:** Mantén el código de simulación (cuando `isnan(c)` es true) para facilitar pruebas sin hardware conectado.

## 🧪 Flujo de Validación
1. **Compilación:** Siempre verifica que el código compile para el target `ESP32 Dev Module`.
2. **Watchdog Térmico:** Si modificas la lógica de control, verifica que el watchdog (`INTERVALO_WATCHDOG`) siga detectando fallos de calentamiento.
3. **Hard Reset:** Verifica que la combinación `OK + EXIT` (3s) mantenga su funcionalidad de reinicio de emergencia.

## 📁 Estructura del Repositorio
- `versiones/`: Histórico de versiones. La versión con el número más alto es la referencia actual.
- `Diagnostic_Hardware/`: Herramientas para validar componentes individuales.
- `FSD.md`: Documento de especificación funcional detallado.
- `PENDIENTES.md`: Rastreo de tareas activas y estado de la migración de hardware.
- `PROTOCOLO_INSTALACION.md`: Guía crítica para la puesta en marcha, calibración y seguridad eléctrica.
