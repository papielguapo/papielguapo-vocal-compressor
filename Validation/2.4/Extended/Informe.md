# Validación ampliada de 2.4.0 — 16/11/2026

No se ha cambiado el DSP ni el VST3 publicado. SHA-256 del binario instalado utilizado: `22c77d67d97bcf77cafc1f5fa2ffdd3dee9cd4a11459d4e05927bdec25db27f4`.

## Estabilidad prolongada

PASS: 45 minutos de audio simulado procesados sin esperas de tiempo real: 30 minutos a 48 kHz, 10 a 96 kHz y 5 a 192 kHz. Véanse Soak.csv y Soak.log para duración real y callbacks. Esta es una prueba acelerada del motor; no equivale a una sesión abierta durante 45 minutos en un DAW.

Estéreo, bloques variables 1/7/64/128/512/4097, cambios de Gate/Comp/Gain/Low Cut/Power/Bypass/DELTA/GR-OUT/FIX, silencios, picos de entrada de 12, NaN/Inf y ciclos de guardado/restauración de los nueve parámetros cada 30 segundos de audio.

Todos los resultados finitos y acotados; cero llamadas C++ new/new[] detectadas dentro de los callbacks vigilados. No se intercepta todo malloc. Los picos de 12 en Soak.csv corresponden a bypass, que conserva la señal seca saneada; no representan una sobrecarga del camino activo protegido.

Memoria privada observada durante el tramo monitorizado: 3,051,520–3,284,992 bytes, en 69 muestras. No se observa crecimiento continuo en ese tramo; no es una demostración general de ausencia de fugas. Los datos de memoria se tomaron después de arrancar la prueba y mientras se ejecutaban las exportaciones.

Para reproducir la prueba del motor, compilar PapisTests y ejecutar:

```
PapisTests.exe --soak C:/ruta/absoluta/Soak.csv
```

## Exportación con voz

Fragmento vocal local repetido hasta 600 segundos, 44,1 kHz, pico de entrada normalizado a 0,7, canal derecho = 0,37 × izquierdo; no se publica la grabación ni sus renders.

- Motor nativo y VST3 instalado en Common Files: Gate -40, Comp -24, Gain -6, Low Cut 80 Hz, FIX/DELTA OFF. Ambos renders contienen 26.482.050 frames (600,5 segundos, incluida cola). Diferencia máxima muestra a muestra: **0**. Pico **-1,433619 dBFS**.
- Segundo render del VST3 instalado: Gain 0 y FIX ON; demás controles iguales. Todo el archivo finito y pico máximo **-0,300000 dBFS**.
- Tres exportaciones completas, sin error de escritura ni caída del proceso. Datos y logs en Exports.json y Export-*.log. Se empleó un host JUCE VST3 de pruebas con bloques de 512, retirando la latencia declarada. No son exportaciones realizadas desde FL Studio o Cubase.

## Comprobaciones que siguen pendientes

Reproducción y exportación dentro de FL Studio y un segundo DAW, y sesiones prolongadas en tiempo real con sus dispositivos de audio. El escritorio estaba siendo utilizado por otra prueba, «Vocal Rider Audit 133», y el controlador devolvía solicitudes simultáneas. Se solicitó dejarlo libre antes de intervenir. No se ha modificado ese proyecto ni su diálogo de guardado. Cubase tampoco se cuenta como validado. La comprobación anterior de controles y recuperación de estado en FL Studio sigue documentada en el informe principal.

La protección continúa siendo de pico de muestra, no true peak; no se ha añadido un modo true peak ni se ha certificado ausencia de picos entre muestras.
