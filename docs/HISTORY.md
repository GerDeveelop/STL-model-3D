# Cortes encadenados y Deshacer/Rehacer — 0.2.2

Fecha: 2026-11-08. Incremento aprobado por el usuario. Añade edición durante la sesión al motor Manifold existente; no añade dependencias, guardado de proyectos ni reparación de mallas.

## Uso

1. Abrir STL/3MF. El modelo importado es la primera pieza activa.
2. Seleccionar una pieza en **Mostrar / exportar**, ajustar el plano y pulsar **Cortar pieza seleccionada**. La pieza se sustituye por sus lados positivo y negativo; las demás piezas se conservan.
3. Repetir sobre cualquiera de las piezas activas. Los identificadores y nombres indican cada resultado; las estadísticas muestran de qué pieza procede.
4. Usar **Editar > Deshacer corte** (`Ctrl+Z`) y **Rehacer corte** (`Ctrl+Y`), o los botones de la barra. Recuperan geometría, piezas activas, selección, parámetros y visibilidad del plano. La cámara vuelve a encuadrar la pieza; su posición no forma parte del historial.
5. Exportar la pieza seleccionada mediante el mismo guardado STL verificado. El original continúa visible/exportable como **Original (referencia)** cuando ya no es una pieza activa; allí Cortar está deshabilitado. Para volver a cortarlo como estado de trabajo, deshacer hasta el estado inicial si sigue disponible.

Solo los cortes aplicados correctamente crean pasos. Seleccionar piezas, editar/mostrar el plano y exportar no crean acciones de deshacer. Esos parámetros se conservan en el estado actual hasta la siguiente operación. Un corte nuevo después de deshacer elimina la rama de Rehacer; los identificadores de esa rama no se reutilizan.

El historial vive en memoria y se pierde al cerrar o tras abrir correctamente otro modelo. Si la nueva importación falla o se cancela, se conserva toda la sesión anterior. No se implementó un formato de proyecto ni una confirmación de guardado de sesión.

## Arquitectura y transacciones

- `core/projects/EditHistory.h/.cpp`: lista de estados, selección/plano, procedencia de piezas, límites y operaciones undo/redo; independiente de Qt. Las mallas se comparten mediante `shared_ptr<const Mesh>` y cada estado mantiene referencias a piezas inmutables.
- `apps/desktop/main.cpp`: adapta la lista a la interfaz y mantiene una caché de ViewMesh por identificador, compartida por todos los estados. Retira vistas de piezas que ya no están referenciadas por el historial ni por el original.
- El trabajador recibe una referencia inmutable a la pieza seleccionada, calcula ambos lados y sus vistas. Solo al terminar correctamente se construye una copia candidata del historial/caché, se comprueban límites y se publica el cambio. Un fallo/cancelación no borra piezas, pasos de deshacer ni la rama de rehacer.
- Mientras hay importación, corte o exportación, los controles de edición y Deshacer/Rehacer están deshabilitados. La exportación conserva su referencia de pieza hasta terminar. Se mantiene el guardado atómico y la protección del archivo fuente.

## Presupuesto de sesión

- Máximo **32 cortes deshacibles**, **256 piezas activas** y **1 GiB contabilizado** para mallas, vistas de CPU y metadatos de los estados/piezas. La interfaz muestra uso aproximado en MiB y acciones disponibles.
- Cuenta el original siempre, así como piezas alcanzables desde estados anteriores, actual y Rehacer. Las mismas mallas/piezas compartidas se cuentan una sola vez; las vistas de CPU se cargan al presupuesto por pieza. Se contabilizan capacidades de los vectores de geometría/vértices y cadenas relevantes.
- Al aplicar un corte se descarta primero la rama de Rehacer sustituida. Después, si hace falta, se retiran los estados más antiguos de Deshacer hasta respetar cantidad y presupuesto. La interfaz indica cuántos pasos antiguos se retiraron; se mantiene el original de referencia. Puede haber menos de 32 pasos disponibles por memoria.
- Si el estado nuevo más el original no cabe aun retirando esos pasos, se rechaza el corte de forma transaccional y se conserva **todo** el historial anterior. El límite de piezas también rechaza el corte sin modificarlo.
- El presupuesto no es una cuota rígida de RAM del proceso: mapas, contenedores/controles, asignaciones temporales del trabajador, estructuras internas de Manifold y memoria GPU pueden aumentar el uso. Los límites del motor (2M triángulos/vértices, presupuesto de buffers de adaptación y exigencia de malla válida) siguen vigentes. Tampoco se promete cancelación inmediata dentro de una operación interna del motor; véase MANIFOLD_CUT.md.

## Edición por comandos

`--edit-stdin '<objeto JSON>'` recibe el modelo STL/3MF por stdin y devuelve JSON con estados, piezas activas, selección, plano, acciones disponibles, bytes retenidos y exportaciones solicitadas. No abre ventanas ni guarda archivos. Comparte EditHistory, el importador, el cortador y el serializador con la interfaz.

```json
{"commands":[
  {"op":"cut","piece":1,"normal":[0,0,1],"offset":5},
  {"op":"cut","piece":2,"normal":[1,0,0],"offset":5,"visible":false},
  {"op":"undo"},
  {"op":"redo"},
  {"op":"export","piece":4}
]}
```

El ejemplo presupone un cubo de lado 10. Cada corte crea dos IDs consecutivos y selecciona el lado positivo. Operaciones: `cut`, `undo`, `redo`, `select`, `plane`, `status`, `export`; `piece` selecciona antes de la operación cuando se proporciona. `plane` usa los mismos campos que cut, sin ejecutar geometría. `export` devuelve `stl_base64`, sin escribir al disco. Exit 0 para éxito, 1 para error; en error se devuelve también el estado que se conservó, si se llegó a cargar un modelo.

Máximo 256 comandos y 256 MiB acumulados de bytes STL exportados por llamada (el JSON base64 ocupa más). La línea de comandos de Windows puede imponer un límite menor al texto de la petición. Opcionalmente `limits` acepta `bytes`, `steps`, `pieces` para reducir los máximos anteriores; el CLI no prepara vistas y por ello su presupuesto no incluye vértices de renderizado. No se aumenta ningún máximo desde JSON. No se aceptan scripts ni se ejecuta código indicado por esos datos.

## Validación del incremento

- Build Release x64 satisfactorio; mismo Qt 6.8.3, lib3mf 2.5.0 y Manifold 3.5.4, sin dependencias nuevas. Logs `build/history-build.log` y `build/history-package.log`.
- **22/22 casos inline de historial** satisfactorios: tres cortes conservando hermanas y volumen total; procedencia/IDs; undo/redo restaurando selección/plano/visibilidad; rama nueva sin reutilizar IDs; rechazo de IDs obsoletos; exportación/reimportación de todas las piezas y original; fallos transaccionales de geometría/presupuesto/piezas; liberación de pasos antiguos; selecciones sin copias ni pasos nuevos; 35 cortes para comprobar el máximo real de 32 pasos y 256 intentos para comprobar 256 piezas. Límites de prueba reducidos mediante la API de producción, sin fuentes ni archivos de pruebas propios. Reporte: `build/history-inline-results.json`.
- Regresión geométrica/importación **63/63** correcta con el ejecutable nuevo, en `build/history-geometry-regression.json`.
- Paquete provisional `STLCutStudioAI-0.2.2-portable-win-x64-20261108T182622100Z.zip`, SHA-256 `581a48d6845d1452707e09a5e2a9134105bee967e490eb43c0b6eb063eeb15fb`. Inventario de 74 archivos más manifiesto. **21/21 comprobaciones** del workflow ejecutadas localmente sobre la copia extraída a ruta con espacio y ñ, PATH limitado a Windows/System32: quince anteriores más seis de historial. Reporte: `build/history-package-verification.json`. Posteriormente se publicó 34b0cb7 y la CI remota 0.2.2 terminó correctamente; véase CI.md.
- Interfaz visible: cubo 3MF de 10 mm, primer corte 1000 → 500+500, segundo sobre la parte positiva 500 → 250+250; tres piezas activas. Ctrl+Z/Ctrl+Y recuperaron las piezas y selección anteriores, normal X, posición 5 y plano oculto. Corte alternativo sobre Y tras deshacer generó IDs 6/7, conservó la pieza 3 y eliminó Rehacer. Original intacto visible como referencia con Cortar deshabilitado. Evidencia `build/history-gui-chain.txt`, `history-gui-undo.txt`, `history-gui-redo.txt`, `history-gui-branch.txt`, `history-gui-original-reference.txt`.

Cancelación visible comprobada con el paquete 0.2.2 y una malla de 1 929 612 triángulos generada en memoria: completar un corte, deshacer y cancelar otro mediante Ctrl+. conservó el original (#1), Deshacer 0 y Rehacer 1. Ctrl+Y recuperó las piezas #2/#3, selección #2 y Deshacer 1/Rehacer 0, sin recalcular el corte. La barra de estado mostró correctamente dos piezas. Evidencia: `build/history-gui-cancel-redo.txt` y `build/history-gui-redo-after-cancel.txt`. Cierre normal con Alt+F4 y ventana ausente en el inventario posterior. El presupuesto mostrado fue 265.00 MiB contabilizados; no es una medición RSS ni certifica latencia máxima. No se repitió guardado nativo asistido; sí la serialización/reimportación de piezas por CLI. Código publicado y CI satisfactoria según CI.md; no se genera instalador final en este incremento.

## Validación posterior con modelos reales

Cinco secuencias reales de cortes y undo/redo correctas. La rama export del CLI solo serializa y puede devolver éxito para una salida float32 con degeneraciones, aunque la importación y el volumen parezcan correctos: dos casos encontrados en quince piezas. No equivale al guardado validado de la interfaz. Hallazgo y ajuste pendiente en [REAL_MODEL_VALIDATION.md](REAL_MODEL_VALIDATION.md).

Desde 0.2.3 la rama export del CLI usa la validación compartida y rechaza esas conversiones, conservando historial y Rehacer; véase [EXPORT_VALIDATION.md](EXPORT_VALIDATION.md). El hallazgo anterior describe 0.2.2.

## Separación de regiones 0.3.1

La separación por material reemplaza una pieza por varios sólidos en un solo paso. commitSplit valida IDs/límites y construye una línea temporal candidata; error/cancelación conserva geometría y Rehacer. El presupuesto incluye materiales, estilos e instancias/nombres. El máximo de 32 pasos aplica a cortes y separaciones. Los resultados conservan color y procedencia; detalles en COLOR_IMPORT.md.

## Metadatos de pintura 0.3.2 en validación

Presupuestados PaintFacet y jerarquías/partes de origen junto a la malla compartida. El original con pintura queda como referencia al hacer un corte geométrico; no se reproyecta pintura a las caras nuevas. La validación final de esta ampliación está bloqueada, según PAINT_IMPORT.md.
