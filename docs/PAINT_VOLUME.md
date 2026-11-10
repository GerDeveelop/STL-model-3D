# Reconstrucción volumétrica por color — incremento 0.3.3

> Actualización 0.3.4: la casilla y objeto JSON de conectores ahora se admiten para ambos modos con diagnóstico de omisiones. Véase [REGION_CONNECTORS.md](REGION_CONNECTORS.md). Compilados, pero su batería final está bloqueada por 4551 y su aceptación pendiente. Las prohibiciones de connector que siguen documentan la versión 0.3.3, al igual que sus 86 aceptaciones; no describen la ruta nueva ni validan el binario 0.3.4.

Fecha: 2026-11-09. Alcance previamente autorizado: insertos con profundidad configurable y partición completa del volumen. Sin dependencias nuevas, IA, instalador, publicación ni revisión visual. Estado de compilación/validación al final.

## Política geométrica explícita

`core/cutting/PaintVolume` clasifica los centros de una cuadrícula cartesiana por distancia euclídea al triángulo superficial más cercano, incluyendo las subdivisiones de pintura decodificadas. Conserva la identidad del material aunque dos materiales tengan el mismo RGB. En empates numéricos (distancias cuadradas normalizadas dentro de 1e-15) gana el índice de material menor. No es una inferencia del interior original del diseñador.

- **Partición completa:** todas las celdas reciben el material superficial más cercano.
- **Insertos:** reciben ese material solo las celdas cuyo centro queda a distancia menor o igual a la profundidad indicada. Las demás forman un núcleo explícitamente sin color asignado, presentado en gris; ese gris no se atribuye a un filamento del archivo. Se rechaza una profundidad que no deje núcleo o que sea menor o igual al error espacial de la celda.

Los bloques se unen por material y se intersectan con el sólido original mediante Manifold 3.5.4 ya incorporado. Se clasifican también centros externos para no perder detalles exteriores delgados. El exterior queda recortado por la geometría original; las interfaces **internas son escalonadas**. Los componentes desconectados se entregan como piezas independientes.

El tamaño de celda es un máximo, en unidades del modelo (mm para 3MF). Cada eje se divide en `ceil(dimensión / resolución)` intervalos iguales. El desplazamiento máximo entre un punto de una celda y su centro es la mitad de su diagonal, comunicado en las advertencias. Esa cota acota el error de muestreo de distancia para el umbral de profundidad; **no garantiza conservar detalles de pintura menores que una celda ni acota el desplazamiento de fronteras de color en zonas equidistantes**. Una resolución más fina puede ser necesaria. No se aplica suavizado ni reparación silenciosa.

Se requiere malla cerrada/orientada con volumen positivo, colores resueltos y etiquetas uniformes por cara o pintura subdividida. No admite interpolación por vértice, texturas ni gradientes. Si un material superficial desaparece del resultado, se rechaza toda la operación para pedir una resolución más fina. Cierre, volumen positivo, conservación del volumen total y exportabilidad STL float32 se comprueban **antes** de publicar el historial. Se permite una diferencia de volumen de `max(volumen*1e-7, extensión³*1e-12)`.

Las celdas tienen interiores disjuntos y cubren la caja completa; el recorte mantiene esa partición, salvo tolerancias numéricas del motor. No se certifica universalmente ausencia de autointersecciones del origen. Modelos con instancias en contacto o solapadas pueden ser rechazados como no manifold. No se garantiza ensamblabilidad de interfaces escalonadas con socavados.

## Uso e integración

Interfaz: elegir modo, tamaño máximo de celda y profundidad; pulsar **Construir piezas desde pintura**. Los controles de profundidad se habilitan para insertos. El procesamiento usa el trabajador cancelable y publica todas las piezas/vistas mediante la transacción de historial existente. Deshacer/Rehacer recupera la geometría; estos controles de reconstrucción todavía no forman parte de los parámetros recuperados del estado de sesión. El original permanece accesible y no se escribe en la fuente.

CLI de producción, modelo por stdin:

```json
{"commands":[{"op":"split-paint","mode":"partition","resolution":2}]}
```

```json
{"commands":[{"op":"split-paint","mode":"inserts","resolution":1,"depth":3}, {"op":"undo"}, {"op":"redo"}]}
```

Ambas rutas rechazan solicitar conectores en esta operación: la casilla debe estar desactivada y CLI no acepta `connector` en `split-paint`. No se ignoran solicitudes. Los conectores entre estas interfaces reconstruidas siguen pendientes; los conectores de corte/contactos planos ya existentes conservan su contrato. STL no conserva color; la sesión mantiene el material y el nombre de región.

## Presupuestos y cancelación

Máximo 100 000 caras fuente, 100 000 facetas superficiales expandidas, 64 materiales usados, 64 celdas por eje, 262 144 celdas y 100 millones de evaluaciones distancia superficie/celda. Se rechazan presupuestos superiores antes de clasificar; no se altera la resolución solicitada automáticamente. Hasta 8192 bloques unidos por material, 256 piezas resultantes y 2 millones de triángulos resultantes en total. Se mantienen los límites del historial y la exportación existente. Estos límites no son una cuota de RAM/tiempo del proceso; booleanas y descomposición pueden requerir memoria adicional.

Cancelación cooperativa entre caras/celdas/bloques/componentes y en la validación STL. `ExecutionContext` recibe cancelación; las booleanas inmediatas pueden demorar su atención hasta retornar, como en el corte existente. Fallo o cancelación no publica resultados parciales ni elimina Rehacer.

## Validación de este incremento

Release x64 compilado con Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4 y miniz 3.1.0 sin cambios de versión. Log `build/paint-volume-build.log`; persiste la advertencia C4244 de la cabecera externa math.h de Manifold ya documentada. Sin advertencias nuevas del código propio en esta compilación.

- **54/54** aceptaciones locales inline: las 36 de pintura existentes y 18 nuevas de volumen. Se comprobó la conservación de 16 000 mm³ de dos cubos desconectados, separación por material/componente, un parche pequeño y, en insertos con celda 5/profundidad 6, dos núcleos de 1000 mm³ y sus cavidades. Exportación/reimportación de todas las piezas de ambos modos, reflexión baricéntrica, undo/redo con las mismas piezas/IDs, preservación de Rehacer ante rechazo, profundidad sin núcleo/insuficiente, pérdida de color, paleta ausente, gradiente por vértice, resolución, presupuesto de trabajo y rechazo expreso de conectores. Reporte `build/paint-volume-inline-results.json`.
- **32/32** regresiones generales del paquete y **54/54** de pintura/volumen contra el ZIP extraído a carpeta con espacios/ñ y PATH reducido a Windows/System32. Incluye los casos existentes de corte, cavidades, geometría cóncava, contorno grande, historial y conectores. Reportes `build/paint-volume-package-verification.json` y `build/paint-volume-package-paint-verification.json`. No se atribuyen todos esos casos geométricos anteriores al nuevo algoritmo de pintura; sus 18 aceptaciones nuevas usan los modelos sintéticos descritos arriba.
- ZIP provisional: `build/packages/20261110T030126956Z/STLCutStudioAI-0.3.3-portable-win-x64-20261110T030126956Z.zip`, SHA-256 `7cfb186af1038ebbf7b90bb9fd47fe2ef6edee8ab27c953c2048a6f1db1528e4`. CRC, inventario de 75 archivos más manifiesto y hashes de cada entrada comprobados. Log `build/paint-volume-package.log`. El sello del nombre está en UTC; esta validación corresponde al 2026-11-09 en Colombia.
- El primer caso de insertos detectó que Decompose separaba la pared de cavidad como componente de volumen negativo. Se corrigió la asociación: recortar el resultado completo contra cada envolvente positiva, de menor a mayor volumen, restando las islas ya asignadas. Los 54 casos y el paquete se ejecutaron después de esa corrección.
- Workflow ampliado con los mismos 18 casos inline; **sin nueva ejecución remota ni commit/push**. La CI publicada 7 sigue correspondiendo a 0.3.2. Sin nuevas dependencias, fuentes/carpetas de pruebas ni modelos privados escritos/publicados. `prueba.stl` mantiene el SHA-256 `8c79e037add42bf0f227b4e3033d6a03cc1f194f9fcc34332326591b5f3d3cdf` y permanece fuera de Git.

No se realizó revisión visual ni nueva medición de modelos privados. La cancelación del motor tiene comprobaciones cooperativas pero no una nueva aceptación de interacción/cancelación desde la ventana. El CLI local nuevo sí se ejecutó sin modificar la seguridad; no se afirma que la política que bloqueó anteriormente 0.3.2 haya cambiado globalmente. El lanzamiento visible posterior de 0.3.2 con Qt desplegado es evidencia separada de estas aceptaciones sin ventana.

## Siguiente incremento

Añadir conectores a interfaces reconstruidas exige detectar áreas de contacto, elegir direcciones de montaje y verificar paredes/colisiones; no se debe reutilizar a ciegas la búsqueda de contactos planos en ejes. También quedan pendientes recuperar los parámetros de esta operación con el historial, ampliar rendimiento y casos geométricos de aceptación (curvatura, paredes delgadas, socavados y contacto complejo), y la validación/reparación de la fase. Este incremento no cierra la fase 0.3 ni promete separación universal de cualquier pintura 3MF.
