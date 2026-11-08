# Registro de cambios

## 2026-11-07 — Preparación de CI Windows y subida autorizada

- Añadido workflow Windows x64 con acciones oficiales fijadas por commit, bootstrap Qt 6.8.3 con SHA-256, MSVC 14.44, empaquetado y verificaciones inline sin archivos de pruebas.
- Añadido bootstrap-windows.ps1 para instalación acotada del kit dentro de build/, sin instalar compiladores ni paquetes Python.
- Añadida conservación de ZIP, checksum, manifiesto y evidencias durante siete días; permisos mínimos y sin publicación de release.
- Añadido .gitattributes para mantener exactos los bytes de licencias con hashes; normalizados contratos de finales de línea. El paquete registra si procede de Actions o validación local.
- Documentados entorno, versiones y límites en CI.md. El usuario autorizó subir también los incrementos acumulados del visor y empaquetado.

Validación local: sintaxis PowerShell/Python/YAML y bootstrap contra Qt existente correctos; cinco casos de importación y verificación de ZIP/manifiesto inline satisfactorios. Ejecución remota pendiente de observar tras el push; no se afirma éxito CI todavía.

## 2026-11-07 — Paquete portátil Windows x64

- Añadido scripts/package-windows.ps1: build Release, staging nuevo, despliegue Qt/OpenGL y plugins, runtime MSVC app-local desde VC/Redist, comprobación PE AMD64, qt.conf relativo, manifiesto de archivos, ZIP y SHA-256. No elimina salidas anteriores ni instala herramientas.
- Ajustado destino CMake install a la raíz portátil. Versionados 37 textos de licencia de Qt Base v6.8.3 con procedencia y hashes, conservando SBOM y avisos dentro del paquete.
- Corregido tratamiento del stderr nativo en PowerShell 5.1 para registrar avisos de windeployqt sin confundirlos con un fallo de despliegue; su exit code sigue siendo obligatorio.
- Añadida BUILD_AND_RELEASE.md con comandos, arquitectura del paquete, validación y límites; actualizado inventario de terceros.

Validación local: ZIP/CRC y hashes de inventario comprobados, extracción con espacios y ñ, PATH mínimo sin kit Qt y variables Qt retiradas. Inspección binaria: 12 triángulos y dimensiones 10×20×30, salida 0; vacío: salida 1. GUI cargó Qt, qwindows y runtime MSVC desde la carpeta extraída y registró envío de buffer OpenGL. Informe vigente y checksum en build/portable-validation.json. Sin archivos de pruebas propios ni publicación/push de binarios.

Limitaciones: equipo con herramientas instaladas, sin Windows limpio ni segundo PC; firma, instalador, actualización, CI y distribución pública pendientes. El proceso GUI de comprobación se terminó por el coordinador; no se declara cierre normal ni validación interactiva del paquete.

## 2026-11-07 — Revisión de cámara en ventana visible

- Verificados directamente arrastre izquierdo (órbita), rueda (zoom), F (encuadre sin cambiar orientación) y Home (recuperación de cámara inicial), con capturas posteriores de la ventana visible.
- Las estadísticas mantuvieron 12 triángulos y dimensiones 10×20×30. No se detectó fallo en esas acciones y no se modificó código.
- El usuario confirmó haber realizado el arrastre derecho para paneo; las capturas posteriores no aislaron el visor, por lo que no se declara corroboración visual independiente de ese desplazamiento. Se conserva la evidencia previa por eventos dirigidos/framebuffer.
- Actualizada PERFORMANCE.md distinguiendo observación directa, comprobación asistida y pendientes. Sin archivos de pruebas ni nuevo commit/push.

## 2026-11-07 — Validación de cargas y cancelación

- Añadidas métricas QElapsedTimer para recepción/análisis, preparación de vértices y envío de buffer GPU. Sin dependencias nuevas, API keys ni cambios en los contratos de malla.
- Medidas cuatro superficies generadas en memoria: 10k, 100k, 1M y 2M triángulos. Las cuatro completaron la carga; la mayor registró 573.42 ms de recepción/análisis, 124.80 ms de preparación, 199.97 ms de envío gráfico y pico residente de 690.94 MiB.
- Observada malla de 10k en ventana visible y cancelación durante recepción de una carga de 2M; cierre posterior normal 0. La revisión manual de cámara no se dio por completada por cambios de foco durante interacción física.
- Documentados método, hardware, límites, limpieza intencional de procesos y evidencias en PERFORMANCE.md; actualizados enlaces y pendientes.

Validación: Release x64 compilado sin warnings C++ detectados. Datos de benchmark e informes bajo build/, sin archivos STL ni fuentes de pruebas. La terminación 1 de las cuatro cargas fue limpieza intencional del coordinador tras dos segundos, no un crash; el caso de cancelación cerró con 0. Envío gráfico no mide finalización GPU ni FPS; RAM no equivale a VRAM. Sesiones previas interrumpidas/ocultas descartadas.

Pendiente: sesión manual de controles, diálogo Unicode, retención de malla previa, cancelación dentro de parser/preparación, VRAM real, otros equipos, CI y distribución. Sin nuevo commit/push durante esta validación.

## 2026-11-07 — Visor 3D OpenGL

- Integrado QOpenGLWidget con Qt OpenGL/OpenGLWidgets 6.8.3 del kit existente, sin nuevas descargas ni cambios en el núcleo geométrico.
- Añadidos adaptador de malla en segundo plano, buffers GPU, iluminación plana y renderizado a dos caras. Centrado/normalización solo para presentación; estadísticas y coordenadas originales preservadas.
- Añadidos órbita, zoom, paneo, encuadre F, restablecimiento Home, cuadrícula XY, ejes y orientación Z arriba. Barra y menú Vista con guías y copia del framebuffer al portapapeles.
- Añadida entrada GUI --view-stdin por tubería, con cancelación incluso mientras espera datos del productor. Inspección CLI existente conservada.
- Documentados uso, dependencias, contratos, memoria y resultados en VIEWER.md; actualizados documentos de estado y licencias.

Validación: build Release x64 satisfactorio sin warnings C++ detectados; OpenGL 3.3.0 NVIDIA 610.88 / GeForce RTX 3060 Ti. Malla binaria de 12 triángulos generada en memoria importada en GUI, estadísticas 10×20×30 intactas, captura 911×720 inspeccionada. Verificados por capturas e interacción dirigida: cuadrícula/ejes, órbita, zoom, paneo, encuadre y restablecimiento a imagen inicial. Cancelar/Cerrar con tubería esperando datos: salida 0. Regresión CLI de cubo/vacío/truncado correcta. Evidencias en build/ referenciadas en VIEWER.md, sin archivos STL ni fuentes de pruebas.

Limitaciones: pendiente interacción manual visible, diálogo con archivo real Unicode, retención de malla previa ante nueva importación fallida, cancelación durante preparación extensa, rendimiento de mallas grandes, otros controladores, CI, segundo equipo y distribución autónoma. La fase 0.1 sigue abierta. Los cambios de este incremento quedan locales, sin nuevo commit/push.

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
