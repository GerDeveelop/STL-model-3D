[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
$archive = Join-Path $repo 'build/tooling/manifold-3.5.4.zip'
$destination = Join-Path $repo 'build/deps/manifold-3.5.4'
$hash = '5efa97c2cd9673294be10fd6725dfc5bcc43b7fd5dd4a115a6d1b48cad1788bb'
New-Item -ItemType Directory -Force (Split-Path $archive) | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $partial = "$archive.partial-$([guid]::NewGuid().ToString('N'))"
    Invoke-WebRequest -UseBasicParsing -Uri 'https://codeload.github.com/elalish/manifold/zip/refs/tags/v3.5.4' -OutFile $partial -TimeoutSec 240
    if ((Get-FileHash -LiteralPath $partial).Hash.ToLowerInvariant() -ne $hash) { throw 'SHA256 Manifold incorrecto.' }
    Move-Item -LiteralPath $partial -Destination $archive
}
if ((Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant() -ne $hash) { throw 'Fuente Manifold modificada.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($entry in $zip.Entries) {
        # Only production sources and build metadata; do not extract upstream tests/fixtures.
        if ($entry.FullName -notmatch '^manifold-3\.5\.4/(CMakeLists\.txt|LICENSE|AUTHORS|README\.md|bindings/CMakeLists\.txt|(?:cmake|src|include)/.+)$' -or
            $entry.FullName.EndsWith('/')) { continue }
        $file = Join-Path $destination $entry.FullName
        $resolved = [IO.Path]::GetFullPath($file)
        if (-not $resolved.StartsWith(([IO.Path]::GetFullPath($destination) + [IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Ruta inesperada dentro del archivo Manifold.'
        }
        if (-not (Test-Path -LiteralPath $file)) {
            New-Item -ItemType Directory -Force (Split-Path $file) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$file,$false)
        }
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $expected = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
        finally { $stream.Dispose(); $sha.Dispose() }
        if ((Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant() -ne $expected) { throw "Fuente Manifold incompleta o modificada: $($entry.FullName)" }
    }
} finally { $zip.Dispose() }
Write-Output 'Manifold 3.5.4 verificado, commit ce50d78021d64507f89e8c9fc2c2e51018117857. Sin descargas implicitas; pruebas upstream desactivadas.'
