# Dependencias del primer incremento

## Manifold 3.5.4 — incremento 0.2.1

Motor de corte 3D aprobado, [fuente oficial v3.5.4](https://github.com/elalish/manifold/tree/v3.5.4), commit ce50d78021d64507f89e8c9fc2c2e51018117857, licencia Apache-2.0. Compilado estáticamente; LICENSE y AUTHORS originales se instalan en licenses/manifold-3.5.4. Su linalg.h deriva del trabajo de dominio público de Sterling Orsten y contiene las atribuciones y términos de la adaptación de The Manifold Authors. TBB, Clipper2, bindings y pruebas upstream desactivados; no se distribuyen esos componentes opcionales. Fuente/checksum fijados en scripts/bootstrap-manifold.ps1 y docs/MANIFOLD_CUT.md. No requiere Python, CGAL ni una DLL Manifold adicional. Esta dependencia no resuelve por sí sola las decisiones de licencia del producto y cumplimiento de los demás componentes.

Fecha: 2026-11-07. Inventario de declaraciones y herramientas detectadas; no acredita distribución.

| Componente | Versión | Finalidad | Licencia / estado |
|---|---|---|---|
| Qt Core, Gui, Widgets, OpenGL, OpenGLWidgets | 6.8.3 exacta | Interfaz y visor; módulos presentes en el kit Qt Base instalado | LGPLv3/GPLv3 o comercial según paquete y componentes; Qt Base instalado localmente desde download.qt.io; no distribuido |
| MSVC Build Tools | 18.8.2; toolset 14.51.36231 | Compilador Microsoft instalado | Términos Microsoft; revisión del runtime pendiente |
| Windows SDK | 10.0.26100.0 | Headers localizados | Términos Microsoft |
| CMake | 4.3.1-msvc1 detectada; mínimo 3.24 | Configuración | BSD-3-Clause upstream; revisar avisos de la distribución Microsoft |
| Ninja | 1.13.2 | Compilación | Apache-2.0 |

Origen y referencias: [Qt 6.8.3](https://doc.qt.io/qt-6.8/cmake-get-started.html), [licencias Qt](https://doc.qt.io/qt-6/licensing.html), [CMake](https://cmake.org/licensing/), [Ninja](https://github.com/ninja-build/ninja/blob/v1.13.2/COPYING). Herramientas Microsoft detectadas en la instalación local de Visual Studio.

Se exige Qt dinámico. Esto por sí solo no satisface todas las obligaciones de distribución. La licencia del producto está pendiente; antes de distribuir deben revisarse las condiciones del kit real y conservarse los textos y materiales exigidos. Se descargó Qt Base 6.8.3 para desarrollo local, verificando SHA-1 oficial y calculando SHA-256 local; véase docs/WINDOWS_SETUP.md. No se redistribuyó.

Sin VTK, CGAL, libigl, Python ni IA como dependencias de la aplicación. Python 3.14.7 preexistente y bsdtar 3.8.9 se usaron como herramientas auxiliares; no se instalaron paquetes Python. Qt se fija en CMake; no hay lock de transitivas ni toolchain reproducible verificada entre equipos.

## Paquete portátil local — 2026-11-07

El paquete de validación incluye Qt Core/Gui/Widgets/OpenGL/OpenGLWidgets 6.8.3, plugins seleccionados por windeployqt y DLL MSVC tomadas exclusivamente del directorio redistribuible `VC/Redist/MSVC/14.51.36231/x64/Microsoft.VC145.CRT` del Build Tools instalado. No se copian DLL desde el binario del compilador ni desde System32. Se utiliza despliegue app-local según [Microsoft](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170); mantener esos archivos actualizados es responsabilidad de la distribución.

Se conservan los 37 textos LICENSES del [tag oficial Qt Base v6.8.3](https://github.com/qt/qtbase/tree/v6.8.3/LICENSES), con procedencia y SHA-256 en `third_party/qt/6.8.3/provenance.json`. La adquisición verificó cada Git blob SHA-1 publicado por la API. El empaquetador vuelve a comprobar SHA-256 y copia estos textos, los avisos presentes y el SBOM del kit, que contiene declaraciones y textos de licencias adicionales. Incluir todos los textos no significa seleccionar simultáneamente todas las licencias alternativas de Qt.

Fuentes Qt: [tag v6.8.3](https://code.qt.io/cgit/qt/qtbase.git/tag/?h=v6.8.3). Se mantienen DLL reemplazables y enlace dinámico; no se afirma que este paquete local complete por sí solo las obligaciones de una distribución pública. La licencia del producto, derechos de redistribución bajo la licencia Microsoft aplicable y cumplimiento final siguen pendientes de la decisión de distribución. No se ha publicado un release público.

## Herramientas exclusivas de CI

Acciones oficiales bajo licencia MIT, fijadas por commit: [checkout v7.0.1](https://github.com/actions/checkout/blob/3d3c42e5aac5ba805825da76410c181273ba90b1/LICENSE) (`3d3c42e5aac5ba805825da76410c181273ba90b1`) y [upload-artifact v7.0.2](https://github.com/actions/upload-artifact/blob/cf430e030ddbb5b0abf93d22962f4752f3646cd9/LICENSE) (`cf430e030ddbb5b0abf93d22962f4752f3646cd9`). Se usan para checkout y conservación de evidencias; no son dependencias del ejecutable. El runner aporta Windows Server 2022, MSVC serie 14.44 y herramientas inventariadas por ejecución. Véase docs/CI.md.

7-Zip, preinstalado en la imagen del runner, se usa únicamente para extraer Qt; no se incluye en el ZIP de la aplicación. La imagen consultada anuncia 26.05 y el bootstrap registra la versión real usada. Su [licencia oficial](https://www.7-zip.org/license.txt) incluye LGPL-2.1-or-later y términos BSD/unRAR para componentes concretos. No se descarga ni instala una versión adicional.

## lib3mf 2.5.0 — incremento 0.2

Dependencia de ejecución para leer 3MF, SDK oficial del [3MF Consortium](https://github.com/3MFConsortium/lib3mf/releases/tag/v2.5.0). Licencia BSD-2-Clause. Se utiliza lib3mf.dll x64 y sus bindings/import library; no se incorporan CGAL ni libigl. Descarga y SHA-256 fijados en scripts/bootstrap-lib3mf.ps1 y docs/PHASE_02.md. El paquete instala sin modificar el License.txt completo del SDK, con avisos de lib3mf, libzip 1.10.1, zlib (el texto del SDK rotula 1.3.0), cpp-base64 2.rc.08 y fast_float 6.0.0. No se infiere una versión binaria de zlib distinta a partir del README actual upstream. Se conservan íntegros los avisos transitivos suministrados por el SDK. La selección de licencia del producto y revisión final de distribución continúan pendientes.
