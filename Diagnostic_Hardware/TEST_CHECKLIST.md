# 📋 Checklist de Validación Técnica - Controlador V12.2

Este documento sirve de guía para validar el funcionamiento del controlador de horno antes de su instalación definitiva en el hardware de potencia.

### 1. Verificación de Interfaz y Sonido
- [ ] **Splash Screen:** ¿Aparece el logo "Controlador by DAC LAB" al encender?
- [ ] **Pitidos de Inicio:** ¿Suena el doble pitido corto al arrancar?
- [ ] **Navegación:** ¿Cada pulsación de botón genera un "click" audible?
- [ ] **Visibilidad:** ¿El menú de Ajustes se ve completo (6 ítems) sin cortarse abajo?
- [ ] **Configuración de Sonido:** Entra en Ajustes > Sonido. Cámbialo a "NO". ¿Se silencian los clicks? ¿Se mantienen silenciados tras reiniciar el equipo?

### 2. WiFi y Conectividad (Modo AP)
- [ ] **Modo Configuración:** Ve a Ajustes > Reset WiFi. ¿Aparece la pantalla de ayuda con la red `HORNO-CONFIG` y la IP `192.168.4.1`?
- [ ] **Portal Cautivo:** Conéctate con tu móvil a `HORNO-CONFIG`. ¿Carga la página de configuración al entrar a la IP?
- [ ] **Conexión Real:** Introduce tus credenciales reales. ¿El horno se reinicia y muestra una IP válida en "Info Sistema"?
- [ ] **Monitoreo Web:** Entra a la IP del horno desde tu móvil. ¿Ves la temperatura en tiempo real en el navegador?

### 3. Lógica de Control y Gráfica (Simulación)
- [ ] **Inicio de Programa:** Selecciona un programa y dale a "Iniciar". ¿Muestra el resumen antes de empezar?
- [ ] **Gráfica Dinámica:** Deja que el programa corra unos minutos. ¿Aparecen los puntos en la gráfica? ¿Son visibles las etiquetas "-10m" y "Ahora" en la base?
- [ ] **Escalado de Eje Y:** Durante la simulación, ¿el número máximo del eje Y de la gráfica aumenta automáticamente?
- [ ] **Control de Relé:** Verifica el LED del Relé (pin 17). ¿Parpadea según la salida del PID? (Debería estar más tiempo encendido al principio y parpadear más al acercarse al setpoint).

### 4. Seguridad y Fallos
- [ ] **Fallo de Sensor:** Con el programa corriendo, desconecta la termocupla. ¿Se detiene el proceso y muestra una pantalla de error roja? ¿El relé se apaga inmediatamente?
- [ ] **Auto-Recovery:** Apaga el equipo en medio de un horneado simulado. Al encenderlo, ¿te pregunta si quieres retomar el ciclo?
- [ ] **Buzzer de Alarma:** En caso de error o fin de ciclo, ¿los sonidos son claramente diferentes a los clicks normales?

---
*Manual de pruebas generado por DAC LAB - 2026*
