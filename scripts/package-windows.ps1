[CmdletBinding()]
param(
    [string]$QtRoot = $env:STL_QT_ROOT,
    [Parameter(Mandatory = $true)][string]$CrtRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot
$utf8 = New-Object System.Text.UTF8Encoding($false)
function Write-Utf8([string]$Path, [string]$Text) {
    [IO.File]::WriteAllText($Path, $Text, $utf8)
}
function Assert-Amd64([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $reader = New-Object IO.BinaryReader($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "No es un PE: $Path" }
        $stream.Position = 0x3C
        $offset = $reader.ReadInt32()
        if ($offset -lt 64 -or $offset -gt $stream.Length - 26) { throw "PE inválido: $Path" }
        $stream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) {
            throw "Se requiere PE AMD64: $Path"
        }
    } finally { $reader.Dispose(); $stream.Dispose() }
}

if (-not $QtRoot) { throw 'Indique QtRoot o STL_QT_ROOT: Qt 6.8.3 MSVC x64.' }
$QtRoot = (Resolve-Path -LiteralPath $QtRoot).Path
$CrtRoot = (Resolve-Path -LiteralPath $CrtRoot).Path
if ($CrtRoot -notmatch '[\\/]x64[\\/]Microsoft\.VC\d+\.CRT$') {
    throw 'CrtRoot debe ser el directorio x64/Microsoft.VC*.CRT de VC/Redist/MSVC, no el binario del compilador.'
}
foreach ($tool in @('bin/windeployqt.exe','bin/qtpaths.exe','bin/qmake.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $QtRoot $tool))) { throw "Falta Qt $tool" }
}
$qtVersion = & (Join-Path $QtRoot 'bin/qmake.exe') -query QT_VERSION
if ($LASTEXITCODE -ne 0 -or $qtVersion -ne '6.8.3') { throw 'Se requiere Qt 6.8.3 exacto.' }
$licenseRoot = Join-Path $repoRoot 'third_party/qt/6.8.3'
foreach ($required in @('LICENSES/LGPL-3.0-only.txt','LICENSES/GPL-3.0-only.txt','provenance.json')) {
    if (-not (Test-Path -LiteralPath (Join-Path $licenseRoot $required))) { throw "Falta licencia Qt: $required" }
}
$provenance = [IO.File]::ReadAllText((Join-Path $licenseRoot 'provenance.json')) | ConvertFrom-Json
if ($provenance.qt_version -ne '6.8.3') { throw 'Versión de licencias Qt inconsistente.' }
foreach ($record in $provenance.files) {
    if ($record.file -notmatch '^[^\\/]+\.txt$' -or $record.sha256 -notmatch '^[a-f0-9]{64}$') {
        throw 'Registro de procedencia de licencias inválido.'
    }
    $licenseFile = Join-Path (Join-Path $licenseRoot 'LICENSES') $record.file
    if ((Get-FileHash -LiteralPath $licenseFile -Algorithm SHA256).Hash.ToLowerInvariant() -ne $record.sha256) {
        throw "Licencia Qt modificada o incompleta: $($record.file)"
    }
}
foreach ($required in @('msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $CrtRoot $required))) { throw "Falta runtime: $required" }
}

$savedQtRoot = $env:STL_QT_ROOT
$savedPath = $env:PATH
Push-Location $repoRoot
try {
    $env:STL_QT_ROOT = $QtRoot
    & (Join-Path $PSScriptRoot 'build-windows.ps1')
    $project = [IO.File]::ReadAllText((Join-Path $repoRoot 'CMakeLists.txt'))
    $versionMatch = [regex]::Match($project, 'project\(STLCutStudioAI VERSION (\d+\.\d+\.\d+)')
    if (-not $versionMatch.Success) { throw 'No se pudo determinar la versión de CMake.' }
    $version = $versionMatch.Groups[1].Value
    $timestamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $name = "STLCutStudioAI-$version-portable-win-x64-$timestamp"
    $outputRoot = Join-Path $repoRoot "build/packages/$timestamp"
    $portable = Join-Path $outputRoot "STLCutStudioAI-$version-win-x64"
    # Never delete/reuse another package directory.
    if (Test-Path -LiteralPath $outputRoot) { throw "La salida ya existe: $outputRoot" }
    New-Item -ItemType Directory -Path $portable | Out-Null
    & cmake --install build/windows-x64-release --config Release --prefix $portable
    if ($LASTEXITCODE -ne 0) { throw 'Falló cmake --install.' }
    $exe = Join-Path $portable 'STLCutStudioAI.exe'
    Assert-Amd64 $exe
    $env:PATH = (Join-Path $QtRoot 'bin') + ';' + $savedPath
    $savedPreference = $ErrorActionPreference
    try {
        # Windows PowerShell 5.1 turns native stderr warnings into ErrorRecords.
        # Record them separately; the tool exit code determines deployment failure.
        $ErrorActionPreference = 'Continue'
        & (Join-Path $QtRoot 'bin/windeployqt.exe') --release --dir $portable `
            --qtpaths (Join-Path $QtRoot 'bin/qtpaths.exe') --no-compiler-runtime `
            --no-translations --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler $exe `
            1>(Join-Path $outputRoot 'windeployqt-stdout.log') 2>(Join-Path $outputRoot 'windeployqt-stderr.log')
        $deployExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($deployExit -ne 0) { throw "Falló windeployqt ($deployExit); consulte los logs en $outputRoot" }
    Get-ChildItem -LiteralPath $CrtRoot -Filter '*.dll' -File | ForEach-Object {
        Assert-Amd64 $_.FullName
        Copy-Item -LiteralPath $_.FullName -Destination $portable
    }
    Write-Utf8 (Join-Path $portable 'qt.conf') "[Paths]`nPrefix=.`nPlugins=.`n"
    foreach ($required in @('Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','Qt6OpenGL.dll',
                           'Qt6OpenGLWidgets.dll','platforms/qwindows.dll',
                           'msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $portable $required))) { throw "Paquete incompleto: $required" }
    }
    Get-ChildItem -LiteralPath $portable -Recurse -File | Where-Object Extension -in @('.dll','.exe') |
        ForEach-Object { Assert-Amd64 $_.FullName }
    $notices = Join-Path $portable 'licenses'
    New-Item -ItemType Directory -Path $notices -Force | Out-Null
    Copy-Item -LiteralPath $licenseRoot -Destination (Join-Path $notices 'qtbase-6.8.3') -Recurse
    Copy-Item -LiteralPath (Join-Path $QtRoot 'sbom') -Destination (Join-Path $notices 'qt-sbom') -Recurse
    Copy-Item -LiteralPath (Join-Path $repoRoot 'THIRD_PARTY_NOTICES.md') -Destination $portable
    Write-Utf8 (Join-Path $portable 'README_PORTABLE.txt') @"
STL Cut Studio AI $version - paquete portátil de validación Windows x64

Extraiga TODO el ZIP y abra STLCutStudioAI.exe. No copie el exe aislado.
No requiere el kit Qt en PATH; incluye sus DLL/plugins y runtime MSVC local.
Requiere Windows 10/11 x64 y controlador OpenGL 3.3 compatible para el visor.
Incluye importación STL/3MF, corte por plano con límites documentados y exportación STL.
Permite cortar piezas resultantes y Deshacer/Rehacer (Ctrl+Z/Ctrl+Y) durante la sesión.
Conserva colores/materiales 3MF estándar; separa regiones que ya forman sólidos cerrados.
Pin/cavidad opcionales al cortar o separar dos regiones con contacto plano en ejes.
Lee pintura paint_color/mmu_segmentation como regiones superficiales y conserva la malla fuente.
Los modos volumétricos de insertos/partición y contactos generales todavía no están implementados.
Historial: hasta 32 pasos y 256 piezas, presupuesto de 1 GiB de geometría/vistas;
puede retirar pasos antiguos y lo informa. No guarda proyectos/historial al cerrar.
No incluye IA, instalador final, actualizador ni firma digital.
STL no define unidades. La apertura no modifica el archivo fuente.

Licencias: THIRD_PARTY_NOTICES.md y licenses/. El producto aún no tiene
licencia de distribución acordada; este es un paquete de validación, no un release público.
Antes de publicar/distribuir, completar la revisión de derechos y cumplimiento.
Fuente de Qt Base 6.8.3: https://code.qt.io/cgit/qt/qtbase.git/tag/?h=v6.8.3
Fuente del proyecto: https://github.com/GerDeveelop/STL-model-3D
No se afirma validación en otro PC ni Windows limpio.
"@
    $crtVersion = (Get-Item -LiteralPath (Join-Path $portable 'vcruntime140.dll')).VersionInfo.FileVersion
    $files = @(Get-ChildItem -LiteralPath $portable -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($portable.Length+1).Replace('\','/');
                   bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
    $validationScope = if ($env:GITHUB_ACTIONS -eq 'true') {
        'github-actions-windows-2022; final installer and clean Windows VM validation pending for 1.0'
    } else { 'local-only; final installer and clean Windows VM validation pending for 1.0' }
    $manifest = [ordered]@{schema_version=1; application='STLCutStudioAI'; version=$version;
        created_utc=$timestamp; platform='windows-x64'; configuration='Release';
        qt_version=$qtVersion; msvc_runtime_version=$crtVersion;
        lib3mf_version='2.5.0';
        manifold_version='3.5.4'; manifold_linkage='static';
        miniz_version='3.1.0'; miniz_linkage='static-read-only';
        validation_scope=$validationScope;
        manifest_excludes_itself=$true; files=$files}
    Write-Utf8 (Join-Path $portable 'manifest.json') ($manifest | ConvertTo-Json -Depth 8)
    $zip = Join-Path $outputRoot "$name.zip"
    Compress-Archive -LiteralPath $portable -DestinationPath $zip -CompressionLevel Optimal
    $hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Utf8 (Join-Path $outputRoot 'SHA256SUMS.txt') "$hash *$name.zip`n"
    Write-Output "Paquete: $zip"
    Write-Output "SHA256: $hash"
} finally {
    $env:PATH = $savedPath
    $env:STL_QT_ROOT = $savedQtRoot
    Pop-Location
}
