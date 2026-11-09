# Validación con modelos reales — fase 0.2

Fecha local: 2026-11-08. Alcance aprobado: modelos existentes en lectura, cortes sucesivos, historial, exportación/reimportación y mediciones. Ejecutable portátil local 0.2.2, código publicado en 34b0cb7, SHA-256 `8bc46daf124e98ae41383effa38301d4a6597afce61c91b07fe18e8d8066d403`. No se modificó código ni se crearon fuentes/fixtures de pruebas.

## Muestra y método

Seis modelos privados existentes: tres STL de rutas Unicode y tres 3MF. Se identifican aquí como R1–R6; rutas, hashes, parámetros y resultados completos permanecen en el reporte ignorado `build/real-model-validation.json`. No se copian ni publican modelos ni sus nombres. SHA-256 antes/después idéntico para las seis fuentes. La salida previa del usuario `prueba.stl` también permanece intacta y fuera de Git.

Bytes recibidos por stdin, sin guardar STL de salida. Primer corte por el centro del eje de mayor extensión. En los cinco modelos admitidos, segundo corte sobre la pieza positiva por el centro del mayor eje restante, conservando la hermana negativa; después Deshacer/Rehacer y exportación de las tres piezas activas. Se comparan IDs, selección, plano/visibilidad, volumen total, conteos, escala, reimportación y parejas de aristas orientadas. No se repitió la interfaz ni el guardado nativo: su evidencia anterior está en HISTORY.md.

## Resultados

- 6/6 importaciones correctas, con dimensiones/unidades/advertencias registradas. STL sigue sin especificar unidades; 3MF se convierte a mm.
- 5/6 primeros cortes admitidos. R2 se rechaza con «Corte: triangulo sin area suficiente.»; su importación ya detecta un degenerado. Estado original #1 y cero pasos de historial conservados. No se reparó ni alteró la fuente.
- 5/5 secuencias admitidas: segundo corte, conservación de hermana, Deshacer/Rehacer con selección/plano/visibilidad, volumen total dentro de la tolerancia documentada. IDs finales 4/5/3, restaurados sin reutilización.
- 15/15 exportaciones CLI se serializan y reimportan, conservando conteos, dimensiones y volumen dentro de las tolerancias de PHASE_02/MANIFOLD_CUT.
- **13/15** conservan parejas de aristas orientadas. **Dos exportaciones de R6 (#5 y #3) fallan:** cuatro claves de arista fuera del criterio de dos caras opuestas y seis triángulos degenerados por pieza después de convertir a float32. Reimportar y conservar volumen no bastan para declarar una exportación válida.

## Hallazgo detectado en 0.2.2

`--edit-stdin` devuelve éxito al serializar esas dos piezas aunque su STL float32 no satisface cierre/topología. Su rama export usa StlWriter directamente, sin la validación posterior de la interfaz. No se presenta como un fallo del cálculo double ni se propone relajar tolerancias: los dos cortes y el historial pasan sus comprobaciones antes de la conversión.

La interfaz verifica precisión, cierre y volumen antes de QSaveFile para piezas cortadas; por lectura del código tiene protección frente a degeneraciones, pero no se ejecutó un guardado nativo de estas piezas y no se afirma ese resultado visual. Ajuste propuesto entonces: compartir esa validación de exportación entre interfaz y CLI, devolver un error explícito para una conversión inválida y conservar sesión/fuentes. La reparación automática de la salida requiere otra decisión; este incremento solo detecta/documenta el problema. La fase 0.2 permanece abierta.

## Mediciones locales

Windows 11 Pro 10.0.26300, Ryzen 7 5700X, 34 282 475 520 bytes de RAM detectados (aproximadamente 31.93 GiB). Paquete extraído en ruta con espacio/ñ, PATH limitado a Windows/System32. Sin Qt del entorno de desarrollo. GetProcessMemoryInfo aproximadamente cada 20 ms; se registra el mayor PeakWorkingSetSize observado en el proceso hijo. Los procesos breves tienen pocas muestras y podrían perder el último pico.

| Modelo | Formato | Triángulos importados | Importación (s) | Primer intento: s / pico RSS MiB | Secuencia: s / pico RSS MiB |
|---|---|---:|---:|---:|---:|
| R1 | STL | 36680 | 0.2786 | 1.2497 / 44.7 | 1.3708 / 44.5 |
| R2 | STL | 297808 | 0.2916 | 4.2508 / 87.82 | Rechazado |
| R3 | STL | 24 | 0.1397 | 0.1619 / 15.02 | 0.1427 / 12.61 |
| R4 | 3MF | 20124 | 0.1926 | 0.6266 / 32.4 | 0.9898 / 33.16 |
| R5 | 3MF | 61396 | 0.4252 | 1.8672 / 64.38 | 3.045 / 64.33 |
| R6 | 3MF | 778560 | 3.0543 | 28.7372 / 667.64 | 42.4764 / 665.25 |

La secuencia incluye importación, dos cortes, undo/redo y tres serializaciones base64, hasta recibir JSON. La reimportación y revisión Python de cada salida se ejecutan después y no están incluidas en esa columna. Lectura de archivos/generación de peticiones previas, GUI/GPU y proceso Python no se incluyen. Una medición por operación; no es un benchmark estadístico ni garantiza máximos. El primer intento de R2 mide un rechazo, no un corte completado. Los presupuestos de buffers/historial no son cuotas de RAM del proceso.

## Alcance de entrega

Validación y documentación locales, sin nueva compilación, dependencias, commit/push ni CI en este incremento. La CI 0.2.2 anterior sigue siendo evidencia de sus 21 casos sintéticos; no acredita estos archivos privados ni elimina el hallazgo de float32. El instalador final continúa en fase 1.0.

## Resolución posterior 0.2.3

Validación compartida implementada y compilada tras aprobación: ambas piezas R6 inválidas se rechazan; #4 y las doce exportaciones de R1/R3/R4/R5 siguen aceptadas. Conservados historial y Rehacer. Este documento mantiene los resultados de 0.2.2 como evidencia histórica; contrato y comprobaciones nuevas en [EXPORT_VALIDATION.md](EXPORT_VALIDATION.md).
