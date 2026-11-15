# Papis Compressor 1.0.0

Compresor VST3 y aplicación independiente para Windows x64, basados en el motor FET 67 v1.2.3. Panel metálico y perillas moradas, con Gate, Comp, Gain, Release y filtro de sidechain.

![Papis Compressor](Preview.png)

[Descargar para Windows x64](https://github.com/papielguapo/papis-compressor/releases/latest)

## Controles

- **Gate**: umbral de expansión descendente, OFF (-96) a 0 dB. Detección enlazada estéreo, apertura suave y cierre progresivo.
- **Comp**: girar a la derecha aumenta la compresión; umbral de compresión de -36 a 0 dB; bajar aumenta la compresión y la compensación automática. Motor feedback FET 4:1, ataque 0,2 ms, sobremuestreo 8x.
- **Gain**: salida y techo de pico de muestra, -36 a 0 dB. El techo protege las muestras de salida; no es un limitador true peak ni asegura ausencia de picos entre muestras.
- **Release**: 100 a 1200 ms. En el extremo AUTO, la recuperación depende de la reducción reciente (120–1020 ms).
- **SC HPF**: OFF a 220 Hz; afecta al detector de compresión, conserva graves en el audio. El gate detecta la entrada principal.
- **SC / AUTO**: ON utiliza automáticamente la entrada sidechain si está habilitada en el DAW; OFF usa detección interna. Sin conexión de sidechain se usa detección interna.
- **Meter**: GR muestra reducción del gate y compresor; OUT muestra RMS de salida con referencia 0 = -18 dBFS. GR no incluye la reducción del limitador de seguridad.
- **FIX / LIM**: LIM usa compensación fija asociada a Comp; FIX añade ajuste RMS adaptativo respecto a la entrada (±24 dB). Ambos conservan el techo de seguridad de Gain.
- **Power / bypass del host**: señal seca con latencia compensada y transición suave.

Doble clic en una perilla para introducir un valor numérico, OFF o AUTO cuando corresponda; Enter aplica y Escape cancela. Automatización, guardado de estado, mono/estéreo y ventana redimensionable.

## Instalación

Copiar la carpeta completa `Binaries/Papis Compressor.vst3` a `C:/Program Files/Common Files/VST3/` y volver a escanear en el DAW. La identidad del plugin es independiente de FET 67, que permanece instalado. `Papis Compressor.exe` es la aplicación independiente.

## Alcance

Implementación propia sobre el motor FET 67, con controles inspirados en R-Vox y en una referencia visual. No contiene código ni recursos gráficos de Waves. No se ha medido ni demostrado equivalencia sonora 1:1 con R-Vox. El circuito de gate, la compensación, el release automático y la protección de picos son aproximaciones propias.

## Compilar

Requiere CMake, MSVC y JUCE 9.0.5 (licencia JUCE según corresponda). Configurar `JUCE_PATH` con el checkout de JUCE:

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH=C:/JUCE
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

`PapisTests` verifica DSP, guardado de estado y puede generar una captura de la interfaz al recibir una ruta PNG absoluta.
