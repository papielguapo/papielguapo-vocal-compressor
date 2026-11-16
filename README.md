# Papis Compressor 2.0.0

Compresor independiente VST3 y aplicación para Windows x64, ajustado mediante mediciones de R-Vox. Conserva la skin metálica y las perillas moradas del FET 67.

![Papis Compressor](Preview.png)

[Descargar versión 2.0](https://github.com/papielguapo/papis-compressor/releases/tag/v2.0.0)

## Controles

- **Gate:** OFF a 0 dB. Expansión suave bajo el umbral para reducir ruido entre frases.
- **Comp:** 0 a -36 dB; girar a la derecha aumenta la compresión y la compensación de volumen.
- **Gain:** -36 a 0 dB; ajusta la salida de la curva de compresión y limitación.
- **Power:** efecto / señal seca con compensación de latencia.

Doble clic para escribir un valor en dB; Gate acepta OFF. Enter aplica y Escape cancela. Recuperación automática integrada. Se han retirado Release, SC HPF, SC/Auto, Meter y FIX/LIM. El medidor muestra reducción total y pico de salida.

## Instalación

Extraer el ZIP y copiar la carpeta completa `Binaries/Papis Compressor.vst3` a `C:/Program Files/Common Files/VST3/`. Volver a escanear en el DAW. La versión 2 sustituye Papis Compressor 1; el procesamiento cambia y se eliminan sus controles adicionales. FET 67 sigue siendo otro plugin.

## Precisión

Es una aproximación medida; no es un clon sonoro 1:1 demostrado. [Resultados y límites](Validation/README.md). No necesita R-Vox ni Waves instalado. No contiene recursos de Waves. La limitación forma parte de la curva; no es un techo brickwall ni true peak.

## Compilar

Requiere CMake, MSVC y JUCE 9.0.5, con la licencia JUCE correspondiente.

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH=C:/JUCE
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

`PapisTests --render entrada.wav salida.wav gate comp gain` permite renderizar audio para comparaciones; usar rutas absolutas. Sin argumentos ejecuta pruebas DSP.

## Estado restaurado y auditoría

Se mantiene el motor medido anterior al prompt de arquitectura. [Auditoría del estado restaurado](Validation/Auditoria-restauracion.md) y [pruebas ampliadas de frecuencia](Validation/Resultados.md). La foto superior corresponde a la interfaz de tres perillas de esta versión.
