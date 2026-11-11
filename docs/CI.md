# CI Windows x64

## Publicación y aceptación CPU 0.3.5–0.3.6 — 2026-11-10 (Colombia)

Usuario autorizó subir ambos incrementos y validar código/regresiones. Publicación inicial 8798204b32a3cec4f669775eb98e19f407d968d7; corrección del generador inline en a31f3986cead51c57f7a42d3c23f77a496a43fd4. [CI 10](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42948952405) completada **success** en todas las etapas: dependencias verificadas, Release/paquete, **32/32** casos generales y **92/92** de pintura/volumen/conectores/historial/normales. Incluye los 13 casos de parámetros y siete de normales del visor; cubre facetas pequeñas, escalas, subdivisión, reflexión y degeneración sin reparación. El paquete se ejecutó con PATH mínimo y carpeta con espacios/ñ; inventario de 75 archivos más manifiesto, CRC, tamaños y hashes correctos. **La batería no abre una ventana: no acredita resultado gráfico, MSAA, shaders ni percepción de nitidez.** La revisión visible corregida y el bloqueo local siguen pendientes, sin cambios de seguridad.

[CI 9](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42948742122) había compilado y aprobado 32 casos generales y 90/92 del segundo bloque. Dos datos sintéticos fallaron porque la expresión que desplazaba x también coincidía con el sufijo de pindex, convirtiendo su entero en decimal. a31f398 añade un límite de palabra para restringirla al atributo x; no cambia el C++ del visor. CI 10 valida la batería corregida completa. Log de CI 9 consultado con la autenticación existente autorizada, solo en memoria; credenciales no mostradas/guardadas. La evidencia del log queda en build/ci-036-run9-job.log, fuera de Git.

Entorno remoto según anotación pública: windows-2022 imagen 20261101.353.1, MSVC 14.44.35207, SDK 10.0.26100.0, CMake 3.31.6, Ninja 1.13.2, Python 3.12.10, 7-Zip 26.05, CRT 14.44.35211.0. Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4 y miniz 3.1.0 fijados. No se promete igualdad binaria con el toolchain local.

[Artefacto windows-x64-10](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42948952405/artifacts/13823534325): 16 284 137 bytes, no expirado al consultar, retención de siete días. Digest del contenedor Actions `sha256:89b9a974aec3c20b5b2187a0ef4bdeff9d31041b1ff3c8306d808c5d260df219`. ZIP interior `STLCutStudioAI-0.3.6-portable-win-x64-20261111T025346995Z.zip`, SHA-256 `a73a8805f06fe995b6f2724f28fd3b279c12cb5ca1e578a38131e6ffe7b69e89`. Nombre en UTC; ejecución de fecha local 2026-11-10. No confundir hashes con el ZIP local ni con el contenedor. No se descargó/ejecutó el artefacto localmente. Los informes ci-verification.json y ci-paint-verification.json se incluyen en el artefacto.

Metadatos consultados por API pública y registrados fuera de Git en build/ci-036-run.json, ci-036-jobs.json, ci-036-artifacts.json y ci-036-annotations.json. Modelos privados, build/ y prueba.stl excluidos de la subida. Sin release público, instalador ni comprobación de VM limpia; fase 0.3 sigue abierta. Las notas de falta de publicación/aceptación remota siguientes son históricas.

> Corrección local 0.3.6: siete casos CPU de normales añadidos a los 85 de pintura/volumen/conectores/historial: previstos 32+92. Sintaxis Python comprobada; no se ejecutaron contra el exe nuevo por bloqueo 4551. Sin publicación ni CI remota nueva. MSAA/shaders/rango de profundidad requieren comprobación gráfica adicional. [VIEWER.md](VIEWER.md).

> Incremento local 0.3.5: workflow preparado con 32 casos generales y 85 de pintura/volumen/conectores/historial (13 nuevos). Ambos bloques pasaron localmente sobre el paquete con PATH mínimo, inventario/CRC/hashes correctos. Sin publicación ni nueva ejecución remota; CI 8 siguiente corresponde a 0.3.4. Evidencia en [HISTORY.md](HISTORY.md).

> Estado 0.3.2 publicado y validado en CI Windows: ejecución 7, commit d2a219a, build/paquete y 32 casos generales + 36 de pintura correctos. El bloqueo local 4551 no se ha cambiado; sin revisión visual. Insertos/partición volumétrica pendientes. Evidencia en [CI.md](CI.md); los resultados anteriores conservan su contexto histórico.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha: 2026-11-07. Workflow subido y ejecución final 4 satisfactoria en el commit 20c6c3b; correcciones y evidencia abajo. El usuario autorizó subir los cambios acumulados y ejecutar Actions. No se publica un release ni se instalan servicios/secretos adicionales.

## Flujo

`.github/workflows/windows-ci.yml` se ejecuta en `windows-2022`, en pushes relevantes a main, pull requests a main y ejecución manual. Los pushes que solo cambian documentación no disparan otra compilación; cambios en código, scripts, CMake, licencias, avisos, atributos Git o workflow sí lo hacen.

1. Checkout sin persistir credenciales, permisos contents:read.
2. `bootstrap-windows.ps1`: descarga Qt Base 6.8.3 MSVC2022 x64 desde download.qt.io, exige 39484220 bytes y SHA-256 `41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f`, extrae con 7-Zip del runner (tar como alternativa local) y comprueba qmake/archivos requeridos. Reutiliza un archivo previamente verificado y rechaza instalaciones parciales; no elimina archivos para reintentar. Conserva descargas parciales y log de extracción para diagnóstico.
3. Selección de Developer PowerShell x64 y MSVC serie 14.44 mediante vswhere/DevShell, sin acciones externas de instalación. Se utiliza el CRT redistribuible x64 de ese entorno.
4. Ejecución del mismo `package-windows.ps1` de uso local: Release, staging, DLL/plugins/runtime, manifiesto, ZIP y SHA-256.
5. Verificación del ZIP, CRC, rutas de extracción, inventario exacto y hashes; extracción a ruta con espacio y ñ. El ejecutable extraído recibe por stdin cubos ASCII/binarios de 12 triángulos, binario con solid, vacío, truncado y coordenadas no finitas. Se exigen estadísticas/códigos de salida correctos, con PATH limitado a Windows y System32 y variables Qt retiradas.
6. Conservación de ZIP, checksum, manifiesto, logs, procedencia de Qt e informes como artefacto `windows-x64-<run_number>` durante siete días, también con logs disponibles cuando una etapa falla. No sube el árbol del kit Qt como artefacto separado.

Las entradas de comprobación se generan en memoria dentro del workflow; no existen carpetas ni archivos de pruebas propios. No se ejecuta CTest porque no se creó esa infraestructura por instrucción del usuario. Esta verificación CLI no demuestra renderizado OpenGL, diálogo de archivos o interacción en un runner sin GPU de usuario.

## Versiones, permisos y límites

Acciones oficiales fijadas por commit y licencia MIT:

- checkout v7.0.1: `3d3c42e5aac5ba805825da76410c181273ba90b1`.
- upload-artifact v7.0.2: `cf430e030ddbb5b0abf93d22962f4752f3646cd9`.

Los commits y licencias se verificaron contra los repositorios oficiales al preparar este incremento. Qt está fijado por versión y hash; MSVC por serie. La imagen windows-2022, los parches de MSVC/SDK y herramientas preinstaladas pueden cambiar. `ci-environment.json` conserva commit, versión de imagen, toolset, SDK, CMake, Ninja, Python y CRT realmente usados: no se afirma reproducibilidad binaria con una imagen inmutable ni igualdad con el MSVC 2026 del equipo local.

`STL_QT_ROOT` es la ruta del kit. `GITHUB_ENV`, `GITHUB_SHA`, `GITHUB_ACTIONS` y demás variables de runner las proporciona Actions. Solo se usa GITHUB_TOKEN automático de lectura para checkout y los mecanismos propios de artefactos; no se requieren API keys, PAT ni secretos configurados por el usuario para compilar. Los jobs de PR no reciben credenciales persistentes ni usan pull_request_target.

`.gitattributes` fija finales de línea de código/documentación y desactiva transformación de los textos LICENSES originales, necesarios para conservar sus hashes de procedencia. Los scripts PowerShell pueden tener CRLF tras checkout; los textos upstream de licencias deben conservar sus bytes originales.

El manifiesto distingue paquetes generados en Actions de validación local. El runner contiene herramientas de desarrollo: verificar con PATH mínimo no equivale a validar el instalador final en VM Windows limpia ni a aprobar la distribución pública. No se exige segundo PC físico. Licencia del producto y condiciones de publicación siguen pendientes.

## Validación inicial local

Bootstrap ejecutado contra el archivo Qt local ya verificado, sintaxis PowerShell/Python y YAML revisadas, cinco casos inline del workflow satisfactorios contra el ZIP local.

## Ejecuciones remotas y correcciones

Las ejecuciones 1 y 2 fallaron antes de empaquetar. La anotación pública de la segunda identificó la extracción del archivo .7z con tar del runner. Se utiliza 7-Zip preinstalado, conservando tar como alternativa local y el log qt-extract.log. Las acciones v4 iniciales emitían un aviso por Node 20: se actualizaron a las revisiones Node 24 fijadas arriba. Los avisos por ausencia de artefactos tras un fallo temprano ya no ocultan la causa original.

La [ejecución 3](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42445885987), commit `d88bd05`, concluyó success. Conservó el artefacto `windows-x64-3` (15021233 bytes), con digest del contenedor de artefactos `sha256:447827a9fc7f56c6cb91ed5094ecc8351a5063885caa77689788ef84f8637a00`. Este digest no se debe confundir con SHA256SUMS.txt del ZIP portátil interior.

El workflow conserva inventario y verificación como JSON y publica anotaciones de diagnóstico sin credenciales: entorno de herramientas y resumen de casos/hash. No se necesitaron credenciales almacenadas para diagnosticar el fallo: los logs detallados estaban protegidos, pero se resolvió mediante anotaciones públicas del propio workflow.

El resultado de CI no sustituye la validación del instalador final en VM Windows limpia ni las comprobaciones de interacción OpenGL de usuario. El segundo PC físico deja de ser requisito. La imagen y toolchain real de cada ejecución quedan en ci-environment.json; no se afirma igualdad binaria entre el MSVC 2026 local y MSVC 2022 remoto.

## Resultado final verificado

[Ejecución 4 satisfactoria](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42446122445), commit `20c6c3b9cb65399861d283bac75237ed5292c864`. Todas las etapas concluyeron success: checkout, descarga/extracción Qt, compilación Release, empaquetado, verificación y subida de evidencias.

Entorno observado en las anotaciones públicas e inventario:

| Componente | Versión real |
|---|---|
| Imagen windows-2022 | win22, 20261101.353.1 |
| MSVC toolset | 14.44.35207 |
| Windows SDK | 10.0.26100.0 |
| CMake | 3.31.6 |
| Ninja | 1.13.2 |
| Python auxiliar | 3.12.10 |
| Qt Base | 6.8.3 |
| 7-Zip | 26.05 |
| CRT redistribuible | Microsoft.VC143.CRT, vcruntime140.dll 14.44.35211.0 |

Los cinco casos pasaron: cubo binario con encabezado solid, cubo ASCII, vacío, truncado y coordenadas no finitas. Se verificaron 70 archivos del manifiesto además del manifiesto mismo; comparación de inventario exacta y checksum del ZIP correctos, ejecutando la copia extraída con PATH mínimo.

[Artefacto windows-x64-4](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42446122445/artifacts/13603179337), disponible durante la retención de siete días: 15021586 bytes. Digest del contenedor de artefactos: `sha256:42e6d37ea829812bf23eaf9b568462d1c872046e35f8f2fd9cc2a0438c5a4a3c`.

ZIP portátil interior: `STLCutStudioAI-0.1.0-portable-win-x64-20261108T041710269Z.zip`; SHA-256 `148d472ee5d33204ec5f2c61f5ad3306bdad2974279dbb220f0170c8f6c87882`. El sufijo es UTC; la ejecución corresponde a la sesión local del 2026-11-07 en Bogotá.

Evidencia local de la consulta remota: build/ci-final-run.json, ci-final-jobs.json, ci-final-annotations.json y ci-final-artifacts.json. Esta actualización final solo cambia documentación, por lo que no dispara otra compilación de push; el código y workflow verificados son los del commit indicado.

Fuentes: [imagen windows-2022](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md), [checkout fijado](https://github.com/actions/checkout/tree/3d3c42e5aac5ba805825da76410c181273ba90b1), [upload-artifact fijado](https://github.com/actions/upload-artifact/tree/cf430e030ddbb5b0abf93d22962f4752f3646cd9).

## Workflow preparado para 0.2

Añadido bootstrap oficial de lib3mf 2.5.0 con SHA-256 y verificación del SDK. A los cinco casos de importación previos se suman STL binario con encabezado PK, un cubo 3MF, una extensión obligatoria desconocida y dos ciclos corte/exportación/reimportación (STL y 3MF): diez casos. El empaquetado incluye DLL y avisos de lib3mf. Diez casos ejecutados localmente contra el ZIP 0.2, con PATH mínimo y manifiesto de 72 archivos más manifiesto, satisfactorios. Estos cambios permanecen locales: no se ejecutó una nueva compilación remota ni se publicó otro artefacto GitHub durante este incremento. La evidencia nueva y el hash del ZIP están en PHASE_02.md.

## Workflow ampliado a 0.2.1

Adquisición de Manifold 3.5.4 con SHA-256 y contraste de fuentes; compilación estática sin dependencias opcionales ni pruebas upstream. Cinco casos adicionales: tubo hueco, cavidad encerrada, aristas coplanares, cara cóncava coplanar y contorno >4096; validan cierre/orientación, volumen, reimportación y huecos sin rellenar. Los quince casos del bloque se ejecutaron localmente contra el ZIP 0.2.1 con PATH mínimo. No se ejecutó GitHub Actions ni se subieron cambios en este incremento.

## Publicación y CI 0.2.1 — 2026-11-08

Usuario autorizó commit/push. Código y workflow subidos a main en `b1ede31a1a54700b2bd2495f5119ca600895a796` (importación 3MF, corte Manifold y exportación STL). Se excluyeron build/, SDK/binarios/ZIP e informe local, y el archivo del usuario prueba.stl permanece sin añadir a Git.

[Ejecución 5: Windows x64 Release](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42503090070): **success**. Adquisición/verificación Qt 6.8.3, lib3mf 2.5.0 y Manifold 3.5.4; build/paquete, quince casos del ZIP y subida del artefacto completados correctamente. Windows image win22 20261101.353.1; MSVC 14.44.35207, SDK 10.0.26100.0, CMake 3.31.6, Ninja 1.13.2, Python 3.12.10, CRT 14.44.35211.0 y 7-Zip 26.05. El bloque verificó 74 archivos más manifiesto y quince casos; no demuestra interacción GUI ni instalación en Windows limpio.

[Artefacto windows-x64-5](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42503090070/artifacts/13625494676): 16 088 466 bytes, retención de siete días. Digest del contenedor de artefactos: `sha256:c915d37223fd8e104bb014f9bb6e1c17a44e9a77ce588b89b28d2d62169919c8`.

ZIP interior: `STLCutStudioAI-0.2.1-portable-win-x64-20261108T170540075Z.zip`; SHA-256 `cee55363fc29221650e93b3d808d1ad5d603559114d6a77a9dc18eee2f5b2393`. No confundir con el ZIP local, cuyo build usa otro toolchain/timestamp. No es un release público ni el instalador final.

Consulta realizada mediante API pública de GitHub; sin extraer credenciales. Evidencia local ignorada por Git: build/ci-021-run.json, ci-021-jobs.json, ci-021-annotations.json y ci-021-artifacts.json. La actualización documental posterior no modifica el código verificado ni dispara otro build por el filtro del workflow.

## Incremento 0.2.2 — 2026-11-08

El workflow añade seis casos de historial a los quince existentes: encadenamiento/undo/redo, rama nueva, exportación, recorte de pasos, rechazo por memoria y límite de piezas. Los 21 pasaron localmente sobre el ZIP extraído con PATH mínimo. Tras autorización del usuario, el código se publicó y la ejecución 6 validó este incremento, según el registro siguiente.

## Publicación y CI 0.2.2 — 2026-11-08 (Colombia)

Código publicado en main como `34b0cb7eb839ca3fd96a3e88a9e4999d2f003bc1`. [Ejecución 6](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42545316348) completada con resultado success en todas las etapas: verificación de dependencias, build/paquete Release x64, 21 casos inline y carga de artefacto. Resultado consultado mediante la API pública de GitHub, sin acceder a credenciales.

[Artefacto windows-x64-6](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42545316348/artifacts/13642754585): 16 109 877 bytes, retención de siete días, no expirado al consultar. Digest del contenedor de Actions: `sha256:03c4fcce16e36b5505ed621587508d8361fcec5c8a28e24ac4a16b7256f1f757`; no equivale al checksum del ZIP interior ni al ZIP local. No se descargó ni verificó de nuevo su contenido fuera del runner. Sin release público, instalador final ni validación en VM limpia. `prueba.stl` permanece intacto y fuera de Git.

## Workflow local 0.2.3

Añadidos dos casos de rechazo float32: edición conserva sesión y no añade bytes inválidos; corte CLI no emite STL al fallar validación. Las 23 aceptaciones pasaron contra el paquete nuevo con PATH mínimo. Sin push ni ejecución remota de este incremento. Evidencia en EXPORT_VALIDATION.md.

## Workflow preparado para primer incremento 0.3.0

25 comprobaciones locales del paquete, incluyendo dos nuevas de conectores y conservando las de exportación 0.2.3. Sin publicación ni nueva ejecución remota; PHASE_03.md contiene hashes y límites.

## Workflow preparado para 0.3.1

Tres casos adicionales de color/material y separación con conectores planares; 28 aceptaciones ejecutadas localmente sobre el ZIP final con PATH mínimo. Se solicitan mensajes MSVC en inglés y se verifica que Ninja registre Mesh.h; inglés disponible en CI anterior, español normalizado y comprobado localmente. No se ha ejecutado una nueva CI remota de este incremento. Hashes y límites en COLOR_IMPORT.md.

## Workflow preparado para 0.3.2

Bootstrap miniz 3.1.0 con SHA-256 y cuatro casos nuevos de pintura/área, presentación CPU sin ventana, código truncado y CRC. Total previsto 32 casos; no ejecutados sobre la versión final por bloqueo local, ni en Actions porque no se publicó este incremento. No confundir sintaxis/inventario comprobados con aceptación dinámica. PAINT_IMPORT.md registra el estado.

Publicación autorizada por el usuario: se preparó además un paso independiente con los 36 casos de pintura antes bloqueados, usando el paquete extraído y PATH mínimo. La CI deberá ejecutar 32 casos generales y 36 de pintura; los modelos privados y reportes locales no se publican. Resultado pendiente al preparar esta subida.

## Publicación y CI 0.3.2 — 2026-11-08 (Colombia)

Publicación autorizada en main: `d2a219a853b8d5d16ef5f7bf50e8b07f6756ccd6`. [Ejecución 7](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42561054281) completada success en todas sus etapas: descarga/verificación de Qt/lib3mf/Manifold/miniz, build y paquete Release x64, **32 casos generales** y **36 de pintura**. El paso de pintura ejecuta también 80 árboles generados dentro de uno de sus casos y los límites reforzados de hojas, expansión, CRC/DTD/metadatos. Se resuelven las comprobaciones que no pudieron ejecutarse en la última compilación local; no se elimina ni modifica la política de Windows del equipo. Sin revisión visual/GPU ni prueba de instalador/VM limpia.

Entorno remoto registrado en anotación pública: windows-2022, imagen 20261101.353.1, MSVC toolset 14.44.35207, SDK 10.0.26100.0, CMake 3.31.6, Ninja 1.13.2, Python 3.12.10, 7-Zip 26.05 y CRT 14.44.35211.0. Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4 y miniz 3.1.0 fijados como en desarrollo. No son los mismos toolchain/CRT locales y no se promete igualdad binaria. Resultado consultado mediante API pública de GitHub, sin recuperar credenciales.

[Artefacto windows-x64-7](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42561054281/artifacts/13649129176): 16 205 053 bytes, no expirado al consultar, retención de siete días. Digest del contenedor Actions `sha256:f775045ade8ede74880f6b559c8c2a4e81a5a192a311e580a7a6cb085a616d8b`. ZIP interior según anotación pública del runner: `STLCutStudioAI-0.3.2-portable-win-x64-20261109T041931693Z.zip`, SHA-256 `8821814d105103dcaae82e56818b53ab32e3a5f4ba7c9bc5e8feffc0a153419c`, inventario de 75 archivos más manifiesto. No confundir sus hashes con el ZIP local ni con el contenedor Actions. No se descargó/ejecutó ese artefacto en el equipo local. Los reportes ci-verification.json y ci-paint-verification.json se conservan en el artefacto.

Fuera de Git: build/, SDK, binarios, modelos privados y prueba.stl. Sin release público ni instalador final. La publicación incluye los incrementos locales acumulados de exportación validada, conectores, materiales y pintura. Insertos, partición volumétrica y contactos generales continúan pendientes en esa versión 0.3.2.

## Publicación y CI 0.3.3–0.3.4 — 2026-11-09 (Colombia)

El usuario autorizó publicar ambos incrementos y completar la aceptación remota. Commit main `dbacc4ae2f378db64110f763d341ecea1a9ba149`. [Ejecución 8](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42751423136) finalizada **success** en todas las etapas: adquisición/verificación de dependencias, compilación/paquete Release, **32/32 comprobaciones generales** y **72/72 de pintura/volumen/conectores**. Incluye los 18 casos de reconstrucción y los 18 nuevos de conectores, entre ellos nominales, anillos STL, ambos modos, omisión por recorrido/pared, reflexión, exterior oblicuo, materiales con RGB igual, exportación e historial transaccional. Los casos antes bloqueados quedaron aceptados en el runner remoto; no se cambió la política local ni se reinterpretan las ejecuciones locales bloqueadas como correctas.

Entorno del runner según anotación pública: windows-2022 imagen 20261101.353.1, MSVC 14.44.35207, SDK 10.0.26100.0, CMake 3.31.6, Ninja 1.13.2, Python 3.12.10, 7-Zip 26.05, CRT 14.44.35211.0; Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4 y miniz 3.1.0 fijados. La validación de paquete usa PATH mínimo y carpeta con espacios/ñ; inventario de 75 archivos más manifiesto, tamaños, hashes y CRC correctos. Sin revisión visual ni prueba de instalación/VM limpia.

[Artefacto windows-x64-8](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42751423136/artifacts/13734061316): 16 269 794 bytes, no expirado al consultar, retención de siete días. Digest del contenedor Actions `sha256:fa3f45989b1b0e8929e96712be1a1d8add01d5d5d111d1698899b135e93a446f`. ZIP interior según anotación del runner: `STLCutStudioAI-0.3.4-portable-win-x64-20261110T034624946Z.zip`, SHA-256 `3de4106f60482d2b914d50b69872cca8a2b893e1e67e6a4c0c1552f38873eb1c`. No confundir esos dos hashes con el paquete local. No se descargó/ejecutó el artefacto en el equipo local. Los informes ci-verification.json y ci-paint-verification.json están incluidos en el artefacto.

Consultado por la API pública, sin recuperar credenciales. Evidencia local ignorada: `build/ci-034-run.json`, ci-034-jobs.json, ci-034-annotations.json y ci-034-artifacts.json. `prueba.stl` continúa fuera de Git e intacto. La actualización documental posterior no modifica el código validado ni dispara otro build por los filtros de rutas. No se publica un release ni instalador final. Persisten límites de resolución, búsqueda local y recorrido conservador, parámetros de reconstrucción sin recuperación de estado y reparación pendiente; no se declara cerrada la fase 0.3.
