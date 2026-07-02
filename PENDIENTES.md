# 📝 Tareas Pendientes - Controlador Horno V13.0

## 🛠️ Hardware (Validado ✅)
- [x] Soldar resistencias de 10k (Pull-up) a 3.3V para los botones.
- [x] Mover botones a los nuevos pines:
    - UP -> GPIO 36 (SVP)
    - DOWN -> GPIO 39 (SVN)
    - OK -> GPIO 34
    - EXIT -> GPIO 35
- [x] Conectar Pin CS de la SD al GPIO 33.
- [ ] **Circuito Snubber (Protección Inductiva):** 
    - Instalar el módulo comercial (RC + Varistor) visto en `hardware/snubber/`.
    - Conectar en paralelo con la bobina del contactor.

## 🧪 Pruebas y Validación
- [x] Ejecutar `Diagnostic_SD_Buttons_V13.ino` para validar el nuevo hardware.
- [x] Verificar que la pantalla no presente parpadeos al usar la SD simultáneamente.

## 💻 Software / Firmware (Hacia V14.0)
- [x] Migrar lógica de `CC_V13.0.ino` a `CC_V14.0.ino`.
- [x] Implementar la lógica de "Personality Module" (Sincronización LittleFS <-> SD).
- [x] Integrar feedback visual en LED RGB (Azul: Standby, Naranja: Calentando, Verde: Fin, Rojo: Error).
- [x] Añadir aviso visual en la barra de estado si la SD no está presente.

---
*Referencia técnica: Ver `GEMINI.md` para mandatos de arquitectura.*
