# Arquitectura y decisiones

Fecha: 2026-11-07. Estado: importador y visor OpenGL compilados Release x64; renderizado y controles verificados mediante capturas y eventos dirigidos. Interacción manual visible pendiente. Decisiones: Qt Widgets 6.8.3 exacto (sustitución autorizada de 6.8.9), enlace dinámico, MSVC x64, CMake y Ninja. El inventario y las limitaciones están en WINDOWS_SETUP.md. Importador, estadísticas y visor implementados. Adaptador de presentación en MeshView.h/.cpp; preparación de vértices en QThread, carga GPU en contexto UI. Contratos y validación del visor en VIEWER.md. Núcleo stlcut_core independiente de Qt, sin dependencias geométricas ni vcpkg adicionales. Contratos y evidencia en STL_IMPORT.md. La tabla siguiente conserva temas futuros; interfaz y enlace quedan resueltos para esta base.

## Separación de responsabilidades

- `apps/desktop`: ventana, diálogos, comandos y presentación de errores.
- `core/io`: lectura STL y errores estructurados, sin dependencia de la interfaz.
- `core/mesh`: datos de malla, estadísticas y caja envolvente.
- `core/validation`: comprobaciones de entrada y límites de recursos.
- Adaptador del visor: convierte la malla al formato de renderizado sin trasladar reglas geométricas a la UI.
- `tests`: fixtures propios, pruebas unitarias y de integración.
- `scripts` y CI: configuración, compilación, pruebas y paquete reproducible.

Flujo previsto: seleccionar archivo → validar límites → importar → validar datos → calcular estadísticas → presentar malla y resultados. Un fallo debe producir un error comprensible y mantener la aplicación utilizable. Se conserva el modelo anterior ante fallo o cancelación; solo se sustituye tras finalizar correctamente.

La lectura STL y preparación de la copia de presentación usan QThread y stop_token. El trabajo comparte un resultado que la UI consume tras esperar al hilo; al cerrar se solicita cancelación y se espera antes de destruirlo. La UI mantiene la malla anterior mientras importa. Véase STL_IMPORT.md para límites de cancelación y validación pendiente. La geometría no dependerá de servicios IA.

## Decisiones pendientes de aprobación

| Tema | Alternativas o cuestión | Estado |
|---|---|---|
| Interfaz | Qt Widgets | Seleccionada para esta base |
| Renderizado | QOpenGLWidget + Qt OpenGL/OpenGLWidgets 6.8.3 | Alternativa aprobada para visor 0.1; módulos del kit instalado |
| Dependencias | Qt Base 6.8.3 MSVC x64 oficial | Instalado y checksum verificado; vcpkg/transitivas futuras pendientes |
| Toolchain | MSVC, SDK, CMake y Ninja | Release local correcto; segundo equipo y CI pendientes |
| Enlace | Dinámico | Exigido por CMake; distribución pendiente |
| Licencia del producto | Uso personal, código abierto o comercial | Por confirmar |
| Importación | Parser C++20 propio; Mesh double; 256 MiB y 2M triángulos | Aprobado e implementado; contrato en STL_IMPORT.md |
| Geometría | CGAL u otra solución para fases posteriores | Fuera de fase 0.1 |
| CI | GitHub Actions windows-2022, MSVC 14.44 y Qt verificado | Configurado; primera ejecución pendiente de observar, véase CI.md |
| Paquete inicial | ZIP Release x64 con Qt/plugins y CRT local | Generado y verificado localmente; Windows limpio/segundo PC pendientes |

Ninguna alternativa de esta tabla es una dependencia aprobada. Antes de incorporarla, registrar versión, origen, licencia, necesidad, forma de instalación y distribución. La compatibilidad se comprobará con compilación y pruebas, sin inventar hashes o resultados.

## Documentación que se completará al implementar

- `WINDOWS_SETUP.md`: herramientas detectadas y pasos reales de configuración.
- `BUILD_AND_RELEASE.md`: comandos comprobados, staging, despliegue de DLL/plugins y verificación del paquete.
- `LICENSE_COMPLIANCE.md` y `THIRD_PARTY_NOTICES.md`: inventario y condiciones de las dependencias realmente elegidas.
- Manifiestos, presets y configuración de CI alineados con esas versiones.

No se crean configuraciones de Python o IA hasta que su fase requiera dependencias concretas. El instalador completo y el actualizador permanecen en fase 1.0.
