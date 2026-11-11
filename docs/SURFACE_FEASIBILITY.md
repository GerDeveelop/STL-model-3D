# Viabilidad por instancia e índice de superficies — 0.3.7

> Actualización 2026-11-10: publicación 15545e9 y [CI 11](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42955441598) satisfactorias, build/paquete y **32+113 comprobaciones correctas**. Aceptación CPU de esta base completada. El caso sintético de 120 000 caras también pasó. Las menciones de aceptación remota pendiente siguientes describen el estado local previo; no se convierten las ejecuciones locales bloqueadas en aprobadas. Siguen pendientes benchmark real C++/RSS, revisión de la nueva interfaz y selección por clic. Entorno/artefacto/hashes en [CI.md](CI.md).

Fecha: 2026-11-10. Primer incremento del orden aprobado para selección interactiva de fase 0.3. Aún no implementa clic/resaltado, selección conectada por color ni extracción dirigida. No introduce dependencias, modifica modelos originales ni amplía silenciosamente límites de reconstrucción/conectores. Estado de aceptación al final.

## Diagnóstico por instancia

`core/mesh/MeshFeasibility` agrupa por identidad de instancia de TriangleStyle; un STL sin instancias usa identidad virtual 0. Informa caras fuente, facetas de presentación/pintura, etiquetas no resueltas/interpoladas, materiales y presupuesto vigente de reconstrucción. No identifica materiales únicamente por RGB.

El filtro topológico une vértices por **coordenadas exactamente iguales** (incluye equivalencia +0/-0), cuenta caras degeneradas, bordes de una cara, bordes con más de dos caras y balances de orientación incoherentes entre dos caras. Cuenta componentes de superficie por aristas compartidas y volumen firmado. Divide la suma de determinantes una sola vez al final para evitar redondeo acumulado por cara. Componentes de superficie no equivalen siempre a piezas físicas: una cavidad puede ser una envolvente desconectada, y varias piezas pueden compartir una instancia de procedencia.

`closed_oriented` solo describe cierre/orientación coherente de aristas en este filtro. `positive_volume` es una comprobación separada. **No certifica ausencia de autointersecciones, cierre con tolerancia, validez Manifold, contención de cavidades, ausencia de solapes entre instancias ni ensamblabilidad.** La extracción futura deberá ejecutar sus validaciones completas. No se repara, invierte ni simplifica ninguna malla.

Hasta 256 instancias y 500 000 caras analizadas topológicamente por instancia, de forma secuencial para liberar mapas temporales entre ellas. Una instancia mayor devuelve topology_analyzed=false y closed_oriented/volume=null; no se etiqueta como inválida por no haberse analizado. El lector conserva sus límites de entrada. Estos topes de trabajo no son cuota rígida de memoria/RSS; mapas de vértices/aristas pueden requerir memoria adicional.

Interfaz: **Analizar viabilidad por instancia**, sobre la pieza visible, con trabajador cancelable. Presenta un resumen por instancia y conserva geometría, selección, historial y Rehacer; no es una edición. No confundir instancia con cuerpo conectado ni con región de color.

CLI de solo lectura, modelo por stdin:

```text
STLCutStudioAI.exe --inspect-feasibility-stdin
```

Devuelve JSON con instances, elapsed_ms, exact_coordinate_screening=true, self_intersections_checked=false y source_modified=false. Cada instancia informa topology_analyzed, closed_oriented, positive_volume, signed_volume, surface_components, contadores de defectos, colores y current_reconstruction_budget. Este último compara solamente caras/facetas con los topes 100 000 actuales; no implica autorización completa para extraer.

## Índice espacial BVH

`core/mesh/SurfaceIndex` expande las caras o subdivisiones pintadas, conservando material, instancia y número global de cara fuente. Normaliza las posiciones respecto a su caja real, sin modificar la fuente. Puede construirse para toda la malla o una instancia concreta. Hasta 1 millón de facetas expandidas y 256 MiB de buffers propios contabilizados; esos límites pertenecen al índice de consultas y no autorizan reconstruir un millón de caras.

Árbol binario de cajas envolventes: división mediana por centroides en el eje más largo, hojas de hasta ocho triángulos. Búsqueda punto-superficie con poda por distancia a las cajas. Dos recorridos: mínimo de distancia cuadrada y después selección determinista de empates dentro de 1e-15 en coordenadas normalizadas, por (material, instancia, cara fuente, faceta). El mínimo geométrico se mantiene separado del desempate. La normalización y poda incorporan margen numérico; se rechazan facetas degeneradas/extremadamente pequeñas (área vectorial normalizada <=1e-30) y puntos fuera del rango numérico.

Cada consulta carga visitas de nodos y evaluaciones de triángulos a un presupuesto acumulado de hasta 100 millones de unidades. Se comprueba cancelación en construcción, particionado y recorridos. La búsqueda exhaustiva de referencia está disponible para diagnóstico y usa el mismo contrato de distancia/desempate sin poda. Los contadores/bytes no equivalen al RSS del proceso ni garantizan tiempos máximos.

```text
STLCutStudioAI.exe --inspect-spatial-stdin '{"points":[[0,0,0],[10,10,10]],"verify":true}'
```

JSON del argumento: points (1–1024 puntos finitos en coordenadas del modelo), instance opcional (ID de procedencia, no ordinal de lista), work opcional (1–100 000 000), verify opcional booleano. Retorna hits con distance/material/source_triangle/instance/facet, buffers del índice, nodos, contadores y tiempos de construcción/consultas. verify compara distancia y faceta elegida con el recorrido exhaustivo y falla si difieren; su presupuesto es independiente pero tiene el mismo límite. No repara ni convierte una superficie seleccionada en sólido. Estas consultas aún no son selección de pantalla: falta intersección de rayo, conectividad de color y resaltado.

## Uso en reconstrucción

PaintVolume sustituye la comparación de cada celda contra todas las facetas por la consulta BVH. Se retira el rechazo anticipado basado solo en `celdas * facetas > 100M`; se conserva el presupuesto real acumulado de nodos/evaluaciones. Los casos que caben gracias a poda pueden ejecutarse sin ampliar automáticamente el tamaño de celda. Cambia el desempate antiguo dependiente del orden por el mínimo global/tie determinista documentado arriba.

Se mantienen **100 000 caras/facetas de reconstrucción, 64 materiales, 64 celdas/eje, 8192 bloques, 256 piezas, verificación de cierre/volumen/STL y límites de conectores**. El modelo privado sigue superando el tope de reconstrucción: la existencia del índice no basta para habilitar su extracción. Se requiere medir el nuevo código sobre sus instancias, conservar los contactos con las demás y validar el flujo antes de ampliar topes. El suavizado visual, selección por clic, extracción dirigida y pincel/lazo mantienen el orden de PHASE_03.md.

## Evidencia disponible y pendiente

- Release compilado, logs build/spatial-configure.log, spatial-core-build.log y spatial-build.log. Sin dependencias nuevas. Persiste C4244 de la cabecera externa de Manifold; una advertencia propia de variable ocultada se corrigió renombrando el dato expandido.
- Compilación preliminar: **109/112** casos correctos. Pasaron las regresiones previas adaptadas y consultas indexadas/exhaustivas, identidad/desempate, distancias analíticas, topología y presupuesto. Tres igualdades exactas de volumen diagnosticaron el redondeo de dividir cada cara antes de sumar; se corrigió dividiendo al final, como en el motor existente. Es evidencia preliminar, no del binario final.
- El primer intento del binario corregido fue bloqueado antes de ejecutar por **Windows 4551**. El workflow se detuvo en ese primer bloqueo; no se hicieron intentos alternativos ni se cambió la protección. No se atribuyen los 109 casos preliminares a este binario. La batería preparada incorpora además una instancia sintética de 120 000 caras: previsto **32+113 casos**, pendientes de ejecución final.
- Referencia independiente Python sobre el modelo privado de las capturas: cuatro instancias ordinales con 174 532, 3072, 174 532 y 3072 caras; componentes de superficie 4,1,4,1. Sin bordes abiertos/no manifold, orientación incoherente ni degeneraciones en ese filtro exacto; volúmenes firmados positivos aproximados 23554.0673,74.6226,23554.0673,74.6226 mm³. No es ejecución del C++ nuevo ni certificación de extracción/ausencia de autointersecciones. Reporte privado build/feasibility-real-reference.json, fuente intacta SHA-256 `1dd19c2615fc74295a3a42ab27b5bd42c556318406228b67e3c2e131b604e90c`. Sus ordinales no se deben confundir con los IDs de SDK que devuelve la aplicación.

Paquete provisional `build/packages/20261111T033835542Z/STLCutStudioAI-0.3.7-portable-win-x64-20261111T033835542Z.zip`, SHA-256 `291562f6aa747782c6dd99c6107d57f6243951a91ed33c67b77f915ea284d8e6`. Solo comprobación estática: CRC, inventario de 75 archivos más manifiesto y todos los tamaños/hashes. El exe coincide con el bloqueado: SHA-256 `263c156aa13b95bdcdccaaca4e57ac05ecae81d2e5e0d54fba5c53bb2f5c4484`. Reportes build/spatial-package-static-verification.json, spatial-inline-results.json (bloqueo), spatial-preliminary-summary.json y spatial-code-integrity.json. Los dos bloques Python del workflow se analizaron sintácticamente sin ejecutar el exe bloqueado. El nombre del paquete usa UTC; fecha local del incremento 2026-11-10.

Pendiente medir tiempos/RSS/bytes del índice y validar resultados del C++ final, especialmente en ese modelo; no hay benchmark real aprobado ni límites ampliados para extraerlo. Sin revisión visual de la nueva interfaz, fuentes/carpetas de pruebas, publicación ni nueva CI remota. El siguiente incremento, después de aceptar esta base, es selección por clic y región conectada con resaltado; no se declara construido todavía.
