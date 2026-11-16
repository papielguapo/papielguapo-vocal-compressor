# Pruebas ampliadas: Papis Compressor 2.0 frente a R-Vox

Se prueba el VST3 publicado 2.0, no la versión posterior basada en el texto de especificaciones. Seno estéreo con pico -6,02 dBFS; 48 kHz; mesetas de 3 s; medición del último segundo; gate OFF y Gain 0. Se compensa la latencia declarada de cada plugin.

| Frecuencia | Comp -12 | Comp -24 | Comp -36 |
|---|---:|---:|---:|
| 30 Hz | +0.124 dB | +0.196 dB | +0.119 dB |
| 60 Hz | +0.111 dB | +0.104 dB | +0.051 dB |
| 110 Hz | +0.051 dB | +0.034 dB | +0.014 dB |
| 200 Hz | +0.007 dB | +0.003 dB | +0.001 dB |
| 500 Hz | -0.002 dB | -0.003 dB | -0.003 dB |
| 1000 Hz | -0.009 dB | -0.014 dB | -0.016 dB |
| 3000 Hz | -0.083 dB | -0.119 dB | -0.130 dB |
| 5000 Hz | -0.285 dB | -0.414 dB | -0.453 dB |
| 10000 Hz | -0.862 dB | -1.234 dB | -1.345 dB |
| 18000 Hz | -0.333 dB | -0.483 dB | -0.529 dB |

Error positivo: Papis entrega más nivel. El JSON incluye el residuo RMS de las formas de onda, relativo a la referencia. Las señales no son idénticas; una coincidencia de nivel no demuestra igualdad sonora.
