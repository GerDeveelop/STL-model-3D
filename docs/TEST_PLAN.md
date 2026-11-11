# Plan de pruebas de fase 0.1

> Resultado remoto 0.3.7: CI 11 en 15545e9 aprobó **32/32+113/113**, build/paquete, inventario/hashes y PATH mínimo. Completa aceptación CPU del diagnóstico/índice; caso sintético de 120 000 caras y comparación exhaustiva correctos. Sin benchmark del archivo privado ni aprobación gráfica de la nueva interfaz. [CI.md](CI.md).

> 0.3.7: preparados 32 casos generales y 113 de pintura/volumen/conectores/historial/visor/viabilidad/índice. Preliminar 109/112, tres igualdades de volumen detectaron acumulación con división por cara; corregida la suma. Primera ejecución final bloqueada con 4551 y abortada sin reintentos. Referencia Python del modelo privado es evidencia independiente, no C++ nuevo. Sin aceptación final ni revisión visual/publicación/CI nueva. [SURFACE_FEASIBILITY.md](SURFACE_FEASIBILITY.md).

> Resultado remoto 2026-11-10: CI 10 en a31f398 aprobó **32/32 + 92/92**, build/paquete e inventario/hashes con PATH mínimo. Incluye 13 casos de parámetros de historial y siete de normales del visor. CI 9 obtuvo 90/92 por dos datos sintéticos cuyo atributo pindex se alteraba accidentalmente; corregido el generador sin modificar el C++. Aceptación CPU completada; revisión gráfica del binario nuevo sigue pendiente. [CI.md](CI.md).

> 0.3.6: siete regresiones CPU del visor preparadas (32+92 casos totales). Primer intento de ejecutar el exe bloqueado por Windows 4551; no se lanzó la batería. Compilación y análisis matemático independientes completados, resultado visual del nuevo binario pendiente. No se confunde la captura original 0.3.5 ni el análisis del XML con una aprobación C++/GPU. [VIEWER.md](VIEWER.md).

> Paquete local 0.3.5 comprobado: **32+85 aceptaciones correctas**, PATH mínimo, carpeta con espacios/ñ e inventario/CRC/hashes verificados. Reportes paint-history-package-verification.json y paint-history-package-paint-verification.json dentro de build/. Sin revisión visual ni CI nueva; [HISTORY.md](HISTORY.md).

> Incremento local 0.3.5: Release y **85/85** casos inline correctos, con 13 nuevos de historial de parámetros sobre los 72 previos. Valores predeterminados, edición sin geometría/paso, ambos modos undo/redo, borrador, profundidad inactiva, rechazos, preservación de Redo/estado ante fallo y presupuesto, nueva rama. Reporte build/paint-history-inline-results.json. Sin revisión visual ni CI nueva; [HISTORY.md](HISTORY.md).

> Resultado remoto vigente 0.3.4: **32/32 + 72/72**, todas las etapas de CI 8 correctas sobre dbacc4a, incluido paquete con PATH mínimo. Completa la aceptación que quedó bloqueada localmente; no cambia esos resultados locales ni acredita revisión visual. Entorno, artefacto y hashes en [CI.md](CI.md).

> 0.3.4: batería preparada de **72 casos** de pintura/volumen/conectores, con 18 casos nuevos. **Bloqueada antes de ejecutar por WinError 4551; ninguna aceptación final**. Dos casos preliminares corresponden a una compilación anterior. Release, ZIP y sus comprobaciones estáticas completados; dos bloques Python del workflow analizados sintácticamente. Se mantienen 32 regresiones generales preparadas, no ejecutadas contra 0.3.4. Detalles en [REGION_CONNECTORS.md](REGION_CONNECTORS.md). Sin revisión visual ni nueva CI; 0.3.3 sigue siendo evidencia separada.

> Incremento local 0.3.3 (2026-11-09): Release y **54/54** aceptaciones de pintura/reconstrucción correctas (36 existentes y 18 nuevas). Incluyen separación/componentes, cavidades y profundidad del núcleo, exportación/reimportación de ambos modos, reflexión, undo/redo, preservación de Rehacer al fallar, colores ausentes/interpolados y límites. Modelos generados en memoria, sin fuentes/carpetas de pruebas; reporte `build/paint-volume-inline-results.json`. Paquete: **32/32** regresiones generales más **54/54** de pintura/volumen, PATH mínimo y carpeta con espacios/ñ; inventario y hashes verificados. Sin revisión visual ni nueva CI remota. Véase [PAINT_VOLUME.md](PAINT_VOLUME.md).

> Estado 0.3.2 publicado y validado en CI Windows: ejecución 7, commit d2a219a, build/paquete y 32 casos generales + 36 de pintura correctos. El bloqueo local 4551 no se ha cambiado; sin revisión visual. Insertos/partición volumétrica pendientes. Evidencia en [CI.md](CI.md); los resultados anteriores conservan su contexto histórico.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha: 2026-11-07. Estado: comprobaciones locales/CI realizadas; pendientes funcionales de fase 0.1 y validación del instalador final de fase 1.0. Segundo PC físico retirado por decisión del usuario.

Lector: 18 casos iniciales y 10 posteriores en memoria satisfactorios; contratos y evidencia en STL_IMPORT.md. Visor: framebuffer, órbita/zoom/F/Home comprobados directamente y paneo realizado por el usuario, sin corroboración visual independiente posterior. Se midieron cargas de 10k/100k/1M/2M y se observó cancelación durante recepción de 2M; detalles y límites en VIEWER.md y PERFORMANCE.md.

ZIP provisional: inventario/hashes, importación y dependencias comprobados localmente con PATH mínimo. CI remota: ejecución 4 satisfactoria con build, empaquetado, cinco casos inline y artefacto; véanse BUILD_AND_RELEASE.md y CI.md. B01/T11 ya no exigen otro PC. No se afirma CTest ni se crean archivos de pruebas propios.

Pendientes de fase 0.1: archivo real Unicode desde diálogo, preservación del origen, retención de malla previa ante fallo/cancelación, cancelación durante parser/preparación y demás comprobaciones funcionales no cerradas. El instalador .exe y su validación en VM Windows limpia pertenecen al cierre de fase 1.0; no se declaran implementados ni son condición de cierre de fase 0.1.

## Casos previstos

| ID | Caso | Resultado esperado |
|---|---|---|
| T01 | Cubo STL ASCII de 12 triángulos | Conteo, dimensiones y caja correctos |
| T02 | Mismo cubo en STL binario | Estadísticas equivalentes dentro de tolerancia acordada |
| T03 | STL binario con encabezado que empieza por `solid` | Importación correcta sin confundirlo con ASCII |
| T04 | Archivo vacío, truncado, sintaxis inválida o conteo inconsistente | Error controlado sin cierre de la aplicación |
| T05 | Coordenadas no finitas o archivo sobre los límites definidos | Rechazo controlado sin asignaciones excesivas |
| T06 | Malla abierta y triángulos degenerados | Comportamiento conforme a la política que se acuerde; sin prometer reparación |
| T07 | Ruta Unicode y con espacios | Apertura correcta y archivo fuente intacto |
| T08 | Visor con fixture conocido | Malla visible; órbita, zoom, paneo, encuadre, ejes y cuadrícula funcionales |
| T09 | Importación extensa y fallida | UI operativa y estado de la escena conforme a la política acordada |
| T10 | Fixtures de 10k, 100k y 1M triángulos | Registrar tiempos, RAM y hardware; objetivos pendientes de medición |
| T11 | Build Release y comprobaciones inline | Ejecución satisfactoria en el equipo de desarrollo y CI con el mismo preset; toolchains documentadas, sin exigir segundo PC ni infraestructura CTest |
| T12 | ZIP portátil provisional con runtime local | Arranque/importación/visor y dependencias del paquete comprobados localmente con PATH mínimo; comprobaciones aplicables del ZIP en CI |

## Evidencia y límites

Registrar por ejecución: commit cuando exista, fecha, equipo, Windows, CPU/GPU/RAM, herramientas y dependencias, comando, código de salida, resultado y log. Para el visor, añadir capturas y registro de interacción; una compilación no demuestra funcionamiento visual.

Los fixtures deben ser propios o tener licencia compatible documentada. Fijar dimensiones y tolerancias antes de automatizar las comparaciones. Hay ejecutable Release e informes de comandos bajo build/, sin fixtures ni archivos de pruebas propios. Los datos de comprobación se generan en memoria y se introducen por --inspect-stdin.

La VM Windows limpia es obligatoria para validar el instalador `.exe` final en fase 1.0. Puede utilizarse antes de forma opcional; no bloquea el cierre de fase 0.1. No se exige un segundo PC físico. Corte, volumen, exportación y reimportación se prueban desde fase 0.2.

## Aceptación final del instalador — fase 1.0

En una VM Windows limpia sin Qt, Python ni Visual Studio preinstalados, ejecutar `STLCutStudioAI-Setup-<version>-win-x64.exe`, instalar sus dependencias, abrir la aplicación instalada y comprobar las funciones entregadas y la desinstalación. El usuario no debe preparar dependencias manualmente. Registrar versión Windows, arquitectura, GPU/controlador, instalador y SHA-256, acciones, resultados y errores. Instalador y comprobación final permanecen pendientes; este documento no declara su ejecución.

## Condición de cierre de fase 0.1

Requisitos aplicables aprobados, decisiones técnicas documentadas, Release compilado en el equipo de desarrollo y CI, comprobaciones por comandos y visuales registradas y ZIP provisional con dependencias comprobado localmente con PATH mínimo y mediante verificaciones aplicables en CI. Los pendientes funcionales de importación, conservación del modelo y cancelación deben resolverse o quedar expresamente acordados antes del cierre. No se exige segundo PC ni instalador/VM limpia en esta fase. La entrega final 1.0 exige el instalador `.exe` y la validación en VM descrita arriba. Retirar el segundo PC no implica declarar cerrada la fase 0.1.

## Comprobaciones de fase 0.2

Ejecutadas localmente 48 comprobaciones inline de importación/ZIP/unidades/transformaciones/corte/exportación y reimportación, sin crear fuentes ni fixtures de pruebas. Detalle, tolerancias, evidencias y límites en PHASE_02.md. Verificación visible de plano, lados positivo/negativo y retención tras corte fallido. Guardado asistido por usuario desde el selector nativo y reimportación CLI de su salida prueba.stl; conservada fuera del commit. Cancelación de corte sobre 1 929 612 triángulos mediante botón y Ctrl+. comprobada, sin certificar cada etapa ni latencia máxima. El workflow incorpora diez casos de paquete, ejecutados satisfactoriamente de forma local con PATH mínimo; ejecución remota aún pendiente.

## Aceptación Manifold 0.2.1

63/63 casos inline locales, incluidos huecos múltiples/encerrados, cortes oblicuos, aristas completas y cara cóncava coplanar, tubo con 8800 intersecciones de contorno, volumen y reimportación con comprobación de aristas orientadas y huecos sin rellenar. 15/15 aceptaciones del ZIP con PATH mínimo. Revisión visible del tubo en ambos lados (24 → 12 + 12), retención tras plano tangente y cancelación Ctrl+. sobre 1 929 612 triángulos. Medición CLI: 13.3604 s, pico RSS 1350.70 MiB. Sin fuentes ni fixtures propios, sin CI remota nueva. Tolerancias de cálculo double y exportación float32 se verifican por separado, con límites en MANIFOLD_CUT.md.

## Aceptación local del historial 0.2.2

Release satisfactorio; 22/22 casos de historial, 63/63 de regresión geométrica/importación y 21/21 del paquete con PATH mínimo. Verificados IDs/procedencia, conservación de hermanas y volumen, restauración de selección/plano, ramas, exportación/reimportación, límites reales de 32 pasos y 256 piezas, rechazo transaccional y descarte por presupuesto. Interfaz visible: encadenamiento, Ctrl+Z/Y, rama alternativa y original de referencia. Evidencia y alcance de cancelación en [HISTORY.md](HISTORY.md). Sin nuevas fuentes/fixtures de pruebas. Posteriormente CI remota satisfactoria: ejecución 6 sobre 34b0cb7, build/paquete y 21 casos; véase CI.md.

## Validación de modelos reales

Seis importaciones, cinco cortes encadenados/undo/redo y quince exportaciones/reimportaciones ejecutados localmente; un modelo rechazado por degeneración conservó su estado. Trece STL cerrados/orientados y dos fallos de topología float32 en exportación CLI. La fase 0.2 no se declara cerrada. Método, métricas y pendiente concreto en [REAL_MODEL_VALIDATION.md](REAL_MODEL_VALIDATION.md).

## Aceptación de exportación 0.2.3

Release satisfactorio, 22 casos de historial y 63 de geometría/importación con el caso trasladado adaptado al rechazo float32; 23 del paquete. Las dos piezas reales inválidas ahora se rechazan sin alterar geometría/historial, incluida rama Rehacer. Trece exportaciones reales válidas siguen aceptadas. Detalles y límites en [EXPORT_VALIDATION.md](EXPORT_VALIDATION.md).

## Aceptación prevista de fase 0.3

Color/material, regiones conectadas, contactos, grosor, holguras, booleanas de conectores, exportación verificada y transacciones de historial según PHASE_03.md. Comprobaciones por comandos, sin revisión visual por petición del usuario. Los casos de separación por color todavía no se han ejecutado.

## Conectores al cortar: aceptación local 0.3.0

16/16 casos de conectores, 22/22 de historial y 25/25 del paquete satisfactorios. Regresión geométrica/importación en connector-geometry-regression.json. Sin revisión visual ni CI remota nueva. Detalles y condiciones pendientes de color en PHASE_03.md.

## Aceptación local 0.3.1

20 casos de color, 63 de geometría/importación, 22 de historial, 16 de conectores y 28 del paquete correctos. Seis importaciones privadas preservadas; separaciones no interpretables rechazadas sin modificar fuentes. Corregida una mezcla de ABI por dependencias /showIncludes localizadas; seguimiento Mesh.h comprobado. No se realizó revisión visual ni CI remota nueva. Evidencia, cambios intencionales y límites en COLOR_IMPORT.md.

## Pintura 0.3.2: aceptación pendiente

Preparada batería inline de 36 casos con 80 árboles de subdivisión, herencia, reflexión, CRC/DTD/límites y fuente intacta. La última ejecución fue bloqueada antes de arrancar por WinError 4551; no se declara aprobada. Hubo 32/36 aceptaciones preliminares antes de corregir cuatro casos del generador/límite del SDK. Workflow ampliado a 32 casos, sin ejecución final local ni remota. Detalle de evidencia y separación de resultados en PAINT_IMPORT.md.

## Resultado remoto 0.3.2

Ejecución 7 sobre d2a219a: **32/32** casos generales y **36/36** de pintura correctos, incluido el caso que genera 80 árboles y los límites de expansión/hojas/CRC/DTD. Build, paquete y artefacto satisfactorios. No se convierten las pruebas locales bloqueadas en aprobadas: esta evidencia corresponde al runner remoto y su toolchain. Enlaces y entorno en CI.md. Sin revisión visual ni aceptación del instalador final.
