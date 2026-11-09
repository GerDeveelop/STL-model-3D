[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $env:STL_QT_ROOT -or
    -not (Test-Path -LiteralPath (Join-Path $env:STL_QT_ROOT 'lib/cmake/Qt6/Qt6Config.cmake'))) {
    throw 'Defina STL_QT_ROOT con la ruta de Qt 6.8.3 MSVC x64 dinámico. Consulte docs/WINDOWS_SETUP.md. No se instalará software automáticamente.'
}
foreach ($toolName in @('cmake', 'ninja', 'cl')) {
    if (-not (Get-Command $toolName -ErrorAction SilentlyContinue)) {
        throw "No se encuentra $toolName. Use Developer PowerShell para MSVC x64 con CMake y Ninja en PATH."
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'Abra Developer PowerShell con arquitectura de destino x64.'
}

Push-Location (Split-Path -Parent $PSScriptRoot)
$previousVsLang = $env:VSLANG
$env:VSLANG = '1033'
try {
    # A localized /showIncludes prefix can be mis-decoded by compiler probing.
    # Migrate such caches once and rebuild all objects, avoiding mixed Mesh ABIs.
    $dependencyMarker = Join-Path (Get-Location) 'build/windows-x64-release/stlcut-deps-v1.txt'
    $refreshCompiler = (Test-Path -LiteralPath 'build/windows-x64-release/CMakeCache.txt') -and
        -not (Test-Path -LiteralPath $dependencyMarker)
    $configureArgs = @('--preset', 'windows-x64-release')
    if ($refreshCompiler) { $configureArgs += '--fresh' }
    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "Configuración CMake fallida: $LASTEXITCODE" }
    $buildArgs = @('--build', '--preset', 'windows-x64-release')
    if ($refreshCompiler) { $buildArgs += '--clean-first' }
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) { throw "Compilación fallida: $LASTEXITCODE" }
    $readerDeps = (& ninja -C build/windows-x64-release -t deps CMakeFiles/stlcut_core.dir/core/io/StlReader.cpp.obj) -join "`n"
    if ($LASTEXITCODE -ne 0 -or $readerDeps -notmatch 'core[\\/]mesh[\\/]Mesh\.h') {
        throw 'Ninja no registra Mesh.h como dependencia; no usar binarios con ABI potencialmente desactualizada.'
    }
    'showIncludes prefix normalized; clean ABI migration completed' | Set-Content -LiteralPath $dependencyMarker -Encoding Ascii
}
finally {
    $env:VSLANG = $previousVsLang
    Pop-Location
}
