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
try {
    & cmake --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "Configuración CMake fallida: $LASTEXITCODE" }
    & cmake --build --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "Compilación fallida: $LASTEXITCODE" }
}
finally {
    Pop-Location
}
