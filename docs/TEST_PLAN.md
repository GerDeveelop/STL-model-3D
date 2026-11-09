# Plan de pruebas de fase 0.1

> Estado vigente 0.2.2: cortes encadenados sobre piezas activas y Deshacer/Rehacer de sesión, con geometría compartida y presupuesto de historial. Uso, límites y evidencia local en [HISTORY.md](HISTORY.md). Se conserva Manifold 3.5.4. CI remota 0.2.2 satisfactoria en 34b0cb7 (ejecución 6); las secciones anteriores por versión son históricas.

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
