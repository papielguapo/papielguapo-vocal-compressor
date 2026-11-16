# VOCAL Compressor 2.4 — prueba en Ableton Live 12

**Comprobación funcional completada en Ableton Live 12 Suite 12.4.5, Windows x64, el 16/11/2026.** Se cargó el VST3 instalado en Common Files, se reprodujo una voz centrada de dos minutos, se comprobaron DELTA y GR/OUT, se guardó y reabrió el proyecto y se exportó su salida desde Live.

## Condiciones

Una sola instancia de PAPIELGUAPO - VOCAL Compressor 2.4.0 en la pista de voz. Gate -40 dB, Comp -24 dB, Gain 0 dB, Low Cut 80 Hz, FIX ON; al terminar DELTA y GR/OUT apagados, Power ON y bypass OFF. Los nueve valores se verificaron también en el estado del plugin dentro del archivo ALS guardado. No hay Vocal Rider en esta sesión final.

Entrada estéreo de 120 segundos a 48 kHz/32 bits, con ambos canales idénticos. Warp OFF, inicio del archivo en cero, clip desde 1.1.1 hasta 61.1.1 a 120 BPM. La muestra se recopiló dentro del proyecto mediante «Recopilar todo y guardar».

Reproducción observada durante una pasada completa de dos minutos; el transporte siguió funcionando al alternar DELTA y GR/OUT. Se observaron valores del medidor de CPU de Live de aproximadamente 10–13 %, que incluyen el host y su interfaz. No son una medición aislada del consumo del plugin ni un contador exhaustivo de cortes. El registro de dispositivo indicaba búfer de entrada 8192 y salida 4096 muestras; esta pasada no valida monitorización a 64/128 muestras ni sesiones de varias horas.

## Exportación y medición

Exportación desde Main: WAV estéreo float32, 48 kHz, 60 compases / 120 segundos, sin normalización y sin renderizar como loop. Se obtuvo un archivo de 5.760.000 frames y todas sus muestras son finitas.

| Comprobación | Resultado |
|---|---:|
| Pico máximo | -0,300000 dBFS |
| Diferencia máxima L/R | 0 |
| Diferencia máxima frente a la referencia del mismo VST3 | 0,000009281607 |
| Residuo RMS frente a esa referencia | -144,896 dBFS |
| Diferencia máxima donde la referencia supera -80 dBFS | 0 |

La comparación se hace con el mismo plugin instalado, renderizado mediante un host JUCE de pruebas y con los mismos controles; **no es una comparación con R-Vox**. Se comparan los primeros 120 segundos de la referencia, que añade medio segundo de cola. El residuo total impide afirmar identidad matemática de ambos archivos; las diferencias medidas quedan en muestras de nivel muy bajo. No se ha determinado su causa exacta.

## Incidencias y repetición

Durante el primer intento, escribir la posición del transporte mediante UI Automation provocó un cierre de Live. Su registro contiene una aserción de `UiFramework/Exp/AxVariantI.h` y Windows registra `ucrtbase.dll / c0000409`. Esto apunta al control de accesibilidad como desencadenante; no constituye una atribución definitiva del cierre. Se recuperó el proyecto y se repitió usando los controles normales del transporte, completando reproducción, reapertura y exportación sin otro cierre en esta pasada.

También hubo interferencias de otro chat que añadió Vocal Rider. Se reabrió el proyecto propio y se verificó una única instancia de VOCAL Compressor antes del ensayo final. El archivo inicial asimétrico tenía R=0,37 L para probar el enlace estéreo; se sustituyó por una voz centrada después de que el usuario señalase el desequilibrio. El WAV final conserva L=R exactamente.

## Archivos

El proyecto y la voz exportada se entregan localmente en `outputs/Ableton-VOCAL-2.4`. La sesión actual incluye su muestra en `Samples/Imported`; requiere el VST3 instalado. En GitHub solo se publican este informe y las métricas, sin la grabación vocal ni sus renders.

El binario del compresor no cambia por esta validación. La protección sigue siendo de pico de muestra a -0,3 dBFS; no es true peak. Esta prueba acotada no certifica todos los equipos, dispositivos, tamaños de búfer o sesiones prolongadas.
