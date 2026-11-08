# STL Cut Studio AI

Aplicación de escritorio prevista para Windows 10/11 x64 orientada a importar, visualizar y, en fases posteriores, cortar y editar mallas 3D.

**Estado al 2026-11-07:** aplicación C++20/Qt compilada Release x64 con importación STL ASCII/binaria, estadísticas y visor 3D OpenGL. Renderizado, órbita, zoom, paneo, encuadre, ejes y cuadrícula comprobados mediante capturas y eventos dirigidos. Paquete portátil comprobado localmente y CI Windows verificada con artefacto descargable. Windows limpio, segundo equipo del usuario y validación ampliada pendientes.

## Documentación

- [Instrucciones de trabajo](AGENTS.md).
- [Documento maestro](docs/MASTER.md).
- [Requisitos de fase 0.1](docs/REQUIREMENTS.md).
- [Arquitectura y decisiones pendientes](docs/ARCHITECTURE.md).
- [Plan de pruebas](docs/TEST_PLAN.md).
- [Registro de cambios](docs/CHANGELOG.md).
- [Uso, contratos y validación del importador STL](docs/STL_IMPORT.md).
- [Visor 3D: controles, arquitectura y validación](docs/VIEWER.md).
- [Mediciones de carga, memoria y cancelación](docs/PERFORMANCE.md).
- [Compilación y paquete portátil](docs/BUILD_AND_RELEASE.md).
- [CI Windows x64 y artefactos](docs/CI.md).

La fase 0.1 cubre ventana Qt, STL ASCII/binario, visor y pipeline. El usuario aprobó el primer incremento; los siguientes cambios requieren aprobación previa. No se crean carpetas ni archivos de pruebas por instrucción del usuario. Consulte [configuración Windows](docs/WINDOWS_SETUP.md) y [dependencias](THIRD_PARTY_NOTICES.md) para pasos previstos y limitaciones.

No se requieren API keys para el alcance local descrito de la fase 0.1. No hay servicios externos ni motores IA configurados.
