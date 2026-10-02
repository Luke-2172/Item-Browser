$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'test-mcm.ps1')
& (Join-Path $PSScriptRoot 'test-security.ps1')
$deps = Join-Path $PSScriptRoot 'build/Data/NVSE/Plugins/LukesItemBrowser'
New-Item -ItemType Directory -Force $deps | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'package/NVSE/Plugins/LukesItemBrowser/zlib1.dll') $deps -Force
& (Join-Path $PSScriptRoot 'build/HookCheck.exe')
if ($LASTEXITCODE -ne 0) { throw "HookCheck failed: $LASTEXITCODE" }

& (Join-Path $PSScriptRoot 'build/PerfCheck.exe')
if ($LASTEXITCODE -ne 0) { throw "PerfCheck failed: $LASTEXITCODE" }
& (Join-Path $PSScriptRoot 'build/RenderMock.exe')
if ($LASTEXITCODE -ne 0) { throw "RenderMock failed: $LASTEXITCODE" }

Write-Output 'PASS: all automated checks (in-game MCM and GPU tests remain manual)'
