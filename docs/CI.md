# CI Windows x64

Fecha: 2026-11-07. Configuración inicial preparada; la ejecución remota se registrará después de observar su resultado. El usuario autorizó subir los cambios acumulados y ejecutar Actions. No se publica un release ni se instalan servicios/secretos adicionales.

## Flujo

`.github/workflows/windows-ci.yml` se ejecuta en `windows-2022`, en pushes relevantes a main, pull requests a main y ejecución manual. Los pushes que solo cambian documentación no disparan otra compilación; cambios en código, scripts, CMake, licencias, avisos, atributos Git o workflow sí lo hacen.

1. Checkout sin persistir credenciales, permisos contents:read.
2. `bootstrap-windows.ps1`: descarga Qt Base 6.8.3 MSVC2022 x64 desde download.qt.io, exige 39484220 bytes y SHA-256 `41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f`, extrae con tar y comprueba qmake/archivos requeridos. Reutiliza un archivo previamente verificado y rechaza instalaciones parciales; no elimina archivos para reintentar. Conserva descargas parciales para diagnóstico.
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

Bootstrap ejecutado contra el archivo Qt local ya verificado, sintaxis PowerShell/Python y YAML revisadas, cinco casos inline del workflow satisfactorios contra el ZIP local. La descarga y compilación desde un runner nuevo se comprobarán mediante la primera ejecución remota; no se presentan como realizadas todavía.

Fuentes: [imagen windows-2022](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md), [checkout fijado](https://github.com/actions/checkout/tree/3d3c42e5aac5ba805825da76410c181273ba90b1), [upload-artifact fijado](https://github.com/actions/upload-artifact/tree/cf430e030ddbb5b0abf93d22962f4752f3646cd9).
