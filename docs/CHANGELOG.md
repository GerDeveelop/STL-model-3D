# Registro de cambios

## 2026-11-06 — Importación STL y estadísticas

- Implementado núcleo C++20 independiente de Qt: malla, caja envolvente e importador ASCII/binario con límites inclusivos de 256 MiB y 2 millones de triángulos.
- Añadida biblioteca estática stlcut_core en CMake, sin nuevas dependencias. Validación de sintaxis, longitud binaria, coordenadas y dimensiones finitas; binarios con encabezado solid reconocidos por longitud exacta.
- Habilitado Archivo > Abrir STL; lectura QThread y cancelación stop_token, progreso indeterminado y sustitución del modelo solo al completar. Se muestran formato, cantidad, dimensiones, caja y advertencias sin asumir unidades ni reparar geometría.
- Añadidas opciones --inspect y --inspect-stdin para estadísticas JSON sin GUI y automatización local del mismo lector.
- Documentados contratos, límites, política de degenerados/normales y resultados en STL_IMPORT.md; actualizado estado de documentos relacionados.

Validación: Release x64 compilado correctamente con scripts/build-windows.ps1; sin warnings C++ detectados. 18 casos iniciales y 10 comprobaciones posteriores satisfactorios, con entradas STL generadas en memoria, sin archivos de fixtures ni fuentes de pruebas. Cubos ASCII/binarios de 12 triángulos con caja [0,0,0] a [10,20,30]; límites inclusivos comprobados a 2M y rechazo de 2000001 triángulos y más de 256 MiB. GUI oculta: ruta inexistente produce error controlado accesible y salida normal 0. Logs e informes locales bajo build/ documentados en STL_IMPORT.md.

Limitaciones: no se comprobó apertura GUI de archivo válido, cancelación durante lectura ni retención de una malla previa; la GUI de error se verificó sin modelo cargado. Sin revisión visual, visor, CI, segundo equipo ni paquete autónomo. No hay nuevas dependencias ni API keys. La fase 0.1 sigue abierta.

## 2026-11-06 — Qt instalado y primera compilación Release

- Autorizadas instalación/configuración de Qt y sustitución de 6.8.9 por 6.8.3. La documentación de 6.8.9 no demostraba disponibilidad del kit; se comprobó ausencia de esa versión en el índice público.
- Instalado Qt Base 6.8.3 MSVC2022 x64 desde download.qt.io en build/qt/6.8.3/msvc2022_64, sin modificar variables persistentes ni añadir dependencias Python a la app.
- Archivo oficial de 39484220 bytes: SHA-1 ab0bd2551618f7128ef30c6d74fe45b6a76e2296 verificado; SHA-256 local 41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f. Descarga por Python estándar y extracción bsdtar 3.8.9; sin pip ni gestores adicionales.
- Actualizados CMakeLists.txt y mensaje del script de build a Qt 6.8.3 exacto, además de documentación de estado, requisitos, arquitectura, validación y dependencias.

Validación: Developer PowerShell Build Tools 18.8.2 x64, MSVC 19.51.36252.0, SDK 10.0.26100.0, CMake 4.3.1-msvc1 y Ninja 1.13.2. Fuera del aislamiento del agente se completó detección ABI sin reparar MSVC. Con STL_QT_ROOT apuntando al kit local, scripts/build-windows.ps1 configuró y compiló Release con código 0. Mensaje opcional de Vulkan headers ausentes; sin impedir build. Los fallos de búsqueda pthread durante configuración son sondeos de plataforma, no un fallo final de Threads.

Ejecutable generado: build/windows-x64-release/STLCutStudioAI.exe. Arranque oculto, WaitForInputIdle, identificación de ventana por PID/título y cierre WM_CLOSE comprobados; salida 0 y stderr vacío. Evidencias startup-result.json y startup-stderr.log bajo el mismo directorio. MainWindowHandle no era adecuado para detectar la ventana oculta y ExitCode nulo de PowerShell no era un código real de fallo; se completó la comprobación con enumeración de ventanas y subprocess de Python.

Limitaciones: sin revisión visual interactiva, importador STL, visor, paquete autónomo, CI, segundo equipo ni VM. No se crearon carpetas ni archivos de pruebas propios. La fase 0.1 sigue abierta. Siguiente incremento propuesto: importador STL y presentación de estadísticas, sujeto a aprobación previa y concreción de límites de entrada.

## 2026-11-05 — Primer incremento autorizado de fase 0.1

- Añadidos CMakeLists.txt, CMakePresets.json y .gitignore para una base C++20/MSVC x64 con Qt Widgets 6.8.9 dinámico.
- Preparada ventana con versión, Salir y Acerca de; Abrir STL deshabilitado. Importador y visor no implementados.
- Añadido scripts/build-windows.ps1: valida requisitos y ejecuta configuración/build sin instalar herramientas.
- Inventariados Visual Studio Build Tools 18.8.2, MSVC (directorio 14.51.36231, compilador 19.51.36252.0), CMake 4.3.1-msvc1, Ninja 1.13.2 y headers SDK 10.0.26100.0.
- Actualizados README, requisitos y arquitectura; añadidos WINDOWS_SETUP.md e inventario THIRD_PARTY_NOTICES.md con fuentes oficiales.
- Sin carpetas ni archivos de pruebas escritos por el agente, conforme a la instrucción del usuario. CMake creó archivos temporales de detección del compilador bajo build/, excluido de Git.

Validaciones ejecutadas desde la raíz del repositorio:

- CMake --list-presets: código 0; reconoce windows-x64-release.
- ConvertFrom-Json sobre CMakePresets.json: correcto.
- Parser de PowerShell sobre scripts/build-windows.ps1: sin errores.
- Ejecución del script sin STL_QT_ROOT: rechazo esperado antes de instalar o configurar; comprobación del mensaje satisfactoria.
- Developer PowerShell Build Tools x64 y cmake --preset windows-x64-release: identifica MSVC 19.51.36252.0; queda sin progreso durante la detección ABI del compilador y se detiene el intento. No se completó configuración ni build. Log parcial: build/windows-x64-release/CMakeFiles/CMakeConfigureLog.yaml.

Limitaciones: Qt no localizado en rutas inspeccionadas; no se instaló software. Sin ejecutable, comprobación visual, importación, CI, segundo equipo ni paquete verificable. La fijación completa de dependencias/toolchain y la licencia del producto siguen pendientes. Siguiente paso propuesto: autorizar configuración de Qt y diagnosticar la comprobación MSVC antes de ampliar funcionalidades.

## 2026-11-05 — Organización documental

- Renombrado `AGENTS_STL_Cut_Studio_AI.md` a `AGENTS.md` y trasladado `STL_Cut_Studio_AI_MASTER.md` a `docs/MASTER.md`.
- Conservada la especificación original; añadidas aclaraciones de alcance y estado en el maestro, versión 1.1.
- Añadidos README, requisitos, arquitectura y plan de pruebas para fase 0.1.
- Documentadas decisiones pendientes, configuración, dependencias y ausencia de API keys requeridas para esta fase local.
- Diferenciado paquete Release inicial de instalador y actualizador finales.
- Explicitada aprobación previa del usuario para cambios e implementación.

Validación: revisión de estructura, enlaces Markdown locales y referencias documentales. Sin código modificado, dependencias instaladas ni pruebas de compilación o funcionamiento ejecutadas.
