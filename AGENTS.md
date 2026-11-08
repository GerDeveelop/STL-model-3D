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

## Documentación y autorización
- Leer también docs/REQUIREMENTS.md, docs/ARCHITECTURE.md y docs/TEST_PLAN.md antes de implementar.
- Documentar por desarrollo requisitos, versiones, librerías, frameworks, configuración y resultados de pruebas.
- Registrar nombres y finalidad de variables o API keys cuando correspondan; nunca sus valores secretos.
- La aprobación del ajuste documental no autoriza implementar la fase 0.1 ni instalar herramientas.
- Las alternativas tecnológicas del maestro no constituyen dependencias aprobadas. Consultar las decisiones pendientes en docs/ARCHITECTURE.md.
- Decisión vigente de entrega: instalador final `.exe` para Windows x64 con las dependencias necesarias, aplicación lista para abrir sin instalar Qt, Python ni herramientas de desarrollo manualmente y validación de instalación en VM Windows limpia. Corresponde a fase 1.0; el ZIP portátil de fase 0.1 es provisional. Esta decisión documental no autoriza implementar el instalador todavía.
