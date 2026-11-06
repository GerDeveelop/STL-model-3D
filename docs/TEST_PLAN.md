# Plan de pruebas de fase 0.1

Fecha: 2026-11-05. Estado: plan; ninguna de estas pruebas ha sido ejecutada.

Actualización 2026-11-06: compilación Release x64 con Qt 6.8.3 y configuración MSVC fuera del aislamiento correctas. Arranque oculto, ventana identificada, cierre WM_CLOSE y salida 0 comprobados en línea. Sin revisión visual. Evidencias en WINDOWS_SETUP.md y CHANGELOG.md. Solo cubren parcialmente B01/T11; CI y segundo equipo pendientes. Por instrucción del usuario no se crean carpetas ni archivos de pruebas. Actualización del importador: 18 casos iniciales y 10 posteriores en memoria satisfactorios, además de GUI oculta con ruta inexistente; detalles y alcance en STL_IMPORT.md. Los casos siguientes tienen cobertura parcial del lector, pero no se declaran cerrados porque falta apertura GUI de archivo válido, cancelación real, revisión visual y validación externa.

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
| T11 | Build y CTest Release | Ejecución satisfactoria en dos equipos y CI con el mismo preset |
| T12 | Paquete inicial en otro Windows sin IDE | Arranque, apertura STL y visor funcionales con runtime incluido |

## Evidencia y límites

Registrar por ejecución: commit cuando exista, fecha, equipo, Windows, CPU/GPU/RAM, herramientas y dependencias, comando, código de salida, resultado y log. Para el visor, añadir capturas y registro de interacción; una compilación no demuestra funcionamiento visual.

Los fixtures deben ser propios o tener licencia compatible documentada. Fijar dimensiones y tolerancias antes de automatizar las comparaciones. Hay ejecutable Release e informes de comandos bajo build/, sin fixtures ni archivos de pruebas propios. Los datos de comprobación se generan en memoria y se introducen por --inspect-stdin.

La VM limpia es parte de la validación final de distribución. Se recomienda usarla también para el paquete inicial, pero su ejecución no se declara realizada. Corte, volumen, exportación y reimportación se prueban desde fase 0.2. Instalación, desinstalación y actualización completas corresponden a fase 1.0.

## Condición de cierre de fase 0.1

Requisitos aplicables aprobados, decisiones técnicas documentadas, código compilado Release, pruebas automatizadas y visuales registradas, pipeline CI satisfactorio, prueba en dos equipos y paquete inicial verificado sin herramientas de desarrollo. Las comprobaciones externas pendientes deben declararse como tales y mantienen abierta la condición de cierre correspondiente.
