# Conectores entre regiones reconstruidas — 0.3.4

> Aceptación remota completada tras autorización de publicación: commit dbacc4a y [CI 8](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42751423136), **32+72 comprobaciones correctas**, build/paquete y artefacto satisfactorios. Los estados de bloqueo local siguientes se conservan como evidencia histórica y no se consideran pruebas aprobadas localmente. Sin cambios en seguridad ni revisión visual. Entorno/hashes en [CI.md](CI.md).

Fecha local: 2026-11-09. Continuación autorizada de fase 0.3. Sin dependencias nuevas, instalación global, IA, publicación ni revisión visual. Se conserva el contrato de reconstrucción de [PAINT_VOLUME.md](PAINT_VOLUME.md).

## Alcance

La opción existente de pin/cavidad se puede solicitar al construir ambos modos de volumen desde pintura. `core/cutting/RegionConnectors` recibe las regiones cerradas resultantes, conserva sus posiciones/materiales y devuelve geometrías más diagnósticos. No actúa sobre las fuentes ni publica cambios parciales. En caso de fallo global/cancelación se conserva la sesión; una pareja sin colocación segura se omite expresamente, manteniendo las demás conexiones que sí se validan.

La búsqueda es local y acotada. Hasta un pin por pareja candidata; puede generar varias uniones sobre distintas parejas. Las interfaces reconstruidas proceden de celdas y pueden tener escalones/socavados. No se asegura encontrar todas las posiciones posibles ni que cualquier pieza pueda montarse. El motor utiliza normales de facetas y no impone una orientación global del eje del conector. No se presenta como solución universal para superficies curvas ni como planificador del orden de ensamblaje.

## Colocación y guardas

1. Las cajas envolventes seleccionan parejas potenciales; no bastan para autorizar un conector. Se descartan solapes volumétricos iniciales.
2. Se prueban centroides de hasta 32 caras por posible donante, primero las 16 de mayor área y después 16 distribuidas en el resto de la lista ordenada. Si hay 32 o menos se usan todas. Se prueban ambos sentidos de donante/receptor, hasta 64 posiciones por pareja. Se puede omitir una superficie segura fuera de esta búsqueda.
3. La normal exterior del donante apunta al receptor. Se exige que un cilindro de anclaje con radio `radio_pin + pared` quepa en el donante y otro de guarda con radio `radio_cavidad + pared` y longitud `profundidad + holgura_axial + pared` quepa en el receptor. Radios de guarda circunscritos por `1/cos(pi/64)`, como en el motor de corte. La comprobación usa diferencia booleana, no solo puntos/cajas.
4. El pin de 64 segmentos se une al donante con anclaje de longitud pared. La cavidad ciega se resta del receptor. Holgura **diametral**; holgura axial igual a su mitad. La profundidad nominal se mide desde la superficie de contacto hacia el receptor. No se disminuyen dimensiones para forzar un ajuste.
5. Los volúmenes añadidos/retirados deben coincidir con los cilindros poligonales nominales, con tolerancia relativa 1e-7. Las guardas de conectores sucesivos sobre una pieza no pueden invadirse. La búsqueda usa las piezas ya modificadas para no perforar paredes debilitadas por conexiones previas.

Diámetro, profundidad, holgura y pared usan unidades del modelo; son mm en 3MF. Se mantienen valores iniciales 3, 4, 0.2 y 1, sin garantizar que quepan en un parche pequeño ni que un ajuste nominal se imprima con idéntica tolerancia física.

## Colisiones y recorrido de entrada

Para aceptar una colocación se comprueba conservadoramente el último tramo recto de inserción de longitud `profundidad + holgura_axial`, trasladando el donante en sentido contrario a su normal de pin. Se usa la envolvente convexa del cuerpo original y su copia trasladada; cada pin añadido tiene su propio barrido convexo, para no rellenar artificialmente todo el espacio entre pin y cara. Los barridos se intersectan contra las otras piezas con sus cavidades actuales. Se vuelven a comprobar los recorridos previamente aceptados antes de añadir otra conexión.

Esta comprobación puede rechazar cuerpos cóncavos aunque exista una trayectoria real libre. **No comprueba un recorrido desde una pieza completamente separada, giros, un orden global de montaje, esfuerzos, deformación ni capacidad de impresión.** Por ejemplo, un núcleo encerrado dentro de una carcasa puede no tener entrada recta: se informa sin forzar pines. No basta con que el pin quepa estáticamente en su cavidad.

Se comprueban intersecciones volumétricas finales entre todas las piezas y el delta global de volumen. Los resultados modificados deben seguir cerrados/orientados, con volumen positivo, y superar la validación STL float32 antes de entregarse al historial. La guarda permite fuera de volumen como máximo una fracción 1e-8 de su volumen. El umbral normalizado de colisión es `min(1e-12, radio_pin² * profundidad * 1e-8)`. No certifica ajuste físico ni ausencia universal de autointersecciones de entradas defectuosas.

## Interfaz y CLI

En la ventana, activar **Añadir pin y cavidad al cortar / separar**, configurar los cuatro parámetros y pulsar **Construir piezas desde pintura**. El resultado muestra cuántos conectores se generaron y qué parejas candidatas se omitieron y por qué. Una pareja candidata por caja puede no tener contacto real; no se anuncia como unión confirmada.

```json
{"commands":[{"op":"split-paint","mode":"partition","resolution":5,
"connector":{"diameter":1,"depth":1,"clearance":0.2,"wall":0.25}}]}
```

Para insertos, `mode: "inserts"` y `depth` de inserto obligatorio al mismo nivel que `resolution`; `connector.depth` es la profundidad independiente del pin. Sin objeto connector se conserva la ruta 0.3.3 sin conectores. Si se proporciona el objeto, sus cuatro números son obligatorios.

CLI añade `connector_reports`: una entrada por operación solicitada y aplicada, con `step` (comando contado desde 1) y `connections`. Cada pareja incluye IDs de piezas `first_piece`, `second_piece`, `connected` y `reason`; las aceptadas añaden `donor_piece`, `receiver_piece`, `position`, `normal`, `added_volume`, `removed_volume`. No se publica informe de una operación cuyo commit falle. Los IDs son los de la sesión, no índices de material. Los informes son resultados de operaciones, no una nueva parte persistida del historial.

Geometría/materiales y parámetros de conector participan en el historial existente; los controles de resolución/profundidad de inserto siguen sin recuperarse mediante undo/redo. Exportación continúa siendo STL sin color. Los conectores de corte y la ruta antigua de materiales cerrados mantienen su contrato anterior; este incremento conecta las regiones construidas desde pintura.

## Límites

Entre 2 y 32 regiones; hasta 100 000 triángulos de entrada acumulados y 64 parejas candidatas por cajas. Máximo 2 millones de triángulos resultantes acumulados. Se respetan los límites de reconstrucción/historial ya existentes. Son límites de datos/trabajo, no de tiempo/RAM total del proceso. El barrido convexo puede requerir memoria adicional.

Cancelación cooperativa durante indexación, posiciones, guardas, barridos, parejas y conversión/exportación. El contexto de Manifold recibe stop_token; operaciones inmediatas pueden atenderlo al regresar. No se termina un hilo de forma insegura. No se promete nueva validación visual de cancelación.

## Evidencia

Compilación Release completada; log `build/region-connectors-build.log`. Persiste C4244 de math.h externa de Manifold; sin dependencias nuevas ni advertencias nuevas del código propio observadas. No se crean fuentes/carpetas de pruebas ni se escriben modelos privados.

- Una compilación preliminar ejecutó ambas rutas: partición completa produjo un pin con volumen añadido 0.7841371226 y cavidad de volumen 1.2420732023; insertos produjo esa conexión y omitió tres parejas candidatas. Se observaron cierre/exportabilidad mediante las validaciones del motor. Son **dos casos preliminares**, no una batería de aceptación ni evidencia del último binario. Después se recompiló para mejorar los motivos de omisión, sin cambiar las reglas geométricas.
- La última compilación fue bloqueada antes de ejecutar la batería de **72 casos** (54 anteriores adaptados y 18 nuevos). Windows devolvió **WinError 4551**; eventos Code Integrity **3033/3077** indican incumplimiento de requisitos de firma/política. SHA-256 del exe: `34388ce93f606823f1ec2ba9f146bf380a9fc22c9d2c3aaa90aae7b47d647700`. Reportes `build/region-connectors-inline-results.json` y `build/region-connectors-code-integrity.json`. Se marcan **72 bloqueados antes de ejecución, cero aceptaciones y cero fallos funcionales observados**; estos ceros no acreditan funcionamiento. No se modificó seguridad ni se ejecutó una copia/variante para eludir el bloqueo.
- Paquete provisional generado sin ejecutar su exe: `build/packages/20261110T032939827Z/STLCutStudioAI-0.3.4-portable-win-x64-20261110T032939827Z.zip`. SHA-256 `ec24f6634b4c64ff365b440dbef326d12a534c8034d954b8bfb0f3ab5e581464`. **Solo comprobación estática:** CRC, inventario exacto de 75 archivos más manifiesto, tamaños/hashes, licencia miniz y exe idéntico al bloqueado. Reporte `build/region-connectors-package-static-verification.json`; log `build/region-connectors-package.log`. Los sellos del paquete están en UTC, con fecha local de trabajo 2026-11-09.
- Workflow preparado: 32 regresiones generales y 72 de pintura/volumen/conectores. Nuevas aceptaciones previstas: dimensiones nominales, anillos STL de pin/cavidad, exportaciones de ambos modos, recorrido bloqueado, paredes insuficientes sin retessellación, parejas desconectadas, reflexión, materiales de RGB igual, exterior oblicuo (octaedro), parámetros inválidos, undo/redo y commit fallido sin informe parcial. Las dos secciones Python del workflow se analizaron sintácticamente sin ejecutar aplicaciones; reporte `build/region-connectors-workflow-static.json`. Ahora aborta al primer bloqueo 4551 para evitar intentos repetidos.

**Pendiente de aceptación:** ejecutar esa batería y validar el paquete dinámicamente en un entorno autorizado, por ejemplo CI Windows cuando se autorice publicar estos cambios. Sin commit/push ni nueva CI en este incremento. Las 86 comprobaciones locales aprobadas de 0.3.3 no se atribuyen a 0.3.4. Sin revisión visual, validación nueva de cancelación ni modelos privados nuevos. También faltan casos de aceptación con varias conexiones generadas en una misma red, contactos curvos, tamaños muy dispares y planes de montaje completos; no se declara cerrada fase 0.3.
