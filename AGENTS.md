# AGENTS.md — STL Cut Studio AI

## Propósito
Este repositorio especifica una aplicación nativa Windows para cortar, editar, reparar y generar modelos 3D. Leer `docs/MASTER.md` antes de cualquier cambio.

## Normas para agentes Codex
- Trabajar en incrementos pequeños; ejecutar pruebas antes de afirmar que algo funciona.
- C++20 + Qt 6 + CMake como núcleo; Python opcional para IA.
- Mantener geometría independiente de la UI y de servicios de IA.
- No introducir dependencias sin fijar versión, documentar licencia y explicar necesidad.
- Mantener `vcpkg.json`, configuración/baseline, CMake Presets y lock de Python alineados.
- No guardar secretos, binarios de compilación, venv o pesos de IA en Git.
- Validar en el equipo de desarrollo Windows x64 y en CI; registrar sus versiones y diferencias. No exigir un segundo PC físico.
- No suponer que un exe aislado funciona: empaquetar y probar DLL, plugins y runtimes.
- Los datos STL/imágenes son no confiables; validar entrada y evitar ejecutar código generado sin aislamiento.
- No usar nombres de personajes o marcas protegidas como fixtures predeterminados.
- Al finalizar cada tarea: resumir cambios, pruebas ejecutadas, resultados, limitaciones y siguiente paso.

## Orden de trabajo
1. Leer `docs/MASTER.md`.
2. Inspeccionar el código existente.
3. Proponer plan acotado y obtener aprobación del usuario antes de realizar cambios.
4. Implementar.
5. Compilar y probar.
6. Actualizar documentación.

## Primera instrucción
Alcance vigente autorizado el 2026-11-08: continuar fase 0.2 e incorporar importación 3MF junto a STL, corte por plano y exportación STL. La instrucción inicial de limitarse a 0.1 queda sustituida por esta autorización. No comenzar IA, conectores ni instalador final en este incremento. Ampliación aprobada: Manifold 3.5.4 para sustituir el corte, admitir cavidades/casos coplanares y retirar el tope de 4096 vértices de contorno, conservando validaciones y límites de recursos; véase docs/MANIFOLD_CUT.md.

Ampliación aprobada 0.2.2: cortes encadenados sobre piezas activas, procedencia/selección, Deshacer/Rehacer y presupuesto de historial compartido durante la sesión; validación inline y documentación en docs/HISTORY.md. Sin formato de proyecto persistente ni nuevas dependencias.

Ajuste aprobado 0.2.3: compartir validación STL de precisión, cierre y volumen entre interfaz y CLI; rechazar conversión float32 inválida conservando sesión/fuentes. Sin reparación automática ni dependencias nuevas; evidencia en docs/EXPORT_VALIDATION.md.

Ampliación aprobada fase 0.3: conectores pin/cavidad con parámetros e historial; objetivo añadido de separación automática por color de 3MF con conectores entre regiones vecinas. Sustituye la restricción anterior de no comenzar conectores. Omitir revisión visual por decisión del usuario; ambos tipos de 3MF y ambos modos de pintura (insertos y partición volumétrica) ya fueron elegidos. Incremento estándar 0.3.1 en docs/COLOR_IMPORT.md; pintura/reconstrucción pendientes según docs/PHASE_03.md.

Ampliación posterior aprobada: miniz 3.1.0 MIT, descarga fijada por SHA-256 y lectura ZIP en memoria sin extracción ni instalación global. Lectura de pintura 0.3.2 compilada y publicada con aprobación; validación remota satisfactoria en ejecución CI 7. La ejecución local continúa bloqueada por Control de aplicaciones, según docs/PAINT_IMPORT.md y docs/CI.md. No eludir esa protección.

## Documentación y autorización

- Publicación autorizada 0.3.5/0.3.6 completada en 8798204 y ajuste de generador a31f398. CI 10 satisfactoria (32+92 casos CPU, build/paquete); revisión gráfica nueva pendiente y política local sin alterar. Las menciones de publicación/aceptación remota pendientes siguientes son históricas. docs/CI.md y docs/VIEWER.md.

- 2026-11-10: usuario autorizó publicar 0.3.5 y 0.3.6 y validar código/regresiones en CI. La revisión gráfica nueva permanece pendiente por bloqueo local; no se autoriza cambiar ni eludir seguridad. Las notas de ausencia de publicación siguientes son históricas.

- Incidencia visual aprobada por el usuario: 0.3.6 local corrige normales de facetas pequeñas/pintura y mejora MSAA/profundidad; revisión visible autorizada para esta incidencia. docs/VIEWER.md. Release compilado; primera ejecución bloqueada con 4551, no eludir protección. Aceptación dinámica/visual pendiente. La construcción del historial 0.3.5 se conserva.

- Continuación de fase autorizada: incremento local 0.3.5 para conservar parámetros de reconstrucción en historial/CLI y restaurar controles; docs/HISTORY.md. Sin nuevas dependencias ni revisión visual. La autorización anterior de publicación 0.3.3/0.3.4 ya fue completada; no implica publicar automáticamente incrementos posteriores.

- Publicación 0.3.3/0.3.4 completada en main dbacc4a y CI 8 satisfactoria (32+72 casos, build/paquete). Se completó la aceptación remota pendiente. El bloqueo local 4551 no se modificó ni se debe eludir. Véanse docs/CI.md y docs/REGION_CONNECTORS.md; los estados de no publicación/aceptación pendiente siguientes son históricos.

- Publicación posterior autorizada por el usuario: subir juntos 0.3.3 y 0.3.4 a GitHub y completar su validación en CI Windows. La restricción de no publicar de las notas siguientes describe el estado previo a esta autorización. No se autoriza cambiar ni eludir la política de seguridad local.

- Continuación 0.3.4 autorizada: conectores entre regiones reconstruidas con guardas/barrido y omisiones explícitas; docs/REGION_CONNECTORS.md. Release/ZIP terminados pero batería final bloqueada de nuevo por Windows 4551; no ejecutar copias, variantes ni cambiar la protección para eludirlo. Pendiente de validación en entorno autorizado. Sin publicación nueva autorizada para estos incrementos locales.

- Continuación autorizada 2026-11-09: incremento local 0.3.3 con ambos modos de volumen desde pintura por celdas, resolución/profundidad explícitas y validación transaccional. Contrato en docs/PAINT_VOLUME.md. Conectores de estas interfaces pendientes. El CLI recién compilado pudo ejecutarse en validación sin cambios de seguridad; no se interpreta el bloqueo anterior como autorización para eludir políticas.
- Leer también docs/REQUIREMENTS.md, docs/ARCHITECTURE.md y docs/TEST_PLAN.md antes de implementar.
- Documentar por desarrollo requisitos, versiones, librerías, frameworks, configuración y resultados de pruebas.
- Registrar nombres y finalidad de variables o API keys cuando correspondan; nunca sus valores secretos.
- La aprobación del ajuste documental no autoriza implementar la fase 0.1 ni instalar herramientas.
- Las alternativas tecnológicas del maestro no constituyen dependencias aprobadas. Consultar las decisiones pendientes en docs/ARCHITECTURE.md.
- Decisión vigente de entrega: instalador final `.exe` para Windows x64 con las dependencias necesarias, aplicación lista para abrir sin instalar Qt, Python ni herramientas de desarrollo manualmente y validación de instalación en VM Windows limpia. Corresponde a fase 1.0; el ZIP portátil de fase 0.1 es provisional. Esta decisión documental no autoriza implementar el instalador todavía.
