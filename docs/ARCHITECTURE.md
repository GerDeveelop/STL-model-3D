# Arquitectura y decisiones

Fecha: 2026-11-06. Estado: base C++20/Qt Widgets compilada Release x64; arranque oculto y cierre normal comprobados, sin revisión visual. Decisiones: Qt Widgets 6.8.3 exacto (sustitución autorizada de 6.8.9), enlace dinámico, MSVC x64, CMake y Ninja. El inventario y las limitaciones están en WINDOWS_SETUP.md. Importador STL y estadísticas implementados y compilados; visor pendiente. Núcleo stlcut_core independiente de Qt, sin dependencias geométricas ni vcpkg adicionales. Contratos y evidencia en STL_IMPORT.md. La tabla siguiente conserva temas futuros; interfaz y enlace quedan resueltos para esta base.

## Separación de responsabilidades

- `apps/desktop`: ventana, diálogos, comandos y presentación de errores.
- `core/io`: lectura STL y errores estructurados, sin dependencia de la interfaz.
- `core/mesh`: datos de malla, estadísticas y caja envolvente.
- `core/validation`: comprobaciones de entrada y límites de recursos.
- Adaptador del visor: convierte la malla al formato de renderizado sin trasladar reglas geométricas a la UI.
- `tests`: fixtures propios, pruebas unitarias y de integración.
- `scripts` y CI: configuración, compilación, pruebas y paquete reproducible.

Flujo previsto: seleccionar archivo → validar límites → importar → validar datos → calcular estadísticas → presentar malla y resultados. Un fallo debe producir un error comprensible y mantener la aplicación utilizable. Se conserva el modelo anterior ante fallo o cancelación; solo se sustituye tras finalizar correctamente.

La lectura STL usa QThread y stop_token. El trabajo comparte un resultado que la UI consume tras esperar al hilo; al cerrar se solicita cancelación y se espera antes de destruirlo. La UI mantiene la malla anterior mientras importa. Véase STL_IMPORT.md para límites de cancelación y validación pendiente. La geometría no dependerá de servicios IA.

## Decisiones pendientes de aprobación

| Tema | Alternativas o cuestión | Estado |
|---|---|---|
| Interfaz | Qt Widgets | Seleccionada para esta base |
| Renderizado | VTK integrado con Qt o alternativa justificada | Por decidir |
| Dependencias | Qt Base 6.8.3 MSVC x64 oficial | Instalado y checksum verificado; vcpkg/transitivas futuras pendientes |
| Toolchain | MSVC, SDK, CMake y Ninja | Release local correcto; segundo equipo y CI pendientes |
| Enlace | Dinámico | Exigido por CMake; distribución pendiente |
| Licencia del producto | Uso personal, código abierto o comercial | Por confirmar |
| Importación | Parser C++20 propio; Mesh double; 256 MiB y 2M triángulos | Aprobado e implementado; contrato en STL_IMPORT.md |
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
