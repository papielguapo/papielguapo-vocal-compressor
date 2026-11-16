# PAPIELGUAPO - VOCAL Compressor 2.4.0

Compresor vocal independiente VST3 y aplicación para Windows x64. Skin metálica y perillas moradas del FET, con el nombre **VOCAL Compressor** en el panel.

![VOCAL Compressor](Preview.png)

[Descargar Windows x64](https://github.com/papielguapo/papielguapo-vocal-compressor/releases/tag/v2.4.0)

## Versión 2.4

Protección de picos de muestra **-0,3 dBFS después de FIX**, automatización suavizada, Low Cut real desde 1 Hz y procesamiento por bloques. Latencia a 48 kHz: **561 muestras / 11,69 ms**, frente a 897 / 18,69 ms en 2.3; declarada al DAW y alineada también en bypass y Delta.

Comp aporta H2/H3 progresivamente, con sobremuestreo 8× y retirada de continua a 20 Hz / 24 dB/oct sobre la corrección. Comp 0 no añade coloración intencional. La protección puede actuar incluso con Comp 0 si se supera el techo. No hay controles adicionales. La respuesta transitoria cambia respecto a 2.3.

[Auditoría, rendimiento y mediciones 2.4](Validation/2.4/Informe.md). [Mediciones históricas de coloración 2.3](Validation/Harmonics-2.3/Informe.md).

## Perillas

- **Low Cut:** 0 (OFF) a 400 Hz, Butterworth de 24 dB/octava. Filtra el audio antes de Gate y Comp.
- **Gate:** OFF a 0 dB; expansión suave para reducir ruido entre frases.
- **Comp:** 0 a -36 dB. Girar a la derecha aumenta compresión y compensación de nivel.
- **Gain:** -36 a 0 dB; nivel de salida.

**Doble clic en una perilla o sobre su lectura numérica** para introducir el valor. Low Cut acepta Hz; las demás, dB. Se admite coma decimal, punto decimal y el sufijo Hz/dB. Gate y Low Cut aceptan OFF. **Enter** aplica; **Escape** cancela. Los valores fuera de rango se limitan al rango de la perilla; entradas no numéricas se señalan en rojo.

## Botones bajo el medidor

De izquierda a derecha: **DELTA · GR/OUT · FIX**. Power permanece a la izquierda.

- **DELTA:** escucha la diferencia entre la señal seca alineada y la procesada, incluyendo Low Cut, Gain y FIX. Sirve para inspeccionar qué cambia el procesamiento; no es una escucha exclusiva de reducción del compresor. Transición de 20 ms. Su escucha se atenúa si el residuo supera -0,3 dBFS; en ese caso no permite reconstrucción exacta por suma.
- **GR/OUT:** alterna el medidor entre reducción de ganancia y pico de salida en dBFS. Solo cambia la visualización. GR incluye la celda de ganancia, gate, reserva de margen y protección final; excluye Gain/FIX y el cambio de amplitud de la coloración. Tiene caída visual de 250 ms.
- **FIX:** Auto Gain RMS adaptativo enlazado estéreo (±24 dB). Compara la entrada seca con la salida procesada, incluido Gain, y compensa su nivel. Aprende con ventanas de energía de 400 ms y aplica compensación suavizada durante aproximadamente 1 s. Mantiene el ajuste durante silencio y con Gate cerrado. Puede necesitar varios segundos para estabilizarse y no es normalización LUFS. La protección de picos actúa después de FIX.

Power y el bypass del host conservan la señal seca original, sin Low Cut, con latencia compensada.

## Instalación

Extraer el ZIP y copiar la carpeta completa `Binaries/PAPIELGUAPO - VOCAL Compressor.vst3` a `C:/Program Files/Common Files/VST3/`. Retirar la copia anterior `Papis Compressor.vst3` para evitar dos archivos con la misma identidad VST3. Volver a escanear en el DAW.

La identidad interna y los IDs existentes se conservan para sesiones antiguas. Los nuevos botones abren apagados cuando no existen en el estado guardado. Low Cut abre en OFF en sesiones anteriores a 2.1. Los históricos 1.0/2.0/2.1 se conservan en Releases bajo el nombre anterior.

## Validación y alcance

Pruebas DSP y avanzadas aprobadas: mono/estéreo, 44,1/48/88,2/96/192 kHz, bloques 1–4097, automatización, estado, bypass, filtros, techo e impulso de latencia. pluginval 1.0.4 **nivel 10: SUCCESS**. [Resultados completos y limitaciones](Validation/2.4/Informe.md).

Implementación independiente inicialmente ajustada mediante mediciones de R-Vox, ahora con coloración y protección propias. No se ha demostrado identidad 1:1 y no requiere Waves ni contiene su código o recursos. La protección es por pico de muestra, **no true peak**. Los [resultados históricos](Validation/README.md) corresponden a las versiones indicadas.

## Compilar

Requiere MSVC, CMake y JUCE 9.0.5 con la licencia correspondiente.

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH=C:/JUCE
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

El ejecutable `PapisTests` conserva su nombre interno y ejecuta las pruebas DSP.

[Validación ampliada: 45 minutos de audio simulado y exportaciones de voz de 10 minutos](Validation/2.4/Extended/Informe.md). Para ejecutar la prueba prolongada: `PapisTests.exe --soak C:/ruta/absoluta/Soak.csv`.
