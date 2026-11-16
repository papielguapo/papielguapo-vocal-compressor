# Auditoría 2.4.0 — Windows x64

## Cambios comprobados

- Protección de pico de muestra enlazada estéreo, después de Gain/FIX, techo -0,3 dBFS y anticipación de 1 ms. No es true peak. Bypass conserva la señal seca; las transiciones desde bypass no prometen ese techo. DELTA reduce su escucha si el residuo supera el techo; en ese caso no sirve para una reconstrucción por suma exacta.
- Low Cut Butterworth 24 dB/oct, 0 OFF a 400 Hz, sin el suelo anterior de 5 Hz. A 1 Hz, atenuación medida en su frecuencia de corte: -3,00799 dB.
- Coeficientes de Comp controlados por el valor suavizado. Automatización con eventos cada 4096 muestras: diferencia máxima 0 entre bloques de 1, 7, 64, 128, 512 y 4097 muestras.
- GR toma la ganancia aplicada de la celda, el gate, la reserva de margen y la protección final, con alineación de los retardos. Excluye Gain/FIX y no mide la modificación instantánea de amplitud de los armónicos. Tiene caída visual de 250 ms.
- Sobremuestreo por bloques y detector de máximo con cola, sin búsqueda completa por muestra. Latencia declarada y comprobada por impulso a 44,1/48/88,2/96/192 kHz; a 48 kHz pasa de 897 a 561 muestras (18,6875 a 11,6875 ms).

## Pruebas

Suite DSP y avanzada aprobadas: mono/estéreo, cinco frecuencias de muestreo, seis tamaños de bloque, automatización, impulso, salida finita, techo después de FIX/DELTA, estado, bypass, estéreo y filtros. Cero llamadas C++ new/new[] detectadas en los callbacks vigilados; no es un rastreador de todo malloc del proceso.

pluginval 1.0.4: SUCCESS, nivel 10, semilla 20261116, 44,1/48/96/192 kHz, bloques 1/7/64/128/512/4097. Incluye editor, estado, automatización y fuzz. El validador oficial independiente de Steinberg no se ejecutó.

Rendimiento local: 1000 callbacks medidos por caso después de 200 de calentamiento. Véase Performance.csv: media, p99, máximo y presupuesto. Incluye planificación de Windows; el consumo depende del equipo y no garantiza ausencia de cortes. Una primera medición mientras corrían otras pruebas registró 1337,2 microsegundos para un presupuesto de 1333,3; en la repetición aislada todos los máximos quedaron dentro del presupuesto. No se demuestra una reducción porcentual frente a 2.3 por no disponer de un benchmark equivalente de esa versión.

## Comparación espectral con 2.3

Host VST3, 48 kHz, estéreo, 6 segundos; FFT rectangular de los segundos 4–6, Comp -24, Gate OFF y Low Cut OFF. Datos en Spectral.json.

- Seno 74 Hz, pico de entrada 0,25, Gain 0, FIX OFF: H2 -40,44 dBc y H3 -52,84 dBc; H4–H10 por debajo de -93 dBc. Son condiciones concretas, no exclusividad de H2/H3 para cualquier audio.
- Dos tonos 60 Hz (0,8) y 7 kHz (0,2), Gain -6: suma de seis bandas laterales / 7 kHz -28,48 dB frente a -28,50 dB en 2.3; no hay mejora de IMD significativa en ese caso.
- Seno 74 Hz a pico 1, Gain 0, FIX ON: máximo de todo el render +0,649 dBFS en 2.3; -0,300 dBFS en 2.4.

La reducción de latencia modifica transitorios: el pico del ensayo de seno sin FIX cambia de -2,32 a -1,15 dBFS y el de dos tonos de -6,85 a -5,02 dBFS, aunque los armónicos en régimen sean próximos. No es una actualización idéntica muestra por muestra.

## Comprobación en DAW

FL Studio 2026: carga del VST3 instalado en Common Files, apertura del editor, Low Cut 80 Hz introducido mediante doble clic y teclado, DELTA/GR-OUT/FIX activados, guardado y reapertura de una copia de proyecto. Los cuatro valores se recuperan correctamente. Evidencia en FL-Studio-state.txt y FL-Studio.png. La sesión de prueba no contenía audio en reproducción; la validación de audio se realizó en el host VST3 y en las suites automáticas. No se afirma una sesión de mezcla prolongada ni exportación validada en FL Studio.

Cubase 14 se lanzó, pero no llegó a un proyecto durante la comprobación inicial de licencias. No se cuenta como DAW validado. No se han validado otros DAW en esta entrega.

## Alcance

Motor independiente con coloración propia, no clon 1:1 de R-Vox. Mantiene identidad VST3 e IDs. El techo por muestras no garantiza picos entre muestras. Las pruebas no equivalen a certificar todos los DAW, equipos ni sesiones de larga duración.


Actualización posterior: se completó una pasada de dos minutos con voz centrada en Ableton Live 12.4.5, controles, guardado/reapertura y exportación. Véase el [informe de Ableton](Ableton-Live12/Informe.md) para resultados, incidencia de accesibilidad y límites; esto no sustituye las pruebas pendientes de otros DAW o sesiones prolongadas.
