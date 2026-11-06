# Dependencias del primer incremento

Fecha: 2026-11-06. Inventario de declaraciones y herramientas detectadas; no acredita distribución.

| Componente | Versión | Finalidad | Licencia / estado |
|---|---|---|---|
| Qt Core, Gui, Widgets | 6.8.3 exacta | Ventana; Core/Gui transitivas de Widgets | LGPLv3/GPLv3 o comercial según paquete y componentes; Qt Base instalado localmente desde download.qt.io; no distribuido |
| MSVC Build Tools | 18.8.2; toolset 14.51.36231 | Compilador Microsoft instalado | Términos Microsoft; revisión del runtime pendiente |
| Windows SDK | 10.0.26100.0 | Headers localizados | Términos Microsoft |
| CMake | 4.3.1-msvc1 detectada; mínimo 3.24 | Configuración | BSD-3-Clause upstream; revisar avisos de la distribución Microsoft |
| Ninja | 1.13.2 | Compilación | Apache-2.0 |

Origen y referencias: [Qt 6.8.3](https://doc.qt.io/qt-6.8/cmake-get-started.html), [licencias Qt](https://doc.qt.io/qt-6/licensing.html), [CMake](https://cmake.org/licensing/), [Ninja](https://github.com/ninja-build/ninja/blob/v1.13.2/COPYING). Herramientas Microsoft detectadas en la instalación local de Visual Studio.

Se exige Qt dinámico. Esto por sí solo no satisface todas las obligaciones de distribución. La licencia del producto está pendiente; antes de distribuir deben revisarse las condiciones del kit real y conservarse los textos y materiales exigidos. Se descargó Qt Base 6.8.3 para desarrollo local, verificando SHA-1 oficial y calculando SHA-256 local; véase docs/WINDOWS_SETUP.md. No se redistribuyó.

Sin VTK, CGAL, libigl, Python ni IA como dependencias de la aplicación. Python 3.14.7 preexistente y bsdtar 3.8.9 se usaron como herramientas auxiliares; no se instalaron paquetes Python. Qt se fija en CMake; no hay lock de transitivas ni toolchain reproducible verificada entre equipos.
