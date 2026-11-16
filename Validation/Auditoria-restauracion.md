# Auditoría de la versión medida restaurada

Estado restaurado: código de Papis Compressor 2.0 anterior al prompt de arquitectura. Los cuatro archivos Processor/Editor coinciden por SHA-256 con el estado público previo. Recompilación Release y pruebas DSP ejecutadas correctamente tras restaurar. La arquitectura de 1,5 ms, HPF 85 Hz y clipper del prompt posterior queda descartada del código activo. No se ha modificado la GUI ni los IDs.

## Hallazgos abiertos

- **P1: falta equivalencia con R-Vox.** Las pruebas ampliadas del VST3 2.0 encuentran hasta 1,345 dB de diferencia a 10 kHz y 5,961 dB en recuperación 10 ms después de un pulso de 200 ms con Comp -36. Son casos concretos, no máximos globales.
- **P2: prueba de bypass insuficiente.** La comprobación usa RMS de un seno y no demuestra alineación temporal. Falta impulso/correlación y comprobación de transiciones.
- **P2: prueba estéreo insuficiente.** Solo compara canales con L=R. No detectaría dos compresores mono independientes. Falta señal asimétrica y no correlacionada.
- **P2: automatización del modelo.** Los coeficientes de recuperación se calculan por bloque desde Comp sin suavizar, mientras el makeup sigue Comp suavizado por muestra. Por inspección puede haber cambios del modelo dependientes del tamaño de bloque al automatizar; requiere medición y corrección antes de certificar ausencia de artefactos.
- **P2: metering de transitorios.** El audio usa historial suavizado de ganancia y retardo; el medidor usa reducción actual del detector. No corresponde exactamente a la atenuación aplicada a la muestra de salida durante transitorios.

## Alcance de las verificaciones

Suite DSP aprobada: silencio, salida finita, mono/estéreo a 44,1/48/96 kHz, efecto del gate y Comp, estado y buffers grandes. Las limitaciones de las pruebas de bypass/estéreo anteriores permanecen abiertas. No se certifica equivalencia 1:1 ni se extrapolan las cifras de curvas estáticas a voz o transitorios.

Se conservan los resultados experimentales anteriores. Esta auditoría es del motor medido 2.0 y no del motor descartado del prompt posterior. No se instala ningún plugin en el sistema durante la restauración.
