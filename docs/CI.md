# CI Windows x64

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

El manifiesto distingue paquetes generados en Actions de validación local. El runner contiene herramientas de desarrollo: verificar con PATH mínimo tampoco equivale a Windows limpio, segundo equipo del usuario o distribución pública aprobada. Licencia del producto y condiciones de publicación siguen pendientes.

## Validación inicial local

Bootstrap ejecutado contra el archivo Qt local ya verificado, sintaxis PowerShell/Python y YAML revisadas, cinco casos inline del workflow satisfactorios contra el ZIP local.

## Ejecuciones remotas y correcciones

Las ejecuciones 1 y 2 fallaron antes de empaquetar. La anotación pública de la segunda identificó la extracción del archivo .7z con tar del runner. Se utiliza 7-Zip preinstalado, conservando tar como alternativa local y el log qt-extract.log. Las acciones v4 iniciales emitían un aviso por Node 20: se actualizaron a las revisiones Node 24 fijadas arriba. Los avisos por ausencia de artefactos tras un fallo temprano ya no ocultan la causa original.

La [ejecución 3](https://github.com/GerDeveelop/STL-model-3D/actions/runs/42445885987), commit `d88bd05`, concluyó success. Conservó el artefacto `windows-x64-3` (15021233 bytes), con digest del contenedor de artefactos `sha256:447827a9fc7f56c6cb91ed5094ecc8351a5063885caa77689788ef84f8637a00`. Este digest no se debe confundir con SHA256SUMS.txt del ZIP portátil interior.

El workflow conserva inventario y verificación como JSON y publica anotaciones de diagnóstico sin credenciales: entorno de herramientas y resumen de casos/hash. No se necesitaron credenciales almacenadas para diagnosticar el fallo: los logs detallados estaban protegidos, pero se resolvió mediante anotaciones públicas del propio workflow.

El resultado de CI no sustituye el segundo equipo del usuario, Windows limpio ni interacción OpenGL en un runner gráfico. La imagen y toolchain real de cada ejecución quedan en ci-environment.json; no se afirma igualdad binaria entre el MSVC 2026 local y MSVC 2022 remoto.

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
