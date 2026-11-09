# Materiales y separación de cuerpos 3MF — 0.3.1

Fecha local: 2026-11-08. Incremento autorizado de fase 0.3. Conserva y separa colores/materiales estándar de regiones que ya forman sólidos. Es una parte del objetivo aprobado: la pintura de fabricante y los dos modos de reconstrucción volumétrica siguen pendientes.

## Funciones implementadas

El importador lib3mf 2.5.0 conserva propiedades de objeto/triángulo de BaseMaterial y ColorGroup, color RGBA, nombre de material, objetos e instancias con sus transformaciones. La herencia de propiedades y la reflexión se resuelven antes de presentar la malla; una reflexión intercambia también las etiquetas de los vértices correspondientes. Los identificadores publicados son los identificadores internos del SDK en la sesión, no necesariamente los índices XML originales.

Mesh contiene paleta de materiales, TriangleStyle por triángulo/instancia y SourceInstance con objeto, nombre y rango de triángulos. El historial contabiliza esos vectores y nombres. Los colores/materiales se comparten con las mallas inmutables; los resultados uniformes de cortes y conectores heredan su material y, cuando hay una sola instancia de origen, su identidad. Un corte geométrico sobre varios colores no proyecta etiquetas a las caras nuevas: avisa de su pérdida y marca la apariencia como incompleta. Para conservar etiquetas, separar primero regiones uniformes.

La preparación del visor usa RGB por vértice, incluido el color de las piezas separadas; conserva RGBA en datos, pero el renderizado sigue siendo opaco y no implementa texturas. No se realizó revisión visual por petición del usuario; compilación y pruebas CLI no certifican el resultado de GPU/shaders.

## Uso en la interfaz

1. Abrir un 3MF con propiedades estándar. Las estadísticas indican cuántas propiedades se conservaron y si hay atributos no interpretados.
2. Seleccionar una pieza activa y pulsar **Separar cuerpos por material / color**.
3. La pieza se sustituye por sus regiones conectadas, con material y procedencia; se conservan las piezas hermanas. Deshacer/Rehacer recuperan toda la operación como un paso.
4. Opcionalmente activar **Añadir pin y cavidad al cortar / separar** y configurar parámetros antes de separar. Las condiciones de contacto admitidas se detallan abajo.
5. Seleccionar y exportar cada pieza. STL no guarda colores/materiales: se conserva su asociación en la sesión y en el nombre de pieza. Aún no hay exportación 3MF de resultados ni guardado de proyecto.

El trabajador prepara todas las geometrías/vistas; un historial y caché candidatos se publican solo cuando caben en el presupuesto. Rechazo, cancelación o falta de memoria conserva las piezas y Rehacer. La selección/parámetros explícitamente cambiados antes de ejecutar conservan la semántica de los comandos anteriores; no crean pasos independientes. La operación puede retirar pasos antiguos por los límites generales del historial; el contador de pasos disponibles se actualiza.

## Separación exacta y límites

- Agrupa por instancia, identidad de material y conectividad a través de aristas. No fusiona materiales diferentes por tener el mismo RGB ni regiones desconectadas del mismo color. No une automáticamente instancias.
- Cada resultado debe ser una malla cerrada/orientada, sin degeneraciones a tolerancia extensión × 1e-9 y de volumen positivo. Comprueba la suma de volúmenes con tolerancia `max(abs(total) × 1e-7, extensión³ × 1e-12)`. La separación conserva triángulos/coordenadas; no es reparación ni unión booleana de cuerpos inicialmente solapados.
- Si las etiquetas corresponden solo a parches abiertos, faltan propiedades o hay colores diferentes por vértice, rechaza la separación directa. Separar triángulos pintados no demuestra que sean piezas sólidas.
- Hasta **500 000 triángulos para esta operación**, **256 regiones** y **4096 propiedades estándar usadas**. Importación/corte mantienen sus límites de 2M triángulos; el límite menor de separación acota mapas temporales de conectividad y no es una cuota de RAM del proceso. Historial: 1 GiB contabilizado, 32 operaciones y 256 piezas activas.
- Nombres de material/objeto limitados a 4096 bytes. JSON expone hasta 64 materiales e instancias, nombres de hasta 256 caracteres, conteos completos y appearance_preview_truncated cuando los datos de vista previa pueden estar recortados. El modelo conserva la paleta/instancias completas dentro de los límites anteriores.
- El SDK puede advertir atributos de fabricante desconocidos. Se marca unsupported_appearance y se bloquea la separación exacta conservadoramente: esa bandera no identifica exclusivamente pintura, y puede incluir otros atributos desconocidos. Materiales compuestos, texturas y propiedades no implementadas también la activan.

## Conectores automáticos de este incremento

Con connector habilitado admite exactamente **dos regiones** cuyo contacto plano esté alineado con X, Y o Z. Deduce el plano a partir de sus cajas, retira las superficies comunes y exige que la envolvente restante cierre correctamente antes de cortar y añadir el conector con el motor 0.3.0. Esto comprueba geometría real antes de publicar, además de la coincidencia de cajas. Si hay contacto parcial incompatible, huecos, paredes insuficientes o el contacto no es uno de los admitidos, rechaza la operación completa.

La primera región recibe el pin y la segunda la cavidad. Se conservan sus materiales/nombres de origen y se valida el delta nominal de volumen, cierre y posterior exportación float32. Dos regiones con separación espacial no necesitan un conector entre sí: se separan sin pin/cavidad y se avisa. Más de dos regiones pueden separarse sin conectores, pero su generación automática para redes de múltiples contactos aún no está implementada. No se promete un plan de ensamblaje general para superficies curvas o enclavamientos.

## Comandos de producción

```json
{"commands":[
 {"op":"split-materials","connector":{"diameter":3,"depth":4,"clearance":0.2,"wall":1}},
 {"op":"undo"},{"op":"redo"},{"op":"export","piece":2}
]}
```

Se ejecuta con `--edit-stdin` y el modelo en stdin. Omitir connector para separar sin uniones. Ambos CLI de inspección informan materiales, instancias, conteos, estilos y apariencia no interpretada. Las exportaciones continúan usando validatedStl; no se añade una salida inválida al JSON.

## Compilación incremental corregida

Las primeras comprobaciones descubrieron un cierre 0xC0000409 en rutas STL después de ampliar Mesh. La inspección de Ninja mostró cero dependencias para StlReader.cpp.obj: CMake había decodificado mal el prefijo localizado de /showIncludes, dejando objetos con la ABI antigua. No era un error de los archivos STL.

CMake normaliza el prefijo español observado o admite el inglés y rechaza prefijos no conocidos. El preset solicita VSLANG=1033; el equipo local solo dispone del paquete español y usa la normalización. build-windows.ps1 migra cachés existentes una vez con --fresh/--clean-first, restaura la variable de entorno del proceso y exige que Ninja registre Mesh.h antes de marcar la migración. No cambia idiomas instalados ni variables persistentes. Se reconstruyeron StlReader/StlWriter y todos los consumidores afectados; un cambio posterior de Mesh volvió a reconstruirlos por dependencia. El build final incremental indicó no work to do. No se garantiza reconocimiento de todos los idiomas de MSVC.

## Validación local final

- Release x64 0.3.1, Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4; sin dependencias nuevas. Logs color-build.log/color-package.log bajo build/.
- **20/20** casos de color: BaseMaterial/ColorGroup, herencia, reflexión, identidad RGB igual, cuerpos desconectados, parámetros/procedencia, undo/redo, ramas/IDs, exportación/reimportación, presupuestos, rechazo de pintura/gradientes/parches abiertos, conectores planares automáticos, ausencia de contacto, pared insuficiente y límites de nombres/vista previa. Reporte build/color-inline-results.json.
- **63/63** regresiones geométricas/importación, **22/22** de historial y **16/16** de conectores en los reportes color-geometry-regression.json, color-history-regression.json y color-connector-regression.json. El caso trasladado mantiene el contrato de rechazo float32 introducido en 0.2.3.
- **28/28** comprobaciones del workflow ejecutadas localmente sobre el paquete extraído con espacio/ñ y PATH mínimo: tres nuevas de separación/color/historial/exportación, conectores automáticos planos y rechazo de pintura no interpretada. ZIP provisional STLCutStudioAI-0.3.1-portable-win-x64-20261109T032016383Z.zip, SHA-256 `88ee0b4c08c45632ae1fc905daf9b72b6cb20616c4093fe440fb4ecea1a1588f`; inventario de 74 archivos más manifiesto. Reporte build/color-package-verification.json.
- Seis modelos privados previos reimportados con conteos/dimensiones intactos y SHA-256 sin cambios. No tienen materiales estándar interpretables para esta operación; se rechazan explícitamente sus intentos de separación, manteniendo una pieza y cero pasos. No acreditan separación de pintura Bambu/Orca. Reporte privado build/color-real-validation.json, sin nombres ni modelos añadidos a Git.
- Sin revisión visual ni ejecución remota nueva; cambios locales sin commit/push. Última CI remota publicada: 0.2.2. Sin fuentes/carpetas de pruebas propias. La entrega final .exe permanece en fase 1.0.

## Siguiente construcción pendiente, ya autorizada

Decodificar la pintura de fabricante preservando subdivisiones, construir los dos modos elegidos —insertos de profundidad configurable y partición completa del volumen—, y conectar redes de regiones con contactos generales. Esta versión no presenta controles inoperantes de esos modos ni los declara implementados. Tampoco implementa reparación universal ni exportación 3MF.

## Incremento posterior 0.3.2

Se construyó la lectura de códigos de pintura y su presentación superficial, todavía sin reconstrucción volumétrica. Hay prueba preliminar sobre un modelo privado, pero la ejecución de la compilación final quedó bloqueada por Windows. Esta aceptación 0.3.1 no se transfiere al nuevo binario; véase [PAINT_IMPORT.md](PAINT_IMPORT.md).
