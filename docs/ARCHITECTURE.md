# Arquitectura y decisiones

> Estado vigente 0.2.2: cortes encadenados sobre piezas activas y Deshacer/Rehacer de sesión, con geometría compartida y presupuesto de historial. Uso, límites y evidencia local en [HISTORY.md](HISTORY.md). Se conserva Manifold 3.5.4. Última CI remota: 0.2.1; las secciones anteriores por versión son históricas.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

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
| Toolchain | MSVC, SDK, CMake y Ninja | Release local y CI verificados; versiones y diferencias documentadas, sin segundo PC obligatorio |
| Enlace | Dinámico | Exigido por CMake; distribución pendiente |
| Licencia del producto | Uso personal, código abierto o comercial | Por confirmar |
| Importación | Parser C++20 propio; Mesh double; 256 MiB y 2M triángulos | Aprobado e implementado; contrato en STL_IMPORT.md |
| Geometría | Manifold 3.5.4 estático, sin bindings ni TBB/Clipper2 | Aprobado, compilado y validado en incremento 0.2.1 |
| CI | GitHub Actions windows-2022, MSVC 14.44 y Qt verificado | Ejecución 4 satisfactoria: build, ZIP, cinco casos y artefacto; véase CI.md |
| Paquete inicial | ZIP Release x64 con Qt/plugins y CRT local | Provisional, verificado localmente y en CI; instalador .exe/VM limpia corresponden a fase 1.0 |

Ninguna alternativa de esta tabla es una dependencia aprobada. Antes de incorporarla, registrar versión, origen, licencia, necesidad, forma de instalación y distribución. La compatibilidad se comprobará con compilación y pruebas, sin inventar hashes o resultados.

## Documentación que se completará al implementar

- `WINDOWS_SETUP.md`: herramientas detectadas y pasos reales de configuración.
- `BUILD_AND_RELEASE.md`: comandos comprobados, staging, despliegue de DLL/plugins y verificación del paquete.
- `LICENSE_COMPLIANCE.md` y `THIRD_PARTY_NOTICES.md`: inventario y condiciones de las dependencias realmente elegidas.
- Manifiestos, presets y configuración de CI alineados con esas versiones.

No se crean configuraciones de Python o IA hasta que su fase requiera dependencias concretas. El instalador completo y el actualizador permanecen en fase 1.0.

## Entrega final acordada

Se trabaja con un equipo de desarrollo y CI; el segundo PC físico deja de ser requisito. Al completar el programa se entregará un instalador `.exe` Windows x64 que despliegue la aplicación y dependencias necesarias, para abrirla sin instalación manual de Qt, Python ni herramientas de desarrollo. La validación de instalación, ejecución y desinstalación se realizará en VM Windows limpia antes del cierre de fase 1.0. El ZIP actual es provisional. El instalador no está implementado y este cambio documental no autoriza comenzar su desarrollo.

## Arquitectura del incremento 0.2

`core/io/MeshReader` detecta STL/ZIP 3MF y adapta lib3mf 2.5.0 a Mesh; `core/cutting/PlaneCut` calcula y verifica ambos lados; `core/io/StlWriter` serializa STL binario. Mesh incorpora unidades y advertencias. El núcleo sigue sin Qt. Desde 0.2.2 la UI conserva un EditHistory de piezas inmutables compartidas, el original de referencia y una caché ViewMesh por ID. El trabajador prepara ambas piezas; el historial/caché candidatos se publican solo tras verificar límites. Se guarda con QSaveFile tras reimportar bytes. El adaptador del visor guarda centro/escala para transformar correctamente el plano sin alterar la geometría. Contratos y límites en PHASE_02.md.

## Adaptador de corte 0.2.1

PlaneCut conserva su API. Indexa/verifica la malla, normaliza coordenadas, usa MeshGL64 y SplitByPlane, consolida resultados coplanares y valida volumen/cierre antes de entregarlos a la UI. El núcleo continúa independiente de Qt. ExecutionContext recibe cancelación de stop_token; las operaciones internas inmediatas se comprueban al regresar. No hay subproceso auxiliar ni cuota rígida de RAM. Detalles de buffers, versión y licencia en MANIFOLD_CUT.md.

## Historial de sesión 0.2.2

`core/projects/EditHistory` es independiente de Qt: estados con piezas activas, selección y plano. Mallas/vistas compartidas evitan duplicar geometría entre pasos. Recorta los estados más antiguos dentro de 32 cortes y 1 GiB contabilizado; máximo 256 piezas activas. Rechazar un estado que no cabe conserva también Rehacer. El original permanece fijado. No se persiste la sesión ni se incluye la cámara en el historial. Contrato completo y CLI de edición en [HISTORY.md](HISTORY.md).
