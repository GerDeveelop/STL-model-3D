# Lectura de pintura 3MF — 0.3.2

> Actualización 2026-11-09: la lectura descrita aquí se conserva. El incremento local 0.3.3 añade los dos modos de reconstrucción por celdas; véase [PAINT_VOLUME.md](PAINT_VOLUME.md). Las menciones de reconstrucción pendiente y bloqueo local en este documento describen el estado histórico de 0.3.2. El CLI 0.3.3 se ejecutó para sus 54 comprobaciones locales; no se modificó la política de Windows ni se certifica que el bloqueo histórico se haya resuelto globalmente.

Fecha local: 2026-11-08. Alcance ya autorizado: ambos tipos de 3MF y ambos modos futuros de reconstrucción (insertos y partición volumétrica). El usuario autorizó además miniz 3.1.0 para leer el ZIP original en memoria. Esta versión añade lectura de pintura y compiló Release. La validación dinámica final local quedó bloqueada por Control de aplicaciones, pero después se publicó con autorización y la CI Windows 7 completó correctamente 32 casos generales y los 36 de pintura. Véase CI.md. No se declara terminada la fase ni implementados los dos modos volumétricos.

## Qué se construyó

- miniz 3.1.0 estática, solo lectura ZIP/inflate/CRC, sin stdio, compresión ni escritura de ZIP. Sin extracción de modelos al disco ni instalación global. Fuente/tag/commit y licencia MIT en THIRD_PARTY_NOTICES.md; bootstrap fijado por SHA-256.
- `core/io/ZipParts` obtiene partes .model y configuraciones seleccionadas, comprueba tamaños, nombres, duplicados, CRC y cancelación. `core/io/PaintCodec` interpreta códigos paint_color/mmu_segmentation como árboles de subdivisión sobre coordenadas baricéntricas. Es implementación propia del formato, sin copiar/enlazar código de los laminadores.
- `apps/desktop/ModelImport` adapta XML y JSON con Qt Core ya existente, fuera del núcleo geométrico. Asocia cada instancia con su parte/modelo original, triángulo y jerarquía; lee filament_colour y asignaciones extruder de objeto/pieza. Si falta paleta o la herencia es ambigua, conserva la región sin color resuelto y lo informa. No supone un filamento por defecto sin datos.
- Mesh conserva PaintFacet por región, referencia al triángulo fuente, material/filamento y contadores. La geometría original no se subdivide ni se modifica. El historial contabiliza vectores y jerarquías nuevos. Un corte geométrico no reproyecta pintura a las caras nuevas; lo advierte y el original pintado permanece en el historial.
- La preparación CPU del visor representa las subdivisiones con RGB por región, y gris para regiones sin color resuelto. Se corrigen coordenadas baricéntricas y orientación al reflejar una instancia. No se realizó revisión visual por petición del usuario; el resultado final GPU/shaders no se certifica.

La separación directa de cuerpos sigue sin aceptar pintura superficial como si fueran sólidos. Leer el color no define todavía el interior de cada pieza. **Los modos de insertos y partición completa del volumen, así como los conectores sobre esas regiones reconstruidas, siguen pendientes de implementación.**

## Diagnósticos de producción añadidos

- `--decode-paint <hex>` o `--decode-paint-stdin`: código hexadecimal sin espacios, resultado JSON con cantidad de hojas, primeras 64 regiones baricéntricas y fracciones de área por filamento. Stdin evita el límite menor de línea de comandos de Windows.
- `--inspect-stdin` y `--inspect ruta`: usan el adaptador completo y añaden painted_triangles, paint_facets y unresolved_paint_facets. painted_triangles cuenta caras con atributo de pintura, incluso si el código vacío hereda el color.
- `--inspect-view-stdin`: prepara únicamente datos de presentación CPU, sin ventana/OpenGL, y añade cantidad/bytes de vértices y hasta 64 grupos RGB con conteos/cajas normalizadas. No equivale a revisión visual.

El lector básico `core/io/MeshReader` conserva su contrato independiente de Qt; la adaptación completa se ofrece desde ModelImport en la aplicación/CLI. Las reimportaciones STL de validación no necesitan la capa de pintura.

## Límites y casos no resueltos

Se conservan límites de archivo/ZIP de 256 MiB, 4096 entradas y stored/deflate sin cifrado. Partes seleccionadas se leen en memoria, no en rutas del equipo. Nombres ZIP hasta 1024 bytes, sin rutas ambiguas ni duplicadas; configuraciones seleccionadas hasta 4 MiB cada una. XML sin DTD/entidades, profundidad 128 y hasta 64 atributos por elemento en la adaptación Qt. Códigos de pintura hasta 262 144 caracteres por cara y 64 MiB acumulados por modelo/partes originales; profundidad del árbol 20, 65 536 hojas por cara y filamentos hasta 4096. La presentación expandida total no puede superar 2M caras. Son límites de trabajo/datos, no una cuota de RAM del proceso.

El SDK geométrico sigue leyendo el 3MF antes de la adaptación de pintura. En comprobaciones preliminares aceptó una cara con código de 21 845 caracteres (16 384 hojas) y rechazó otra con 87 381 caracteres antes de llegar al decodificador, con un diagnóstico genérico de sección build. No se promete que el lector 3MF alcance el máximo aislado del codec. La prueba de 65 536 hojas se preparó por stdin directo, evitando ese límite previo, pero no se ejecutó en la compilación final por el bloqueo descrito abajo.

La asociación de color de fabricante implementada usa filament_colour de Metadata/project_settings.config y extruder de Metadata/model_settings.config, con herencia por jerarquía. No se declara compatibilidad universal con todos los dialectos de Prusa/Bambu/Orca, texturas, gradientes o mezcla de filamentos. Dos asignaciones diferentes sobre la misma jerarquía se conservan sin resolver en lugar de elegir una a ciegas. Las propiedades estándar y las paletas de filamento siguen distinguidas; estas últimas usan resource 0 y property como número de filamento en los resúmenes.

## Dependencia aprobada

[Fuente miniz 3.1.0](https://github.com/richgel999/miniz/tree/3.1.0), commit `174573d60290f447c13a2b1b3405de2b96e27d6c`. Archivo oficial de 306 766 bytes, SHA-256 calculado `89ce8fc6012b1f48407bb1422dd75701bb639e00fc235090cd8b47283fe6197b`. Bootstrap verifica el ZIP y nueve archivos de producción; no extrae pruebas ni ejemplos upstream. LICENSE íntegra incluida en el paquete; no hay DLL miniz adicional ni biblioteca XML nueva. No se ejecuta código contenido en un 3MF.

Referencias de formato consultadas: [TriangleSelector de Bambu Studio](https://raw.githubusercontent.com/bambulab/BambuStudio/master/src/libslic3r/TriangleSelector.cpp) y [constantes de filamento](https://raw.githubusercontent.com/bambulab/BambuStudio/master/src/libslic3r/Model.cpp). Son referencias de diseño; no dependencias incorporadas ni una selección de licencia AGPL para el producto.

## Evidencia y bloqueo

- Compilación Release x64 y empaquetado completados. Logs build/paint-build.log y build/paint-package.log. Qt 6.8.3, lib3mf 2.5.0 y Manifold 3.5.4 sin cambios; miniz 3.1.0 añadida. Se mantiene la advertencia C4244 externa ya documentada de Manifold; no se afirma build sin advertencias.
- En una compilación preliminar se comprobaron colores completos, siete patrones de subdivisión, conservación de área sobre 80 árboles generados, reflexión de pintura y otros casos. Una primera batería obtuvo 32/36 aceptaciones; cuatro casos necesitaron correcciones de sus relaciones/UUID OPC o separar el límite del SDK del límite del codec. Esto **no acredita la batería final**; además se reforzó que los rechazos de CRC/DTD/tamaño ocurran por la causa esperada, no por entradas duplicadas del generador.
- Modelo privado R5 en comprobación preliminar: 61 396 triángulos geométricos intactos, 20 116 caras con pintura/20 116 regiones, cero sin color resuelto y 184 188 vértices de presentación CPU. Tres grupos RGB usados, coherentes con su paleta. 0.4562 s y pico RSS observado 34.91 MiB, una sola medición incluyendo lectura/decodificación/preparación; no es benchmark ni certificación del binario final. SHA-256 de la fuente intacto. Reporte privado build/paint-real-initial.json.
- Al intentar ejecutar la última compilación, Windows devolvió WinError 4551. Eventos Code Integrity 3033 y 3077 indican requisitos de nivel de firma o incumplimiento de política. Hash del ejecutable: `d364d6ad8bc76eb8a3ca5ce66c59cc3f6a512823aebdd22b95835930caeb942b`. La batería final de 36 casos quedó **bloqueada antes de ejecutarse**, no son 36 fallos funcionales ni 36 pruebas aprobadas. Registros en build/paint-code-integrity.json y paint-inline-results.json. No se cambió la protección ni se intentó ejecutar una copia/variante para eludirla.
- ZIP provisional `STLCutStudioAI-0.3.2-portable-win-x64-20261109T040214286Z.zip`, SHA-256 `6eeed888d4ed36dbeb3bdd512ecfb06909969edae80c75571461b8952f0e203f`. **Solo validación estática**: CRC del ZIP, inventario exacto de 75 archivos más manifiesto, hashes y licencia miniz original comprobados sin extraer/ejecutar su exe. El hash de ese exe coincide con el bloqueado. Reporte build/paint-package-static-verification.json.
- Workflow preparado con bootstrap miniz y cuatro casos nuevos (32 en total), sin ejecución local final ni remota de 0.3.2. Las regresiones de 0.3.1 no se atribuyen al binario nuevo. Última CI remota publicada: 0.2.2. Cambios locales sin commit/push, sin fuentes/carpetas de pruebas propias y sin modelos privados publicados.

## Estado de la validación local y paso completado en CI

Completar la batería en un entorno autorizado para ejecutar la aplicación, por ejemplo CI Windows tras autorización de publicación de los cambios. No se propone desactivar Control de aplicaciones ni modificar la política del equipo. La validación de lectura pendiente quedó completada remotamente según la sección siguiente; la restricción local y la ausencia de revisión visual se mantienen.

## Validación remota completada tras publicación

Código d2a219a publicado en main por autorización del usuario. Ejecución 7 satisfactoria: build/paquete y **32/32** casos generales más **36/36** de pintura. Incluye los casos OPC externos con UUID, herencia anidada/ambigua, límite aislado de 65 536 hojas por stdin y límite global de presentación de 2M, con las causas de rechazo comprobadas. Se mantienen como evidencia histórica las pruebas preliminares y el bloqueo local descritos arriba. Enlaces, entorno y hashes remotos en [CI.md](CI.md). El siguiente desarrollo de insertos/partición ya puede apoyarse en esta aceptación remota, sin afirmar ejecución local ni revisión visual.
