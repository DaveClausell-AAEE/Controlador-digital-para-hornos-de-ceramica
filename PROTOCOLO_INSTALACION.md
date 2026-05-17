# 📋 Protocolo de Instalación y Puesta en Marcha (V13.0)

Este protocolo asegura que el controlador pase del banco de pruebas al horno real sin fallos críticos. Sigue los pasos en orden estricto.

## Fase 1: Verificación Eléctrica (Sin ESP32 conectado)
*Objetivo: Proteger el microcontrolador de picos de tensión.*

1.  **Fuente de Alimentación:** Conecta la fuente 220V/5V. Mide con multímetro la salida: debe ser **5V (+/- 0.2V)** constantes.
2.  **Continuidad de GND:** Verifica que todos los puntos de tierra (botones, pantalla, relé, sensor) compartan el mismo negativo.
3.  **Seguridad del Relé:** Verifica que sin señal del ESP32, el relé de potencia esté **ABIERTO** (horno apagado).

## Fase 2: Montaje y Sensores
1.  **Instalar ESP32:** Inserta el módulo y carga el firmware.
2.  **Conexión de Termocupla:** Conecta los cables de la sonda al MAX31855. 
    *   *Nota:* Si la temperatura baja al calentar la punta, invierte los cables de la sonda.
3.  **Buzzer y LED:** Confirma que al encender, el LED se ponga Azul (Stand-by) y el buzzer emita dos pitidos cortos.

## Fase 3: Calibración de Temperatura (Puntos de Referencia)
*Usar el termómetro físico de vidrio como patrón. El horno tiene un offset ajustable en Menú > Ajustes > Calibración.*

1.  **Punto Cero (Hielo):** 
    *   Sumerge la punta de la sonda y el termómetro de vidrio en un vaso con hielo picado y un poco de agua.
    *   Espera 2 minutos a que estabilice.
    *   Anota la diferencia. Si el controlador marca 2°C y el de vidrio 0.5°C, el offset inicial es de -1.5°C.
2.  **Punto Ebullición (Agua Hirviendo):**
    *   Sumerge ambos en agua hirviendo. 
    *   Ajusta el **Offset** en el menú hasta que la lectura del controlador coincida con el termómetro de vidrio (aprox 100°C dependiendo de la altitud).
3.  **Verificación de Ambiente:** Deja que la sonda vuelva a temperatura ambiente. Debe marcar lo mismo que el termómetro de vidrio en reposo.

## Fase 4: Prueba de Carga (Simulada y Real)
1.  **Simulación de Relé:** Sin conectar el horno a 220V, inicia un programa. Escucha el "click" del relé o observa su LED indicador.
2.  **Watchdog Térmico:** Inicia el calentamiento real. Verifica que en los primeros 10 minutos la temperatura suba consistentemente. Si no sube, el sistema debe entrar en **FALLO** automáticamente por seguridad.

## Fase 5: Backup Final
1.  Una vez calibrado y configurado el WiFi, inserta la tarjeta SD.
2.  Entra en ajustes y fuerza un guardado para que se cree el `config.bin` de backup.
3.  **Prueba de Fuego:** Apaga el equipo, quita el ESP32, pon uno nuevo, inserta la SD y enciende. El nuevo módulo debe heredar toda la configuración automáticamente.

---
*Si algún paso falla, NO procedas al siguiente. Consulta el `FSD.md` o el `GEMINI.md` para diagnóstico.*
