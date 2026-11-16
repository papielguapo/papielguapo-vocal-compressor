# Armónicos — VOCAL Compressor 2.3

Seno de 74 Hz, 48 kHz, Low Cut OFF, Gate OFF, Gain 0, FIX/Delta OFF. Medición FFT coherente de los últimos 2 s de una señal de 6 s. Valores relativos a la fundamental (dBc). No son mediciones de ruido analógico.

| Comp | H2 | H3 | Máximo H4–H10 |
|---|---:|---:|---:|
| 0 dB | sin aporte intencional | sin aporte intencional | < -140 dBc |
| -6 dB | -54.65 dBc | -62.26 dBc | -90.13 dBc |
| -12 dB | -45.47 dBc | -54.45 dBc | -87.60 dBc |
| -18 dB | -39.57 dBc | -49.28 dBc | -87.24 dBc |
| -24 dB | -36.66 dBc | -46.61 dBc | -86.58 dBc |
| -30 dB | -34.66 dBc | -44.79 dBc | -85.90 dBc |
| -36 dB | -33.06 dBc | -43.40 dBc | -84.78 dBc |

La coloración aumenta con Comp y la reducción real de ganancia. Se generan intencionadamente los términos cuadrático y cúbico; no se pretende generar H5/H7/H9. El motor completo, sus moduladores y la operación fuera del margen nominal pueden producir componentes adicionales: no se afirma exclusividad matemática de H2/H3 para cualquier señal.

El gain cell utiliza tres medias móviles en dominio logarítmico para reducir la modulación periódica. La corrección no lineal se procesa a 8x; el camino lineal se retrasa sin pasar por el filtro del sobremuestreador. Un Butterworth de 24 dB/oct a 20 Hz filtra la continua de la corrección, sin añadir ese filtro a la señal principal. Se reserva progresivamente hasta 3 dB de margen; Gain sigue controlando la salida. No es un limitador true peak.

Latencia total medida/declarada: 897 muestras a 48 kHz (18,6875 ms). Aumenta respecto a 2.2 por el suavizado de ganancia y el sobremuestreo; el DAW debe compensarla. Es relevante para monitorización en directo.

Prueba de dos tonos: 60 Hz + 7 kHz en relación de amplitudes 4:1, Comp -24, Gain -6, señal nominal. Métrica: suma cuadrática de seis bandas laterales 7 kHz ±60/120/180 Hz, relativa a 7 kHz. Versión anterior: -21,845 dB; versión 2.3: -28,502 dB. Mejora 6,658 dB en esa métrica concreta. Las bandas ±60 Hz crecen al añadir H2; las de ±120 Hz bajan. No significa ausencia de IMD ni mejora universal para cualquier material.

La etapa aislada supera las pruebas de H2/H3, rechazo de órdenes superiores, continua y alias de un seno de 10 kHz. También pasan las pruebas de Low Cut, FIX, Delta y estado. La equivalencia host/nativo en la señal de integración arroja diferencia máxima de muestra 0. El DSP cambia deliberadamente respecto a 2.2 y se aleja de una reproducción estricta de R-Vox. La GUI y los IDs de parámetros se mantienen.
