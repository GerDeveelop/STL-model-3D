# STL Cut Studio AI

Aplicación de escritorio Windows 10/11 x64 para importar STL/3MF, visualizar mallas, cortar por plano y exportar piezas STL.

**Estado al 2026-11-08:** versión de desarrollo 0.2.2 con STL/3MF, corte Manifold, cortes encadenados y Deshacer/Rehacer durante la sesión. Release y validaciones locales satisfactorios: 22 casos de historial, 63 de geometría/importación y 21 del paquete. Uso y límites en [HISTORY.md](docs/HISTORY.md). Código publicado en main como 34b0cb7; CI Windows 0.2.2 satisfactoria (ejecución 6: build, paquete y 21 casos), con evidencia en [CI.md](docs/CI.md). La fase 0.2 continúa y el instalador final corresponde a 1.0.

La entrega final acordada es un instalador `.exe` para Windows x64 con las dependencias necesarias, para abrir la aplicación sin instalar Qt, Python ni herramientas de desarrollo manualmente. El ZIP portátil es provisional. Se elimina el segundo PC físico como requisito y se valida la instalación final en una VM Windows limpia.

## Documentación

- [Instrucciones de trabajo](AGENTS.md).
- [Cortes encadenados y Deshacer/Rehacer](docs/HISTORY.md).
- [Documento maestro](docs/MASTER.md).
- [Motor Manifold: capacidades, límites y evidencia](docs/MANIFOLD_CUT.md).
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
