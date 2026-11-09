[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
$archive = Join-Path $repo 'build/tooling/miniz-3.1.0.zip'
$destination = Join-Path $repo 'build/deps/miniz-3.1.0'
$hash = '89ce8fc6012b1f48407bb1422dd75701bb639e00fc235090cd8b47283fe6197b'
New-Item -ItemType Directory -Force (Split-Path $archive) | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $partial = "$archive.partial-$([guid]::NewGuid().ToString('N'))"
    Invoke-WebRequest -UseBasicParsing -Uri 'https://codeload.github.com/richgel999/miniz/zip/refs/tags/3.1.0' -OutFile $partial -TimeoutSec 240
    if ((Get-FileHash -LiteralPath $partial).Hash.ToLowerInvariant() -ne $hash -or (Get-Item -LiteralPath $partial).Length -ne 306766) { throw 'Fuente miniz no coincide con la version fijada.' }
    Move-Item -LiteralPath $partial -Destination $archive
}
if ((Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant() -ne $hash) { throw 'SHA256 miniz incorrecto.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($name in @('LICENSE','miniz.c','miniz.h','miniz_common.h','miniz_tinfl.c','miniz_tinfl.h','miniz_tdef.h','miniz_zip.c','miniz_zip.h')) {
        $entry = $zip.GetEntry("miniz-3.1.0/$name")
        if (-not $entry) { throw "Falta fuente miniz: $name" }
        $file = Join-Path $destination $name
        New-Item -ItemType Directory -Force $destination | Out-Null
        if (-not (Test-Path -LiteralPath $file)) { [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$file,$false) }
        $stream = $entry.Open(); $sha = [Security.Cryptography.SHA256]::Create()
        try { $expected = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
        finally { $stream.Dispose(); $sha.Dispose() }
        if ((Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant() -ne $expected) { throw "Fuente miniz modificada: $name" }
    }
} finally { $zip.Dispose() }
Write-Output 'miniz 3.1.0 verificado; commit 174573d60290f447c13a2b1b3405de2b96e27d6c. Solo fuentes de lectura; sin tests ni instalacion global.'
