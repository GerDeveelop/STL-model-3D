# STL Cut Studio AI

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
