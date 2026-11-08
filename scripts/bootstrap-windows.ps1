[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot
$qtRoot = Join-Path $repoRoot 'build/qt/6.8.3/msvc2022_64'
$tools = Join-Path $repoRoot 'build/tooling'
$archiveName = '6.8.3-0-202503201308qtbase-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z'
$url = "https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/$archiveName"
$expectedHash = '41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f'
$archive = Join-Path $tools $archiveName
New-Item -ItemType Directory -Path $tools -Force | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    $partial = "$archive.partial-$([guid]::NewGuid().ToString('N'))"
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $url -OutFile $partial -UseBasicParsing -TimeoutSec 180
    if ((Get-FileHash -LiteralPath $partial -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
        throw "Checksum Qt incorrecto. Se conserva la descarga para diagnostico: $partial"
    }
    Move-Item -LiteralPath $partial -Destination $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash -or
    (Get-Item -LiteralPath $archive).Length -ne 39484220) {
    throw 'El archivo Qt existente no coincide con la version fijada; no se sobrescribe.'
}
$qmake = Join-Path $qtRoot 'bin/qmake.exe'
if (-not (Test-Path -LiteralPath $qmake)) {
    if (Test-Path -LiteralPath $qtRoot) { throw 'La instalacion Qt esta incompleta; no se sobrescribe.' }
    New-Item -ItemType Directory -Path $qtRoot | Out-Null
    $sevenZip = Join-Path $env:ProgramFiles '7-Zip/7z.exe'
    $extractLog = Join-Path $tools 'qt-extract.log'
    if (Test-Path -LiteralPath $sevenZip) {
        & $sevenZip x -y "-o$qtRoot" $archive *> $extractLog
    } else {
        & tar -xf $archive -C $qtRoot *> $extractLog
    }
    if ($LASTEXITCODE -ne 0) {
        $detail = (Get-Content -LiteralPath $extractLog -Tail 12) -join ' '
        throw "No se pudo extraer Qt: $detail"
    }
}
$version = & $qmake -query QT_VERSION
if ($LASTEXITCODE -ne 0 -or $version -ne '6.8.3') { throw 'El kit no es Qt 6.8.3.' }
foreach ($file in @('lib/cmake/Qt6/Qt6Config.cmake','bin/windeployqt.exe','sbom/qtbase-6.8.3.spdx.json')) {
    if (-not (Test-Path -LiteralPath (Join-Path $qtRoot $file))) { throw "Kit Qt incompleto: $file" }
}
$env:STL_QT_ROOT = $qtRoot
if ($env:GITHUB_ENV) { Add-Content -LiteralPath $env:GITHUB_ENV -Value "STL_QT_ROOT=$qtRoot" -Encoding utf8 }
$record = [ordered]@{version='6.8.3';url=$url;bytes=39484220;sha256=$expectedHash;root=$qtRoot}
[IO.File]::WriteAllText((Join-Path $tools 'qt-runtime-source.json'),($record | ConvertTo-Json),
                       (New-Object System.Text.UTF8Encoding($false)))
Write-Output "Qt $version verificado: $qtRoot"
