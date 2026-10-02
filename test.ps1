$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
& (Join-Path $PSScriptRoot 'test-mcm.ps1')
& (Join-Path $PSScriptRoot 'test-security.ps1')
$deps = Join-Path $PSScriptRoot 'build/Data/NVSE/Plugins/LukesItemBrowser'
New-Item -ItemType Directory -Force $deps | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'package/NVSE/Plugins/LukesItemBrowser/zlib1.dll') $deps -Force
Copy-Item (Join-Path $PSScriptRoot 'package/NVSE/Plugins/LukesItemBrowser/Fonts') $deps -Recurse -Force
& (Join-Path $PSScriptRoot 'build/HookCheck.exe')
if ($LASTEXITCODE -ne 0) { throw "HookCheck failed: $LASTEXITCODE" }

& (Join-Path $PSScriptRoot 'build/PerfCheck.exe')
if ($LASTEXITCODE -ne 0) { throw "PerfCheck failed: $LASTEXITCODE" }
& (Join-Path $PSScriptRoot 'build/RenderMock.exe')
if ($LASTEXITCODE -ne 0) { throw "RenderMock failed: $LASTEXITCODE" }

Write-Output 'PASS: all automated checks (in-game MCM and GPU tests remain manual)'

& (Join-Path $PSScriptRoot 'build/ChainCheck.exe')
if ($LASTEXITCODE -ne 0) { throw "ChainCheck failed: $LASTEXITCODE" }

foreach($file in @('BarlowCondensed-Bold.ttf','BarlowCondensed-OFL.txt','ShareTechMono-Regular.ttf','ShareTechMono-OFL.txt')) {
 if(!(Test-Path (Join-Path $PSScriptRoot "package/NVSE/Plugins/LukesItemBrowser/Fonts/$file"))) { throw "Missing bundled font/license $file" }
}

Push-Location $PSScriptRoot
try {
 & ./build/ControllerCheck.exe
 if($LASTEXITCODE -ne 0){throw "ControllerCheck failed: $LASTEXITCODE"}
} finally {Pop-Location}
