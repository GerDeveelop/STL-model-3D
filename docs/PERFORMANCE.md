# Validación de carga y memoria — visor 0.1

Fecha: 2026-11-07. Validación local autorizada, sin archivos STL ni fuentes de pruebas. No constituye una garantía de rendimiento ni completa la aceptación de fase 0.1.

## Entorno y método

Windows x64, AMD Ryzen 7 5700X (8 núcleos), 34282475520 bytes de RAM física (aproximadamente 31.93 GiB), NVIDIA GeForce RTX 3060 Ti de 8192 MiB y controlador 610.88. Build Tools 18.8.2, MSVC 19.51.36252.0, SDK 10.0.26100.0, Qt 6.8.3, CMake 4.3.1-msvc1 y Ninja 1.13.2. Build Release satisfactorio sin warnings C++ detectados; log `build/performance-build.log`.

Se generaron superficies trianguladas en memoria, con alturas sinusoidales y triángulos distribuidos espacialmente. No se usaron millones de copias superpuestas de un mismo triángulo, para evitar medir principalmente sobrepintado. Cada STL binario tenía exactamente 10k, 100k, 1M o 2M triángulos, con entradas de 500084, 5000084, 50000084 y 100000084 bytes respectivamente. Generación anterior al inicio del proceso medido; recepción mediante --view-stdin.

Una ejecución por tamaño, proceso nuevo por carga, ventanas mostradas sin activación ni entradas de ratón/teclado. Se muestrearon contadores de memoria del proceso aproximadamente cada 10 ms y se mantuvo cada proceso unos dos segundos después de registrar envío del buffer. Después, el coordinador terminó únicamente ese proceso sintético: el código 1 registrado corresponde a esa limpieza con TerminateProcess, **no** a un crash ni a una salida normal de la aplicación. El cierre normal se verificó por separado durante la cancelación.

Había otras aplicaciones activas y cambios de foco del usuario. No se controlaron frecuencia CPU/GPU, cachés, temperatura o carga de fondo. No se hicieron repeticiones estadísticas ni mediciones de FPS. Los tiempos son observaciones puntuales en este equipo.

## Resultados

Tiempos en milisegundos; memoria en MiB (1048576 bytes).

| Triángulos | Recepción + análisis | Preparación de vértices | Envío buffer GPU | Inicio a envío observado | Pico RAM residente | Pico memoria comprometida |
|---:|---:|---:|---:|---:|---:|---:|
| 10000 | 14.04 | 0.88 | 2.75 | 766.72 | 126.79 | 147.70 |
| 100000 | 40.23 | 5.16 | 11.64 | 818.80 | 154.75 | 178.41 |
| 1000000 | 215.44 | 41.10 | 86.17 | 1231.65 | 412.32 | 519.16 |
| 2000000 | 573.42 | 124.80 | 199.97 | 1696.01 | 690.94 | 862.32 |

Los cuatro tamaños completaron análisis, preparación y asignación/envío del buffer sin errores registrados. La malla de 10k también se observó dibujada en ventana visible con conteo y dimensiones correctos.

- Recepción + análisis incluye transferencia por tubería, copia en memoria y parser; no mide lectura desde disco.
- Preparación incluye construcción de la copia float32 y normales de presentación en el hilo de importación.
- Envío GPU mide tiempo CPU de asignación/envío y comprobación inmediata de errores. No espera una fence ni mide finalización GPU o tiempo por frame. La subida se ejecuta en la UI y puede pausarla durante ese intervalo; no se afirma ausencia de pausas a 2M.
- Inicio a envío observado incluye arranque de Qt/controlador y sondeo del log. No debe sumarse a los tiempos de etapas ni interpretarse como tiempo de parser.
- Pico RAM residente usa PeakWorkingSetSize; pico comprometido usa PeakPagefileUsage. Son contadores del proceso Qt, no del productor Python ni de todo el equipo; memoria comprometida no equivale a bytes actualmente escritos al disco.
- Los buffers de vértices contienen 720000, 7200000, 72000000 y 144000000 bytes (0.69, 6.87, 68.66 y 137.33 MiB). Son tamaños lógicos, **no** mediciones de VRAM física máxima. El controlador puede usar memoria adicional.

## Cancelación y revisión visible

Se inició otra entrada binaria de 2M triángulos, ralentizando su productor. Se observaron los estados Importando y posteriormente Importación cancelada en la ventana visible. Antes del cierre se habían enviado 12320768 de 100000084 bytes: la entrada aún no había finalizado. El visor no publicó una malla incompleta y después cerró por su control Cerrar con código 0. No se midió latencia exacta de cancelación y esta comprobación cubre **recepción**, no cancelación dentro del parser, preparación de vértices o asignación GPU. La escena estaba vacía; no demuestra retención de una malla previa.

La interacción física simultánea provocó cambios de foco y capturas de otras ventanas durante los intentos de cámara. Se descartaron esas observaciones para validar órbita/paneo/zoom y se evitó seguir enviando entradas globales. Se confirmó presentación visible de la malla de 10k; los controles conservan la evidencia previa de eventos dirigidos y framebuffer de [VIEWER.md](VIEWER.md), pero la sesión manual completa sigue pendiente.

En la sesión interrumpida anterior también se observó un STL real con caracteres chinos en su ruta. No se controló su apertura ni se midió antes/después, por lo que no se da por cerrada la prueba del diálogo Unicode o de preservación del archivo.

## Diagnóstico y evidencias

Se añadieron métricas con QElapsedTimer, sin dependencias nuevas ni cambios en los contratos geométricos:

- `STL_IMPORT_METRIC`: JSON con triangles, input_ms, presentation_ms y presentation_bytes, emitido al completar importación y preparación.
- `STL_GPU_UPLOAD_METRIC`: bytes y submit_ms, emitido cuando la asignación gráfica completa sin error inmediato.
- `QT_FORCE_STDERR_LOGGING=1`: variable estándar Qt habilitada únicamente en los procesos medidos para recoger esos mensajes en stderr. No se modificaron variables persistentes ni se requieren API keys.

Datos: `build/performance-results.json`, logs `build/perf-<triangles>-stderr.log`, `build/cancel-large-result.json` y `build/cancel-large-stderr.log`. Los informes permanecen excluidos de Git. En los logs apareció un aviso de Qt sobre el orden de configuración HighDPI; no impidió las cargas. No se identificó su origen y no se aplicó un cambio especulativo.

Primeros intentos descartados: una sesión interrumpida por Escape, una carga visible cuyo foco/datos cambiaron durante interacción y un intento con ventanas completamente ocultas que analizó la malla pero no recibió pintura OpenGL. La tabla usa solamente la serie final de cuatro procesos con ventanas no activadas y métricas completas.

## Revisión visible tras liberar el escritorio

El usuario confirmó disponibilidad del escritorio y se abrió una nueva malla de 12 triángulos por memoria. Se verificaron mediante acciones directas de Computer Use y capturas posteriores:

- Órbita por arrastre izquierdo: orientación del modelo y del indicador de ejes cambiaron.
- Zoom por rueda: el modelo aumentó de tamaño en pantalla.
- F: encuadró el modelo conservando la orientación.
- Home: recuperó la orientación y encuadre iniciales.
- Las estadísticas permanecieron en 12 triángulos, caja [0,0,0]–[10,20,30] y dimensiones 10×20×30.

No se detectó un fallo en esas acciones. Para el paneo derecho se solicitó una comprobación asistida porque la API de arrastre disponible solo admite botón izquierdo. El usuario confirmó que realizó el arrastre derecho. Las capturas posteriores no aislaron correctamente la ventana del visor, por lo que esa acción se registra como realizada por el usuario, sin corroboración visual independiente de su desplazamiento en esta sesión. Se mantiene además la evidencia previa de paneo por eventos dirigidos y framebuffer de VIEWER.md. Log de la sesión: `build/manual-camera-stderr.log`. No se escribieron fuentes ni STL de pruebas ni se modificó código en esta revisión.

Pendientes: corroboración visual independiente del paneo asistido, archivo real Unicode desde diálogo, preservación/retención de malla previa, cancelación durante parser/preparación, renderizado sin GPU compatible, picos de VRAM reales, mediciones repetidas y desde disco. CI y ZIP provisional están verificados; la entrega final exige instalador .exe y VM Windows limpia en fase 1.0, sin segundo PC físico. No se crearon archivos de pruebas propios; la subida posterior del código está registrada en CI.md.

## Corte Manifold 0.2.1 — 2026-11-08

Una ejecución CLI Release con 1 929 612 triángulos (cubo subdividido generado en memoria), Manifold 3.5.4 en modo serial: 13.3604 s desde inicio del proceso hasta STL stdout, pico RSS 1350.70 MiB y commit 1363.88 MiB, 445 muestras mediante GetProcessMemoryInfo aproximadamente cada 20 ms. Incluye recepción/análisis, adaptación, corte/consolidación/validación y exportación; no la generación Python previa ni GUI/GPU. El resultado positivo se consolida a 12 triángulos y volumen 0.5: caso plano favorable, no representativo de cualquier malla compleja. El presupuesto de 512 MiB del adaptador no es una cuota de RAM del proceso. Reporte build/manifold-performance.json y contrato en MANIFOLD_CUT.md. No se hicieron repeticiones estadísticas ni se garantiza tiempo/memoria máximos.
