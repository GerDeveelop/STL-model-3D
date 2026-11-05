# Arquitectura y decisiones

Fecha: 2026-11-05. Estado: diseño previsto; no hay módulos implementados.

## Separación de responsabilidades

- `apps/desktop`: ventana, diálogos, comandos y presentación de errores.
- `core/io`: lectura STL y errores estructurados, sin dependencia de la interfaz.
- `core/mesh`: datos de malla, estadísticas y caja envolvente.
- `core/validation`: comprobaciones de entrada y límites de recursos.
- Adaptador del visor: convierte la malla al formato de renderizado sin trasladar reglas geométricas a la UI.
- `tests`: fixtures propios, pruebas unitarias y de integración.
- `scripts` y CI: configuración, compilación, pruebas y paquete reproducible.

Flujo previsto: seleccionar archivo → validar límites → importar → validar datos → calcular estadísticas → presentar malla y resultados. Un fallo debe producir un error comprensible y mantener la aplicación utilizable. La política para conservar el modelo anterior ante fallo debe definirse antes de implementar.

Las tareas extensas deben ejecutarse con un mecanismo que mantenga operativa la UI; hilos, cancelación y propiedad de datos todavía requieren diseño. La geometría no dependerá de servicios IA.

## Decisiones pendientes de aprobación

| Tema | Alternativas o cuestión | Estado |
|---|---|---|
| Interfaz | Qt Widgets o QML | Por decidir |
| Renderizado | VTK integrado con Qt o alternativa justificada | Por decidir |
| Dependencias | Distribución de Qt, vcpkg, baseline y versiones exactas | Por verificar y fijar |
| Toolchain | MSVC, SDK, CMake y Ninja compatibles | Entorno sin inspeccionar |
| Enlace | Dinámico o estático y efecto en distribución | Por decidir |
| Licencia del producto | Uso personal, código abierto o comercial | Por confirmar |
| Importación | Parser propio o biblioteca, representación y límites | Por decidir |
| Geometría | CGAL u otra solución para fases posteriores | Fuera de fase 0.1 |
| CI | Repositorio remoto, runner Windows y permisos | Sin configurar |
| Paquete inicial | Estructura Release, DLL, plugins y runtimes | Por diseñar y probar |

Ninguna alternativa de esta tabla es una dependencia aprobada. Antes de incorporarla, registrar versión, origen, licencia, necesidad, forma de instalación y distribución. La compatibilidad se comprobará con compilación y pruebas, sin inventar hashes o resultados.

## Documentación que se completará al implementar

- `WINDOWS_SETUP.md`: herramientas detectadas y pasos reales de configuración.
- `BUILD_AND_RELEASE.md`: comandos comprobados, staging, despliegue de DLL/plugins y verificación del paquete.
- `LICENSE_COMPLIANCE.md` y `THIRD_PARTY_NOTICES.md`: inventario y condiciones de las dependencias realmente elegidas.
- Manifiestos, presets y configuración de CI alineados con esas versiones.

No se crean configuraciones de Python o IA hasta que su fase requiera dependencias concretas. El instalador completo y el actualizador permanecen en fase 1.0.
