# Configuración Windows

Actualizado: 2026-11-06. Qt Base 6.8.3 MSVC x64 instalado localmente; configuración y compilación Release con importador STL satisfactorias. Uso y contratos en STL_IMPORT.md. Arranque de ventana oculta y cierre normal comprobados, sin revisión visual interactiva.

## Herramientas comprobadas

| Herramienta | Versión | Situación |
|---|---|---|
| Visual Studio Build Tools | 2026, 18.8.2 / 18.8.12023.21 | Developer PowerShell x64 |
| MSVC | Toolset 14.51.36231; compilador 19.51.36252.0 | Compilación Release correcta |
| Windows SDK | 10.0.26100.0 | Headers y herramientas detectados |
| CMake | 4.3.1-msvc1 | Incluido en Build Tools; mínimo declarado 3.24 |
| Ninja | 1.13.2 | Incluido en Build Tools |
| Qt Base | 6.8.3, kit MSVC2022 x64 dinámico | Instalado en build/qt/6.8.3/msvc2022_64 |
| tar | bsdtar 3.8.9 | Extracción del archivo oficial .7z |
| Python | 3.14.7, preexistente | Solo descarga y verificaciones auxiliares con biblioteca estándar |

Visual Studio Community 18.6.2 también está instalado, pero no se utilizó. Git no estaba en PATH ni en las rutas habituales inicialmente inspeccionadas. No se instalaron vcpkg, Python ni paquetes pip.

## Instalación Qt realizada

El usuario autorizó instalar/configurar Qt y después sustituir 6.8.9 por 6.8.3. La versión 6.8.9 fue fijada inicialmente a partir de documentación; el índice público consultado no ofrece su kit. Se corrigió esa selección antes de instalar.

Se descargó únicamente Qt Base del [repositorio oficial MSVC x64](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/):

```text
Archivo: 6.8.3-0-202503201308qtbase-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z
Tamaño: 39484220 bytes
SHA-1 publicado y verificado: ab0bd2551618f7128ef30c6d74fe45b6a76e2296
SHA-256 calculado localmente: 41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f
```

La descarga se hizo con urllib.request de Python, comparando SHA-1 con el archivo .sha1 oficial antes de extraer. El SHA-256 es referencia local del archivo recibido, no un hash publicado por Qt. Metadatos: build/tooling/qtbase-download.json. Archivo comprimido en el mismo directorio. Se extrajo con `tar -xf <archivo> -C <destino>` al prefijo indicado. `bin/qmake.exe -query QT_VERSION` devolvió 6.8.3.

No se modificaron PATH ni variables persistentes del sistema. Kit y binarios están bajo build/, excluido de Git. Borrar esa carpeta también borraría el kit; conservar el archivo verificado o reinstalar desde el origen documentado. No hay instalación automatizada en el repositorio todavía.

## Compilación comprobada

Desde Developer PowerShell de Visual Studio con destino x64 y la raíz del repositorio:

```powershell
$env:STL_QT_ROOT = Join-Path (Get-Location) 'build/qt/6.8.3/msvc2022_64'
.\scripts\build-windows.ps1
```

El script ejecuta `cmake --preset windows-x64-release` y `cmake --build --preset windows-x64-release`: completados con código 0. Ejecutable: build/windows-x64-release/STLCutStudioAI.exe.

STL_QT_ROOT es una ruta local no secreta. No hay API keys ni servicios externos. Se usa C++20, Qt dinámico y runtime MSVC dinámico. El script no instala software.

El intento previo se detenía detectando ABI de MSVC dentro del aislamiento del agente. La misma configuración fuera del aislamiento completó esa detección y compiló con Qt instalado; no fue necesario reparar MSVC. La comparación no identifica el mecanismo concreto del bloqueo. No se omitieron comprobaciones del compilador. CMake informó ausencia opcional de Vulkan headers, sin impedir configuración ni compilación.

## Ejecución local

Para abrir manualmente la base desde PowerShell, usando el kit de desarrollo:

```powershell
$qtKit = Join-Path (Get-Location) 'build/qt/6.8.3/msvc2022_64'
$env:PATH = (Join-Path $qtKit 'bin') + ';' + $env:PATH
$env:QT_PLUGIN_PATH = Join-Path $qtKit 'plugins'
& .\build\windows-x64-release\STLCutStudioAI.exe
```

Se verificó arranque oculto por API Windows: WaitForInputIdle satisfactorio, ventana con título STL Cut Studio AI 0.1.0 — Base de escritorio, cierre WM_CLOSE y salida 0. Stderr vacío. Evidencias: build/windows-x64-release/startup-result.json y startup-stderr.log. MainWindowHandle no detectaba la ventana oculta; se usó enumeración de ventanas del proceso. La lectura inicial de ExitCode desde PowerShell devolvió nulo; se verificó después con subprocess de Python y código 0 real. No se inspeccionaron visualmente menús ni contenido.

## Pendientes

Apertura GUI de archivo STL válido, cancelación durante importación, visor, revisión visual, toolchain reproducible en dos equipos, CI y paquete DLL/plugins verificado en Windows limpio. El ejecutable necesita el kit local; no es una distribución autónoma. La licencia del producto y cumplimiento de distribución siguen pendientes; véase [inventario](../THIRD_PARTY_NOTICES.md).

Sin carpetas ni archivos de pruebas propios. Comprobaciones ejecutadas en línea; archivos de detección CMake y herramientas de terceros bajo build/ pertenecen al entorno de construcción. No se añadieron Python/IA a la aplicación, VTK, CGAL ni manifiestos vcpkg vacíos. Fase 0.1 abierta.
