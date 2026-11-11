# STL Cut Studio AI

**Incremento local 0.3.7 en aceptación:** diagnóstico por instancia y búsqueda de superficies mediante índice espacial BVH, integrada en la reconstrucción. Primer paso del orden aprobado para selección por clic; clic/resaltado aún pendientes. Release compilado, pero Windows bloqueó el primer arranque del binario final. No se ampliaron límites de extracción ni se modificaron fuentes. [Contrato y evidencia](docs/SURFACE_FEASIBILITY.md). Sin publicación/CI nueva de este incremento.

**Estado 0.3.6 publicado (2026-11-10):** 0.3.5/0.3.6 subidos a main; [CI 10](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42948952405) aprobó compilación, paquete y **124 comprobaciones CPU** (32+92). Normales estables y parámetros de historial aceptados dentro del alcance probado. **La revisión visual de nitidez/MSAA sigue pendiente** por bloqueo local, sin alterar seguridad. [Evidencia y límites](docs/VIEWER.md). Las notas locales siguientes son históricas.

**Incremento local 0.3.5:** modo, resolución y profundidad de reconstrucción forman parte del historial; Deshacer/Rehacer restaura los controles. CLI añade paint-settings y paint_volume en el estado. Release y 85/85 aceptaciones inline correctas. [Contrato del historial](docs/HISTORY.md). Sin publicación ni nueva CI remota de este incremento; la versión publicada siguiente sigue siendo 0.3.4.

**Publicación anterior 0.3.4 (2026-11-09):** incrementos 0.3.3/0.3.4 publicados en main dbacc4a. [CI 8](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42751423136) satisfactoria: build/paquete, **32+72 comprobaciones correctas** y artefacto. Incluye ambos modos de volumen desde pintura y conectores locales compatibles, con omisiones explícitas. [Contrato y límites](docs/REGION_CONNECTORS.md). La aceptación remota quedó completada sin cambiar la seguridad local ni hacer revisión visual; fase 0.3 aún abierta. Los registros locales siguientes conservan su contexto histórico.

**Incremento local 0.3.3 (2026-11-09):** construcción de volúmenes desde pintura con dos modos seleccionables: insertos con profundidad y partición completa, mediante celdas de resolución explícita. Release y 54 comprobaciones de pintura/volumen correctas. Contrato y límites en [PAINT_VOLUME.md](docs/PAINT_VOLUME.md). Los conectores para estas interfaces siguen pendientes. Sin publicación ni nueva CI remota; el estado 0.3.2 siguiente corresponde a la versión publicada.

Aplicación de escritorio Windows 10/11 x64 para importar STL/3MF, visualizar mallas, cortar por plano y exportar piezas STL.

**Estado al 2026-11-08:** versión 0.3.2 publicada en main (d2a219a): lectura de pintura 3MF, colores/regiones y conectores del alcance documentado. CI Windows ejecución 7 satisfactoria: build, paquete, 32 comprobaciones generales y 36 de pintura. Evidencia en [CI.md](docs/CI.md). El bloqueo local 4551 permanece; no se modificó la protección ni se hizo revisión visual. Insertos y partición volumétrica siguen pendientes.

La entrega final acordada es un instalador `.exe` para Windows x64 con las dependencias necesarias, para abrir la aplicación sin instalar Qt, Python ni herramientas de desarrollo manualmente. El ZIP portátil es provisional. Se elimina el segundo PC físico como requisito y se valida la instalación final en una VM Windows limpia.

## Documentación

- [Instrucciones de trabajo](AGENTS.md).
- [Cortes encadenados y Deshacer/Rehacer](docs/HISTORY.md).
- [Lectura de pintura y estado de validación](docs/PAINT_IMPORT.md).
- [Color y separación de cuerpos 3MF](docs/COLOR_IMPORT.md).
- [Fase 0.3: diseño de conectores y separación por color](docs/PHASE_03.md).
- [Documento maestro](docs/MASTER.md).
- [Motor Manifold: capacidades, límites y evidencia](docs/MANIFOLD_CUT.md).
- [Exportación STL validada](docs/EXPORT_VALIDATION.md).
- [Validación con modelos reales y pendiente float32](docs/REAL_MODEL_VALIDATION.md).
- [Fase 0.2: uso, límites y validación](docs/PHASE_02.md).
- [Requisitos](docs/REQUIREMENTS.md).
- [Arquitectura y decisiones pendientes](docs/ARCHITECTURE.md).
- [Plan de pruebas](docs/TEST_PLAN.md).
- [Registro de cambios](docs/CHANGELOG.md).
- [Uso, contratos y validación del importador STL](docs/STL_IMPORT.md).
- [Visor 3D: controles, arquitectura y validación](docs/VIEWER.md).
- [Mediciones de carga, memoria y cancelación](docs/PERFORMANCE.md).
- [Compilación y paquete portátil](docs/BUILD_AND_RELEASE.md).
- [CI Windows x64 y artefactos](docs/CI.md).

La fase 0.1 cubre ventana Qt, STL ASCII/binario, visor y pipeline. El usuario aprobó continuar la fase 0.2 y añadir importación 3MF; el incremento actual está descrito en PHASE_02.md. Ampliaciones fuera de este alcance requieren aprobación previa. No se crean carpetas ni archivos de pruebas por instrucción del usuario. Consulte [configuración Windows](docs/WINDOWS_SETUP.md) y [dependencias](THIRD_PARTY_NOTICES.md) para comandos y limitaciones.

No se requieren API keys para el alcance local descrito de la fase 0.1. No hay servicios externos ni motores IA configurados.
