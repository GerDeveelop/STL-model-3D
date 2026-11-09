# Fase 0.3 — Conectores y separación por color 3MF

## Autorización y estado

El usuario aprobó continuar la construcción con pines/cavidades configurables e integración con Deshacer/Rehacer y añadió separar automáticamente un 3MF por colores con sus conectores. Solicitó omitir revisión visual. Primer incremento local 0.3.0 compilado: conectores opcionales al cortar. La separación por color y sus dos modos volumétricos siguen en diseño; no se declaran implementados. Uso y evidencia del primer incremento se detallan al final de este documento.

## Objetivo de uso

Abrir 3MF → identificar materiales/colores y regiones → obtener piezas sólidas por región → localizar uniones entre piezas → proponer/generar pines y cavidades compatibles → exportar cada pieza con su identificación de color. Las operaciones deben poder deshacerse/rehacerse sin modificar la fuente.

No confundir un color con una sola pieza: varias regiones desconectadas del mismo color pueden requerir piezas independientes. No agrupar automáticamente materiales distintos solo porque tienen el mismo RGB. STL sigue sin guardar color: la asociación pieza/material deberá mantenerse en el modelo de sesión y en los nombres/metadatos de salida.

## Dos clases de entrada

1. **Volúmenes existentes por material/color.** Conservar objetos, instancias, transformaciones y propiedades 3MF antes de aplanar las mallas. Validar cada sólido y su relación con los vecinos. Cuerpos que no se tocan no requieren conectores entre sí.
2. **Pintura sobre una superficie.** Decodificar propiedades por triángulo y, cuando exista, la subdivisión de pintura. Una etiqueta superficial no define por sí sola la frontera dentro del sólido. Hace falta acordar una política de construcción volumétrica (por ejemplo, inserto con profundidad configurable o partición del volumen) y validar espesores, colisiones y cierre. No inventar esa política silenciosamente ni presentar agrupaciones de triángulos abiertos como piezas imprimibles.

Decisión del usuario: admitir ambos tipos de 3MF y ofrecer ambos modos para pintura superficial: insertos con profundidad configurable y partición de todo el volumen según los colores exteriores. La profundidad debe poder configurarse; no se impone una profundidad universal. El algoritmo de partición debe declarar resolución/errores y comprobar cierre/solapes. Estas rutas aún no están implementadas. Texturas, gradientes y mezcla de materiales necesitan un contrato específico antes de prometer su separación automática.

## Inspección técnica realizada

- MeshReader actualmente reúne las instancias en Mesh y avisa que no conserva colores/materiales. Mesh/Triangle no tienen etiquetas de color ni de objeto; Piece guarda solo nombre/procedencia y malla. Debe preservarse esa información antes del agrupamiento geométrico.
- El SDK lib3mf 2.5.0 instalado ofrece propiedades de objeto/triángulo y grupos de color/material. Usar esos datos no requiere añadir otro SDK.
- Lectura local de tres archivos privados: uno contiene atributos paint_color en 20 116 triángulos repartidos entre seis mallas; los otros dos inspeccionados no tienen colorgroup/basematerials ni paint_color. Eso no demuestra ausencia de información de filamento en sus metadatos del laminador. No se publican nombres ni contenidos de las fuentes.
- Referencia primaria consultada: [importador 3MF de Bambu Studio](https://raw.githubusercontent.com/bambulab/BambuStudio/master/src/libslic3r/Format/bbs_3mf.cpp) usa paint_color como atributo de segmentación; [TriangleSelector](https://raw.githubusercontent.com/bambulab/BambuStudio/master/src/libslic3r/TriangleSelector.cpp) interpreta un flujo de pintura. Consulta de diseño; no se copia código ni se incorpora una dependencia del laminador. Cualquier adaptación posterior requiere revisión de versión/licencia y límites.

## Secuencia de implementación

1. Conectores sobre una pareja de piezas originadas por corte: diámetro, longitud/profundidad y holgura explícitos en mm. Definir si la holgura es radial o diametral; evitar valores ambiguos.
2. Ampliar el historial para reemplazar varias piezas en una sola transacción; fallos/cancelación conservan piezas, metadatos y Rehacer. Los colores/procedencia también forman parte del estado. Mantener el presupuesto de sesión incluyendo los datos nuevos.
3. Importación con etiquetas de objeto/material y diagnóstico de la representación de color, conservando compatibilidad con STL/3MF sin color.
4. Separación de regiones sólidas y contactos; asignar conectores solo donde encajen y sean ensamblables. Las superficies pintadas dependen de la política volumétrica acordada.
5. Integración con interfaz y CLI; exportación validada compartida, automatización inline y documentación. Sin revisión visual por petición del usuario; registrar expresamente ese límite.

## Condiciones geométricas para conectores

- Pin unido a su pieza y cavidad contenida en la otra, con holgura y profundidad verificadas. No basta colocar un cilindro por caja envolvente o centroide.
- Respetar grosor mínimo alrededor de la cavidad, distancia a bordes y separación entre conectores. Comprobar colisiones y que el pin no atraviese el exterior de la pieza receptora.
- Mantener coordenadas de ensamblaje. En regiones curvas o pequeñas puede no existir un conector seguro: informar esa unión sin alterar las demás ni reducir parámetros silenciosamente.
- No prometer ensamblabilidad universal ni encajes físicos garantizados para todas las impresoras/materiales; las tolerancias nominales se configuran y verifican geométricamente.

## Aceptación prevista

Datos propios generados en memoria, sin carpetas/archivos de pruebas: dos sólidos bicolor en contacto; colores iguales en componentes desconectados; propiedades heredadas/transformaciones/reflexión; pintura parcial con subdivisión cuando se habilite; metadatos ausentes o inválidos con error explícito; paredes demasiado delgadas; conectores fuera del contacto; holgura y dimensiones; volumen/cierre/orientación; exportación/reimportación float32; undo/redo y fallos transaccionales; límites/cancelación. Los modelos privados solo se leen, se comprueban sus hashes y no se suben.

El instalador final permanece en fase 1.0. Esta ampliación no publica cambios ni introduce IA. Posteriormente el usuario autorizó específicamente miniz 3.1.0 para lectura ZIP; otras dependencias no quedan autorizadas por esa decisión.

## Primer incremento implementado: conector al cortar

En la interfaz existe la opción **Añadir pin y cavidad al cortar**, desactivada por defecto. Introducir diámetro del pin, profundidad, holgura **diametral** y pared mínima antes de cortar. El lado positivo lleva el pin; el negativo, una cavidad ciega. Valores iniciales: 3, 4, 0.2 y 1 unidades de modelo. Para 3MF son mm; STL no declara unidades, por lo que esos valores se interpretan en sus coordenadas, sin conversión automática.

El cilindro tiene 64 segmentos. Diámetros/holgura son nominales respecto a esa aproximación poligonal; no constituyen una garantía de ajuste impreso. El radio de cavidad es `(diámetro + holgura) / 2`; añade holgura axial de la mitad de la holgura diametral. La profundidad del pin se mide desde el plano hacia el lado negativo. El anclaje al positivo y la pared alrededor/fondo de la cavidad se comprueban con sólidos de guarda, circunscritos para no subestimar la pared por la discretización circular.

El motor busca hasta 25 posiciones sobre el plano, alrededor de la proyección del centro de normalización. Comprueba por diferencia booleana que los sólidos de guarda caben en cada mitad y valida los incrementos/decrementos de volumen nominales. Si no encuentra una posición segura dentro de esa búsqueda acotada, rechaza la operación completa sin modificar piezas ni Rehacer. Puede rechazar secciones que admitirían otra posición fuera de la búsqueda; no es un optimizador universal ni coloca múltiples pines.

Pin y cavidad se calculan con Manifold 3.5.4 dentro de `cutByPlane`, antes de convertir/publicar resultados. El volumen final ya no tiene por qué sumar el original: se descuenta el espacio de holgura; se verifica el delta esperado del pin y de la cavidad y después la conversión a Mesh. Se mantienen cierre/orientación, límites de triángulos/vértices/buffers, cancelación cooperativa y exportación validada float32. Las booleanas pueden demorar la cancelación, como el corte existente.

El corte conectado es un solo paso del historial; Deshacer/Rehacer recuperan geometría, piezas, plano y parámetros/modo del conector. Cambiar parámetros modifica el estado actual y no crea un paso independiente. No se implementa aún una operación para añadir conectores a una pareja arbitraria de piezas ya existentes; este incremento actúa al cortar una pieza.

### CLI de edición

```json
{"commands":[{"op":"cut","normal":[0,0,1],"offset":10,
"connector":{"diameter":3,"depth":4,"clearance":0.2,"wall":1}},
{"op":"undo"},{"op":"redo"},{"op":"export","piece":2}]}
```

Recibe el modelo por stdin con `--edit-stdin`. El ejemplo requiere una pieza con sección y espesor suficientes alrededor de z=10. Los cuatro campos son obligatorios si se proporciona connector. Sin ese objeto se mantiene el corte sin conectores. Los estados JSON informan parámetros y activación del conector. La exportación usa la validación compartida de 0.2.3.

### Evidencia local

- Compilación/paquete Release 0.3.0, sin dependencias nuevas: `build/connector-build.log` y `build/connector-package.log`.
- 16/16 casos inline de conectores: dimensiones por volumen poligonal esperado, exportaciones verificadas, undo/redo y parámetros, cuatro planos, normal escalada, holgura cero explícita, límites de parámetros, pared insuficiente y conservación de Rehacer tras rechazo. Reporte `build/connector-inline-results.json`.
- 22/22 regresiones de historial, `build/connector-history-regression.json`. Regresión geométrica/importación en `build/connector-geometry-regression.json`, con el caso trasladado bajo el contrato de exportación validada 0.2.3.
- Paquete provisional `STLCutStudioAI-0.3.0-portable-win-x64-20261109T025345331Z.zip`, SHA-256 `c70552463fde6399f7267c9ed4ffbc44c3ad4368f79dcbc876f737cbc9005f92`. Inventario de 74 archivos más manifiesto y **25/25** aceptaciones del workflow ejecutadas localmente contra el ZIP extraído a ruta con espacios/ñ y PATH mínimo. Dos casos añadidos de conectores: dimensiones/exportación/historial y rechazo por pared insuficiente. Reporte `build/connector-package-verification.json`.
- Sin revisión visual por instrucción expresa del usuario. Los controles de interfaz están compilados; su uso visible no se declara verificado en este incremento. Sin archivos de pruebas propios ni modelos del usuario modificados/publicados. Sin commit/push ni nueva CI remota.

### Trabajo que sigue pendiente

Preservar objetos/colores en el importador y decodificar las variantes de pintura; construir sólidos por color con ambos modos elegidos; detectar contactos entre esas regiones y conectarlas; ampliar el historial para transacciones generales entre múltiples piezas y conservar metadatos de color. Los conectores al cortar ya implementados no equivalen a separación automática por color ni a reconstrucción volumétrica. Esos objetivos siguen autorizados y no requieren volver a pedir aprobación del mismo alcance.

## Incremento 0.3.1 implementado

Importación con metadatos estándar, RGB por vértice en presentación, separación de regiones sólidas, transacción múltiple de historial y conectores automáticos para dos regiones con contacto plano en ejes. 20 casos de color, 63 de geometría, 22 de historial, 16 de conectores y 28 del paquete correctos. Contrato y límites en [COLOR_IMPORT.md](COLOR_IMPORT.md). Este incremento resuelve parte del trabajo listado arriba; la pintura de fabricante, ambos modos volumétricos y las redes generales de contactos siguen pendientes y autorizados. Sin revisión visual, push ni CI remota nueva.

## Lectura de pintura 0.3.2 en validación

Se añadió miniz autorizada, lectura de partes XML/JSON, codec de subdivisiones y presentación CPU sin modificar geometría. No son todavía sólidos por color. Compilación y paquete completados; validación dinámica final bloqueada por la política de Control de aplicaciones de Windows. Evidencia preliminar, límites y siguiente paso en [PAINT_IMPORT.md](PAINT_IMPORT.md). Insertos/partición volumétrica siguen pendientes y autorizados, pero no se debe dar por validada su base antes de completar estas comprobaciones.
