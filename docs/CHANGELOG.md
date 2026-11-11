# Registro de cambios

## 2026-11-10 — Publicación y aceptación CPU 0.3.6

0.3.5/0.3.6 publicados en 8798204 y corrección de generador inline a31f398 (restringir atributo x para no modificar pindex). CI 10 aprobó build/paquete y **32+92 casos**. CI 9 había aprobado 32+90, con dos rechazos de datos sintéticos por ese índice decimal. Código del visor sin cambios entre ambas ejecuciones. Aceptación CPU y de parámetros completada, con nitidez/MSAA/shaders todavía pendientes de revisión gráfica y bloqueo local intacto. Artefacto y hashes en CI.md; sin modelos privados ni instalador final publicados.

## 2026-11-09 — Corrección visual 0.3.6 en aceptación

Incidencia de moteado 3MF confirmada en ventana 0.3.5 y corrección autorizada. Normales double por cara fuente, heredadas por subdivisiones de pintura; evita normales cero debidas al umbral absoluto de QVector3D::normalized. MSAA 4x solicitado con soporte real informado, rango near/far adaptativo y retirada de polygon offset global. Geometría/fuentes intactas. Release compilado; primer intento CPU bloqueado por Windows 4551, sin eludirlo. Análisis independiente reproduce la causa; no equivale a aceptación C++/GPU. Siete casos adicionales preparados (32+92 totales), sin ejecución final ni publicación/CI nueva. VIEWER.md.

Validación del paquete local 0.3.5: **32+85 comprobaciones correctas**, con PATH mínimo y carpeta con espacios/ñ; inventario/CRC/hashes verificados. Sin revisión visual ni publicación/CI remota nueva. Evidencia y checksum en HISTORY.md.

## 2026-11-09 — Historial de reconstrucción 0.3.5 local

EditState conserva modo, resolución y profundidad de inserto; GUI restaura controles con señales bloqueadas y habilita profundidad según modo. CLI paint-settings edita borrador sin operación geométrica y expone paint_volume en estados. split-paint publica sus parámetros tras commit satisfactorio; fallos preservan borrador y Rehacer. El presupuesto incluye el estado ampliado por sizeof. Release y 85/85 aceptaciones inline correctas (13 nuevas). Sin nuevas dependencias, fuentes/carpetas de pruebas, revisión visual ni publicación/CI nueva. Contrato en HISTORY.md.

## 2026-11-09 — Publicación y aceptación CI 0.3.4

Tras autorización, 0.3.3/0.3.4 publicados en main dbacc4a. CI 8 completó build/paquete, **32/32 comprobaciones generales y 72/72 de pintura/volumen/conectores** y artefacto windows-x64-8. Resuelta la aceptación remota pendiente; el bloqueo local permanece documentado sin modificar seguridad. Sin revisión visual ni cierre de fase. Evidencia y hashes en CI.md.

## 2026-11-09 — Conectores entre regiones 0.3.4 en aceptación

Conectores opcionales en partición/insertos; búsqueda sobre facetas, cilindros de guarda, nominales y delta de volumen, separación, colisiones y recorrido recto conservador. Omisiones explícitas por pareja, informes JSON asociados a IDs de sesión y GUI con resumen; publicación por historial transaccional. Release/ZIP completados; dos casos preliminares ejecutados. La batería final de 72 casos quedó bloqueada antes de arrancar por Windows 4551; no se declara aprobada. Paquete solo estático: CRC/inventario/hashes/licencia y exe idéntico al bloqueado. Workflow preparado con 18 casos nuevos y detención al primer 4551. Sin revisión visual, dependencias nuevas, fuentes/carpetas de pruebas ni cambios de seguridad. Sin commit/push ni CI remota nueva. Contrato en REGION_CONNECTORS.md.

## 2026-11-09 — Volúmenes desde pintura 0.3.3 local

Dos modos seleccionables: partición completa por color exterior más cercano e insertos de profundidad configurable con núcleo sin color asignado. Resolución por celdas explícita; exterior recortado contra el sólido fuente, interfaces internas escalonadas. Componentes/cavidades, validación previa de cierre/volumen/exportación float32, transacción de historial y cancelación. Controles GUI y CLI `split-paint`. Release y 54/54 comprobaciones inline correctas; ZIP con inventario/hashes y **32+54** comprobaciones con PATH mínimo correctas. Workflow ampliado con 18 casos; sin fuentes de pruebas ni dependencias nuevas. Conectores de estas interfaces y reparación pendientes. Sin revisión visual, commit/push ni nueva CI; contrato en PAINT_VOLUME.md.

## 2026-11-08 — Publicación 0.3.2 y CI satisfactoria

Publicados en main los incrementos acumulados como d2a219a, con autorización del usuario; modelos privados, prueba.stl y build/ excluidos. Añadida la batería inline completa de 36 casos de pintura al workflow, además de 32 comprobaciones generales. Ejecución CI 7 satisfactoria en todas las etapas y artefacto windows-x64-7 disponible. Evidencia, entorno y hashes en CI.md. El bloqueo local de Windows permanece y no se cambió su protección. Sin revisión visual ni instalador final; insertos/partición volumétrica pendientes.

## 2026-11-08 — Lectura de pintura 0.3.2 en validación

Incorporada miniz 3.1.0 MIT tras autorización explícita; lector ZIP acotado, codec de pintura, asociación XML/JSON de filamentos y regiones baricéntricas separadas de la malla original. Nuevos diagnósticos CLI/CPU sin ventana. Release y paquete completados; solo inventario/CRC/hashes/licencias verificados en la versión final. Evidencia preliminar de pintura real, pero Windows bloqueó ejecución final con 4551 (Code Integrity); no se eludió ni se declara aceptación de las 36 pruebas o 32 del workflow. Sin revisión visual, push ni CI nueva. Insertos/partición volumétrica pendientes; PAINT_IMPORT.md.

## 2026-11-08 — Materiales y regiones estándar 0.3.1

Conservados materiales RGBA, propiedades por triángulo, objetos e instancias 3MF; separación exacta de regiones sólidas por material/conectividad con historial transaccional y RGB en presentación. Conectores automáticos para dos regiones con contacto plano en ejes, rechazando geometría incompatible. Corregido seguimiento de cabeceras Ninja/MSVC localizado y reconstruida ABI completa. 20 casos de color, 63 geométricos, 22 de historial, 16 de conectores y 28 del paquete correctos; seis modelos privados intactos. Sin revisión visual, push ni CI remota nueva. Pintura propietaria y ambos modos volumétricos siguen pendientes; alcance en COLOR_IMPORT.md.

## 2026-11-08 — Primer incremento de conectores 0.3.0

Pin en lado positivo y cavidad ciega en negativo al cortar; diámetro, profundidad, holgura diametral y pared configurables. Búsqueda acotada sobre el plano con guardas booleanas, volumen nominal y exportación validada. Historial conserva geometría y parámetros. 16 casos de conectores, 22 de historial y 25 del paquete correctos localmente; sin revisión visual, push ni CI nueva. Usuario eligió admitir cuerpos por color y pintura superficial, ofreciendo insertos y partición volumétrica; separación por color aún pendiente. Contrato en PHASE_03.md.

## 2026-11-08 — Alcance aprobado de fase 0.3

Conectores pin/cavidad y objetivo añadido de separar automáticamente 3MF por color con conectores. Inspeccionado importador/SDK y tres archivos privados en lectura: detectada pintura paint_color en uno. Documentado diseño y distinción entre cuerpos por color y pintura superficial en PHASE_03.md; política volumétrica pendiente. Sin funciones nuevas implementadas ni pruebas de fase 0.3 ejecutadas. Revisión visual omitida por instrucción del usuario.

## 2026-11-08 — Exportación compartida validada 0.2.3

Ajuste aprobado: extraída validación STL a StlWriter::validatedStl y usada por interfaz, edición JSON y corte CLI. Rechaza conversiones float32 que pierden precisión/cierre/volumen antes de guardar o devolver la pieza. Historial y Rehacer conservados; sin reparación ni dependencias nuevas. Release, 22 casos de historial, 63 de geometría (criterio trasladado actualizado), trece exportaciones reales válidas, dos rechazos reales y 23 casos del paquete comprobados. Detalles en EXPORT_VALIDATION.md. Cambios locales, sin commit/push ni CI remota nueva; instalador final pendiente.

## 2026-11-08 — Validación real de fase 0.2

Seis modelos privados importados sin modificar fuentes; cinco secuencias de cortes/undo/redo correctas y un rechazo esperado por degeneración. Quince exportaciones reimportadas: trece cerradas/orientadas y dos con degeneraciones tras conversión float32, emitidas por CLI sin su validación posterior. Registrados tiempos/memoria y pendiente de unificar validación CLI/interfaz en REAL_MODEL_VALIDATION.md. Sin cambios de código, dependencias, fuentes de pruebas, commit/push ni nueva CI. Fase 0.2 abierta.

## 2026-11-08 — Historial y cortes encadenados 0.2.2

- Cortar cualquier pieza activa conservando hermanas, IDs y procedencia; original visible/exportable como referencia.
- Deshacer/Rehacer recuperan piezas, selección y plano. Geometría compartida; máximo 32 pasos, 256 piezas y 1 GiB contabilizado. Fallo/cancelación no altera el historial; corte nuevo elimina Rehacer. Sesión en memoria, sin guardar proyectos.
- CLI de producción --edit-stdin, seis aceptaciones adicionales del workflow y documentación HISTORY.md. Sin nuevas dependencias ni archivos de pruebas propios.

Validación local: Release, 22 casos de historial, 63 de regresión y 21 del paquete satisfactorios; interfaz visible revisada. Alcance de cancelación y evidencia en HISTORY.md. Publicado en main como 34b0cb7 por autorización del usuario; ejecución CI 6 satisfactoria con build, paquete, 21 casos y artefacto windows-x64-6. Evidencia en CI.md; instalador final pendiente de fase 1.0.

## 2026-11-08 — Publicación 0.2.1 y CI Windows satisfactoria

- Subido main en b1ede31a1a54700b2bd2495f5119ca600895a796, tras autorización del usuario. Fuera de Git: build/, binarios, SDK y prueba.stl.
- Ejecución 5 completada success: bootstrap de las tres dependencias, build/paquete, quince comprobaciones inline y artefacto windows-x64-5. Inventario remoto de 74 archivos más manifiesto. Entorno, enlaces y checksums en CI.md.
- Actualizados estados vigentes para reflejar publicación y verificación remota. Se mantienen los límites del motor, las limitaciones históricas de pruebas y la entrega del instalador final en fase 1.0. Sin release público ni fuentes/fixtures de pruebas propios.

## 2026-11-08 — Corte Manifold 0.2.1 autorizado

- Sustituido el motor propio por Manifold 3.5.4 estático, fuente/commit/SHA-256 fijados, licencia Apache-2.0 y autores incluidos en el paquete. Sin CGAL, TBB, Clipper2, Python ni bindings de aplicación adicionales.
- Admitidos tubos, cavidades múltiples/encerradas, contornos anidados y coincidencias con aristas/caras. Retirado tope separado de 4096 vértices; se conservan límites de 2M triángulos/vértices y presupuesto de buffers contabilizados. Adaptación double centrada/escalada, consolidación de resultados coplanares y comprobación de cierre/volumen.
- Cancelación cooperativa preserva estado anterior; operaciones internas inmediatas pueden demorar su atención. No se promete latencia máxima ni reparación universal.

Validación: build Release x64, 63/63 casos inline, quince casos del paquete con PATH mínimo, ambos lados visibles de un tubo (24 → 12 + 12) y retención ante plano tangente. Cancelación por Ctrl+. sobre 1 929 612 triángulos comprobada. Medición CLI puntual: 13.3604 s, pico RSS 1350.70 MiB. Nueva evidencia y límites en MANIFOLD_CUT.md. Se observó una advertencia C4244 en header externo, sin modificar upstream. No se crearon fuentes/fixtures de pruebas ni se alteró prueba.stl. Sin commit/push, nueva CI remota o instalador final.

## 2026-11-08 — Primer incremento autorizado de fase 0.2

- Versión de desarrollo 0.2.0. Añadida importación STL/3MF por ruta y stdin con lib3mf 2.5.0 oficial, descarga/hash fijados, límites ZIP y expansión de componentes/unidades/transformaciones. Advierte pérdida de colores/materiales/configuración; rechaza extensiones obligatorias y geometría no admitida.
- Plano por normal y posición con previsualización amarilla; corte en ambos lados, cierre de contornos simples y comprobación de orientación/volumen. Conserva original y resultado anterior ante error/cancelación. No admite huecos anidados ni caras/aristas coplanares en este primer motor.
- Exportación STL binaria de selección, reimportación y validación de precisión/cierre/volumen antes de guardado atómico; protege origen.
- CMake/paquete incorporan DLL y licencia completa del SDK; workflow ampliado a diez casos inline y documentación actualizada. Sin instalación de CGAL/Python/IA ni archivos de pruebas propios.

Validación: Release x64 local correcto, 48/48 casos inline satisfactorios; un archivo 3MF real de 20 124 triángulos leído sin modificarlo; revisión visible del plano, ambos lados de un cubo 3MF (1000 → 500 + 500 mm³) y conservación de pieza tras corte inválido. Exportación asistida a prueba.stl, reimportación CLI correcta, 20 triángulos y volumen 500. Cortes completos sobre 756 012 y 1 929 612 triángulos; cancelación por botón y Ctrl+. conservando original. Corregido el atajo perdido al cambiar el texto del botón, mediante QShortcut de ventana. ZIP provisional final con 72 archivos más manifiesto y diez comprobaciones satisfactorias localmente con PATH mínimo; no es nueva ejecución CI remota. Evidencia y límites en PHASE_02.md. La aceptación de fase 0.1 por el usuario permite avanzar, sin inventar ejecución de pendientes históricos. Sin commit/push ni instalador final.

## 2026-11-07 — Retiro del segundo PC y entrega final .exe

- Por instrucción del usuario, eliminado el segundo PC físico de requisitos, planificación y criterios de cierre. Desarrollo en un equipo Windows y validación de compilación en CI.
- Fijada entrega final de fase 1.0: instalador `STLCutStudioAI-Setup-<version>-win-x64.exe` que despliegue aplicación y dependencias necesarias, con apertura sin preparación manual de Qt, Python ni herramientas de desarrollo.
- La VM Windows limpia será obligatoria para validar instalación, ejecución de funciones entregadas y desinstalación final. Su uso en fase 0.1 es opcional; el ZIP portátil actual sigue siendo provisional.
- Alineados maestro 1.2, instrucciones AGENTS, README, requisitos, arquitectura, plan de validación y documentación de CI/paquete/importador/visor/rendimiento. Las menciones anteriores a segundo PC en este registro son históricas y no requisitos vigentes.

Validación: revisión de referencias y coherencia documental; sin cambios de código, scripts, workflow, ZIP ni manifiestos existentes y sin nuevas ejecuciones de compilación o pruebas. No se implementó el instalador ni se declara cerrada la fase 0.1. Los mensajes históricos dentro de paquetes anteriores se alinearán cuando se autorice modificar el empaquetador. Esta actualización queda local, sin commit/push.

## 2026-11-07 — CI remota verificada y artefacto disponible

- Subidos los incrementos acumulados de visor, métricas, empaquetado, licencias y workflow a main.
- Corregida extracción Qt con 7-Zip del runner y actualizadas acciones oficiales a checkout 7.0.1/upload-artifact 7.0.2, fijadas por commit y usando Node 24.
- Añadidas anotaciones públicas de diagnóstico e inventario, sin credenciales. El acceso protegido a logs no fue necesario para resolver el fallo.
- Ejecución 4, commit 20c6c3b, completada success en todas sus etapas. MSVC 14.44.35207, SDK 10.0.26100.0, Qt 6.8.3, CMake 3.31.6, Ninja 1.13.2, Python 3.12.10 y 7-Zip 26.05.
- Verificados ZIP/inventario y cinco casos inline ASCII/binario/errores con PATH mínimo. Artefacto windows-x64-4 disponible con retención de siete días; enlaces, hashes y límites en CI.md.

Validación remota: https://github.com/GerDeveelop/STL-model-3D/actions/runs/42446122445. Sin archivos de pruebas propios ni release publicado. Siguen pendientes segundo PC del usuario, Windows limpio y validaciones ampliadas de interfaz/GPU; no se declara cierre completo de fase 0.1.

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
