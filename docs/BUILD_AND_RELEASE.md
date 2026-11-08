# Compilación y paquete portátil Windows

> Estado vigente 0.2.1: Manifold 3.5.4 sustituye el motor propio; cavidades/casos coplanares y contornos >4096 comprobados. Contrato y evidencia en [MANIFOLD_CUT.md](MANIFOLD_CUT.md). Las secciones fechadas anteriores son históricas.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha: 2026-11-07. Paquete local de validación, no release público ni instalador. Importador y visor pertenecen a fase 0.1; el instalador/actualizador sigue en fase 1.0.

## Requisitos y comando comprobado

Usar Developer PowerShell de Build Tools con destino x64 y la raíz del repositorio. Entorno utilizado: Qt Base 6.8.3 MSVC x64, Build Tools 18.8.2, CMake 4.3.1-msvc1, Ninja 1.13.2, SDK 10.0.26100.0 y runtime redistribuible Microsoft.VC145.CRT del toolset 14.51.36231. DLL vcruntime140.dll: versión 14.51.36247.0, distinta de la versión del compilador.

```powershell
$env:STL_QT_ROOT = Join-Path (Get-Location) 'build/qt/6.8.3/msvc2022_64'
$crtRoot = Join-Path $env:VCINSTALLDIR 'Redist/MSVC/14.51.36231/x64/Microsoft.VC145.CRT'
.\scripts\package-windows.ps1 -CrtRoot $crtRoot
```

También admite `-QtRoot <prefijo Qt>` en lugar de STL_QT_ROOT. CrtRoot es explícito: debe apuntar al directorio x64 redistribuible, nunca al binario del compilador o a System32. No instala herramientas ni descarga paquetes. La ruta del runtime debe ajustarse si cambia el toolset, previa revisión de versión/compatibilidad. Las [licencias y avisos](../THIRD_PARTY_NOTICES.md) acompañan el paquete.

## Proceso implementado

1. Comprueba Qt 6.8.3, herramientas, runtime y hashes de los textos de licencia versionados.
2. Ejecuta build-windows.ps1, que configura y compila Release x64.
3. Crea una salida nueva en `build/packages/<timestamp-UTC>/`. Nunca elimina ni reutiliza una salida previa; un fallo conserva sus archivos y logs para diagnóstico.
4. Ejecuta cmake --install con prefijo de staging explícito. El destino RUNTIME es ahora `.`: el ejecutable queda en la raíz portátil.
5. Ejecuta windeployqt del kit con --release, qtpaths explícito y --no-compiler-runtime. Deshabilita traducciones no instaladas, rasterizador software OpenGL y compiladores D3D no usados por este visor OpenGL de escritorio.
6. Copia las DLL del directorio CRT redistribuible x64 indicado. UCRT y DLL del sistema se obtienen del Windows 10/11 compatible del destino; no se copian de System32.
7. Escribe qt.conf con Prefix y Plugins relativos; exige Qt6Core/Gui/Widgets/OpenGL/OpenGLWidgets y platforms/qwindows.dll. Verifica cabeceras PE AMD64 de todos los exe/DLL incluidos.
8. Copia textos LICENSES de Qt Base 6.8.3, procedencia, SBOM del kit y THIRD_PARTY_NOTICES.md. Añade README_PORTABLE.txt con uso y alcance real.
9. Genera manifest.json con versión, fecha UTC, plataforma, configuración, versión Qt/runtime y ruta/tamaño/SHA-256 de cada archivo. El manifiesto se excluye a sí mismo para evitar autorreferencia.
10. Comprime la carpeta completa en ZIP y genera SHA256SUMS.txt. No firma ni publica artefactos.

Salida:

```text
build/packages/<timestamp-UTC>/
  STLCutStudioAI-0.1.0-win-x64/
    STLCutStudioAI.exe
    Qt6*.dll
    msvcp140*.dll, vcruntime140*.dll y otras DLL CRT
    platforms/qwindows.dll y plugins seleccionados por windeployqt
    qt.conf
    licenses/
    THIRD_PARTY_NOTICES.md
    README_PORTABLE.txt
    manifest.json
  STLCutStudioAI-0.1.0-portable-win-x64-<timestamp-UTC>.zip
  SHA256SUMS.txt
  windeployqt-stdout.log
  windeployqt-stderr.log
```

El script usa rutas relativas al repositorio, restaura PATH/STL_QT_ROOT del proceso y utiliza UTF-8 con BOM para compatibilidad con Windows PowerShell 5.1. No se requieren API keys ni variables nuevas persistentes. Todos los binarios y ZIP permanecen bajo build/, excluido de Git; se versionan script, documentación y textos de licencia, no el kit Qt ni el paquete generado.

## Uso del paquete

Extraer **todo** el ZIP y abrir STLCutStudioAI.exe desde la carpeta extraída. No copiar solo el exe. El paquete contiene las DLL del entorno de ejecución, pero el visor requiere un controlador OpenGL 3.3 compatible. No lleva IA, compiladores, vcpkg ni Python.

Verificación del checksum desde la carpeta de salida:

```powershell
$zip = Get-ChildItem -File -Filter '*.zip' | Select-Object -First 1
$expected = (Get-Content .\SHA256SUMS.txt).Split(' ')[0]
if ((Get-FileHash -LiteralPath $zip.FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'El checksum ZIP no coincide.'
}
```

## Validación local realizada

- Sintaxis PowerShell y build Release satisfactorios.
- ZIP íntegro por CRC; inventario exacto de 71 archivos: 70 entradas verificadas por tamaño/SHA-256 y el manifiesto mismo. SHA-256 ZIP contrastado con SHA256SUMS.txt.
- Extracción en un directorio con espacios y `ñ` (U+00F1), independiente del staging. Sin archivos STL ni fuentes de pruebas.
- Procesos hijos con PATH limitado a `C:\WINDOWS\System32;C:\WINDOWS`, variables Qt/QML/STL_QT_ROOT retiradas y directorio de trabajo ajeno al paquete. Solo se habilitó QT_FORCE_STDERR_LOGGING para recoger evidencia.
- --inspect-stdin: cubo binario de 12 triángulos, dimensiones 10×20×30, salida 0. Entrada vacía: error controlado y salida 1.
- GUI --view-stdin: recepción de la malla y envío de buffer OpenGL observados. Se enumeraron módulos cargados del proceso: Qt6Core, Gui, Widgets, OpenGL, OpenGLWidgets, qwindows, msvcp140, vcruntime140 y vcruntime140_1 procedían de la carpeta extraída. Ningún módulo procedía del kit de desarrollo.
- El proceso GUI se terminó por el coordinador después de comprobar sus módulos; no se presenta ese cierre como una salida normal. Los modos CLI sí terminaron normalmente con los códigos indicados.

Evidencia vigente: `build/portable-validation.json`, `build/portable-startup-stderr.log`, `build/package-build.log` y logs de windeployqt en la salida. El JSON identifica el ZIP, su hash, tamaño, ruta extraída y DLL realmente cargadas; no es un informe de VM limpia.

Durante el primer empaquetado, PowerShell 5.1 convertía el aviso stderr de windeployqt en error terminante por ErrorActionPreference=Stop. Se corrigió capturando stdout/stderr por separado y verificando el código de salida real. El aviso de traducciones ausentes persiste porque no se instaló ese componente; --no-translations es intencional y la interfaz propia mantiene sus textos españoles. No se ignoran códigos de fallo ni se inventan catálogos. Los sondeos opcionales de Vulkan del build tampoco impidieron la compilación.

Los primeros intentos del comprobador tuvieron errores de alias de codificación y normalización de claves del entorno Windows; se corrigieron y se descartaron. La validación satisfactoria corresponde al ZIP final identificado por el informe.

## Límites y siguiente paso

El equipo de validación conserva herramientas de desarrollo instaladas: PATH mínimo y trazado de módulos dan evidencia local de independencia del kit, pero no sustituyen la validación del instalador final en VM Windows limpia. No se exige segundo PC físico. El ZIP provisional y CI ya están comprobados con el alcance documentado; siguen pendientes las comprobaciones funcionales ampliadas, otros controladores y derechos/licencia de distribución.

## Entrega final del programa completo — fase 1.0

Entregar `STLCutStudioAI-Setup-<version>-win-x64.exe`, que instale la aplicación y las dependencias necesarias. Tras instalar, abrir `STLCutStudioAI.exe` sin preparar Qt, Python, compiladores ni gestores de desarrollo manualmente. Validar instalación, apertura, funciones entregadas y desinstalación en VM Windows limpia. El ZIP portátil de fase 0.1 es provisional y no satisface por sí solo esta entrega final.

El instalador no existe todavía. Esta actualización es solo documental: no cambia scripts, manifiestos, README internos ni ZIP ya generados. Estos pueden conservar el alcance histórico con referencias al segundo PC; las plantillas del empaquetador se alinearán al modificar código con autorización posterior. El requisito vigente es el acordado aquí y en MASTER.md, sin segundo PC físico.

Fuentes técnicas: [despliegue Qt 6.8](https://doc.qt.io/qt-6.8/windows-deployment.html), [despliegue Microsoft app-local](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170) y ayuda local de windeployqt 6.8.3.

## Automatización CI

El workflow Windows utiliza este empaquetador con MSVC serie 14.44 y registra el entorno real por ejecución. Qt mantiene versión y checksum. Véase [CI.md](CI.md) para comandos, artefactos y validación remota.

## Paquete provisional 0.2

Ejecutar bootstrap-lib3mf.ps1 antes del build/empaquetado. CMake instala tambi?n lib3mf.dll y su License.txt completo en licenses/lib3mf-2.5.0; el manifiesto del paquete incluye lib3mf_version=2.5.0 e inventario SHA-256. Los mensajes del empaquetador ya reflejan STL/3MF/corte/exportación y no exigen segundo PC. QSaveFile se usa para exportaciones de malla; no reemplaza el empaquetado. El instalador final y VM limpia siguen en fase 1.0. No confundir la ejecución CI histórica 0.1 con el paquete actual.

## Paquete provisional 0.2.1 con Manifold

Ejecutar bootstrap-manifold.ps1 antes del build. El motor está enlazado estáticamente; CMake instala solo LICENSE/AUTHORS originales en licenses/manifold-3.5.4 y excluye la instalación de headers/bibliotecas de desarrollo del subproyecto. Manifiesto con manifold_version=3.5.4 y manifold_linkage=static. Paquete actual verificado localmente: 74 archivos más manifiesto, 15 casos de importación/corte/reimportación con PATH mínimo y revisión visible/cancelación. Hash y limitaciones en MANIFOLD_CUT.md. Es un ZIP provisional; no se implementó instalador ni se publicó release.
