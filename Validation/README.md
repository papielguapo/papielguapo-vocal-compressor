# Comparación con R-Vox

Implementación independiente ajustada mediante mediciones de R-Vox Stereo instalado (Waves V14). No contiene código, binarios ni gráficos de Waves y no requiere Waves instalado.

176 puntos de meseta DC y seno de 1 kHz, niveles -72 a 0 dBFS, varias posiciones de Gate y Comp: error absoluto medio 0,00455 dB y máximo 0,07291 dB. Se midieron los últimos 200 ms de mesetas de 2 s; no es una garantía para señales arbitrarias.

Recuperación ajustada con pulsos DC de 20, 50, 100, 200, 500 y 1500 ms. Se verificaron señales sintéticas dinámicas y un fragmento de voz procedente de una prueba previa del FET. Las señales de voz no se redistribuyen.

| Voz: Gate / Comp / Gain | Error de nivel global | Mediana absoluta en ventanas 10 ms | Percentil 95 |
|---|---:|---:|---:|
| -80 / -12 / -3 | 0.068 dB | 0.105 dB | 0.260 dB |
| -40 / -24 / -6 | 0.352 dB | 0.366 dB | 1.009 dB |

Ventanas activas: salida de referencia por encima de -65 dBFS. Los JSON incluyen también residuo RMS entre formas de onda: las señales no se anulan. No se ha obtenido identidad 1:1. Persisten diferencias en recuperación, graves, frecuencias altas y compresión fuerte.

Pruebas DSP aprobadas: mono/estéreo a 44,1/48/96 kHz, silencio, salida finita, enlace estéreo, gate, compresión, bypass, guardado de estado y automatización con bloques de 4097 muestras. VST3 cargado en host y comparado con el ejecutable DSP: diferencia máxima de muestra 0 en la prueba de integración.

La limitación se integra en la curva de compresión. No hay control Lim separado ni limitador brickwall añadido. Con Comp en 0, las mediciones de R-Vox dejan pasar entradas superiores a 0 dBFS; este modelo reproduce ese comportamiento. No es true peak.
