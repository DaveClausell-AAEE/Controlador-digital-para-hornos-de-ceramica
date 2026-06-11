# 📝 Tareas Pendientes - Controlador Horno V13.0

## 🛠️ Hardware (Pausa por materiales)
- [ ] Soldar resistencias de 10k (Pull-up) a 3.3V para los botones.
- [ ] Mover botones a los nuevos pines:
    - UP -> GPIO 36 (SVP)
    - DOWN -> GPIO 39 (SVN)
    - OK -> GPIO 34
    - EXIT -> GPIO 35
- [ ] Conectar Pin CS de la SD al GPIO 33.
- [ ] **Circuito Snubber (Protección Inductiva):** 
    - Instalar el módulo comercial (RC + Varistor) visto en `hardware/snubber/`.
    - Conectar en paralelo con la bobina del contactor.
    - *Nota de Seguridad:* Para uso industrial prolongado, considerar reemplazar el capacitor rojo del módulo por uno **Clase X2 (275VAC)**.

## 🧪 Pruebas y Validación
...
```text
DIAGRAMA DEL SNUBBER (Red RC):
-----------------------------
(Fase AC) ---- [Relé] ----+---- (Bobina Contactor) ---- (Neutro AC)
                          |
                          +---[ R: 120R 2W ]---+
                                               |
                          +---[ C: 0.1uF X2 ]--+
```
- [ ] Ejecutar `Diagnostic_SD_Buttons_V13.ino` para validar el nuevo hardware.
- [ ] Verificar que la pantalla no presente parpadeos al usar la SD simultáneamente.

## 💻 Software / Firmware
- [ ] Actualizar el firmware principal `CC_V13.0.ino` con el nuevo pinout de botones.
- [ ] Implementar la lógica de "Personality Module" (Sincronización LittleFS <-> SD).
- [ ] Añadir aviso visual en la barra de estado si la SD no está presente.

---
*Referencia técnica: Ver `GEMINI.md` para mandatos de arquitectura.*
