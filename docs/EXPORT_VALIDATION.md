# Exportación STL validada — 0.2.3

Ajuste aprobado tras la validación real de 0.2.2. La conversión de double a float32 puede colapsar triángulos o perder precisión aunque el corte y su volumen sean correctos. Se rechaza esa exportación; no se repara ni se desplaza la geometría.

## Contrato compartido

`core/io/StlWriter::validatedStl(mesh, requireClosed, stop)` serializa en memoria, reimporta y compara conteo y coordenadas (error máximo extensión × 1e-6). Para piezas cortadas exige cierre, orientación y ausencia de degeneraciones mediante verifyClosedMesh con extensión × 1e-9. Verifica volumen con tolerancia `max(abs(volumen) × 1e-5, extensión³ × 1e-8)` y atiende cancelación cooperativa. Devuelve bytes únicamente tras completar todos los controles.

Interfaz, `--edit-stdin` y `--cut-stdin` usan esa misma función sin depender de Qt en el núcleo. El original exportado desde interfaz/edición mantiene su política anterior: precisión y volumen comprobados, sin exigir que una malla originalmente abierta sea cerrada. `writeStl` continúa como serializador de bajo nivel, sin promesa de validación de su salida.

- Interfaz: valida antes de abrir QSaveFile; conserva reemplazo atómico y protección del origen.
- Edición JSON: error `ok:false`, exit 1, sin añadir la exportación inválida a `exports`. Puede conservar exportaciones anteriores válidas de esa petición; no se deben confundir con la pieza rechazada. Las piezas, pasos de historial y Rehacer permanecen; la selección explícita del comando se conserva, igual que antes.
- Corte CLI: valida antes de escribir STL a stdout. Un rechazo de validación devuelve exit 1, diagnóstico en stderr y stdout vacío. Los fallos de escritura del propio stdout también se notifican; los consumidores deben descartar stdout ante cualquier exit distinto de cero.

No se relajan tolerancias. Los casos antes aceptados por el CLI pueden rechazarse ahora si su STL pierde precisión o cierre. Esto no significa que el cálculo double haya fallado. La validación requiere buffers y una reimportación adicionales; no es una cuota de RAM ni garantiza latencia máxima.

## Validación local

- Release x64 0.2.3 compilado; Qt 6.8.3, lib3mf 2.5.0 y Manifold 3.5.4 sin cambios. Logs `build/export-validation-build.log` y `build/export-validation-package.log`.
- 22/22 regresiones de historial, `build/export-history-regression.json`.
- Regresión geométrica/importación: 62 casos anteriores sin cambios correctos; el caso trasladado se actualizó al contrato nuevo y se verificó por separado: corte double y volumen correctos, salida float32 imprecisa rechazada sin bytes. Resultado consolidado 63/63, `build/export-geometry-regression.json`. La primera ejecución registró 62/63 porque esperaba la aceptación antigua; se conserva este detalle para no ocultar el cambio de criterio.
- Modelos reales R1/R3/R4/R5: secuencias de cortes, undo/redo y sus doce exportaciones correctas. R6: pieza #4 válida; #5 rechazada tras conservar esa exportación anterior, con las tres piezas activas y dos pasos. Otra petición rechazó #3 tras deshacer y conservó Deshacer 1/Rehacer 1 y piezas #2/#3. Error explícito «STL float32 no conserva una pieza cerrada y valida». Las dos salidas inválidas detectadas en 0.2.2 ya no se emiten como exportaciones válidas. Reporte privado `build/export-real-validation.json`; hashes de fuentes intactos.
- Paquete provisional `STLCutStudioAI-0.2.3-portable-win-x64-20261109T021858429Z.zip`, SHA-256 `5d19efa5e2e9a146c7fb77a22b55f85b4c20bc4e667fcb2e4d849f12c3643b5f`. CRC, inventario de 74 archivos más manifiesto y **23/23** casos del workflow correctos localmente, extraído a ruta con espacio/ñ y PATH mínimo. Dos casos nuevos comprueban rechazo float32, conservación de sesión y ausencia de stdout inválido. Evidencia `build/export-package-verification.json`.

No se repitió el selector nativo ni una sesión visual: la ruta de guardado conserva su comportamiento y llama a la misma función compilada y ejercitada por CLI. Sin nuevas dependencias, archivos de pruebas propios, commit/push ni CI remota de 0.2.3. La última CI remota es 0.2.2. No se entregó instalador final ni se declara terminada la fase 0.2; la reparación de geometría y los límites de corte siguen según MANIFOLD_CUT.md.
