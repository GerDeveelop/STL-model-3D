[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
$archive = Join-Path $repo 'build/tooling/lib3mf_sdk_v2.5.0.zip'
$root = Join-Path $repo 'build/deps/lib3mf-2.5.0'
$hash = 'd04290d1309f775501d21c50858614cdf75c36fa0cbbfad8706e03e3b66b37d9'
New-Item -ItemType Directory -Force (Split-Path $archive) | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $partial = "$archive.partial-$([guid]::NewGuid().ToString('N'))"
    Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/3MFConsortium/lib3mf/releases/download/v2.5.0/lib3mf_sdk_v2.5.0.zip' -OutFile $partial
    if ((Get-FileHash -LiteralPath $partial).Hash.ToLowerInvariant() -ne $hash) { throw 'SHA256 lib3mf incorrecto.' }
    Move-Item -LiteralPath $partial -Destination $archive
}
if ((Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant() -ne $hash) { throw 'SDK lib3mf modificado.' }
# Verify installed SDK against the pinned ZIP, including headers, import library and DLL.
if (-not (Test-Path -LiteralPath $root)) { Expand-Archive -LiteralPath $archive -DestinationPath $root }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($relative in @('Bindings/Cpp/lib3mf_implicit.hpp','Bindings/Cpp/lib3mf_types.hpp',
                            'Bindings/Cpp/lib3mf_abi.hpp','Lib/lib3mf.lib','Bin/lib3mf.dll','License.txt')) {
        $entry = $zip.GetEntry("lib3mf_sdk/$relative")
        if (-not $entry) { throw "SDK incompleto: $relative" }
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $expected = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
        finally { $stream.Dispose(); $sha.Dispose() }
        $file = Join-Path $root "lib3mf_sdk/$relative"
        if (-not (Test-Path -LiteralPath $file) -or (Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant() -ne $expected) {
            throw "SDK instalado incompleto o modificado: $relative"
        }
    }
} finally { $zip.Dispose() }
Write-Output 'lib3mf 2.5.0 SDK verificado (BSD-2-Clause y avisos transitivos en License.txt).'
