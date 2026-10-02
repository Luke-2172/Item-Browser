$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$package = Join-Path $root 'package'
$m = Get-Content (Join-Path $package 'MCM/LukesItemBrowser.json') -Raw | ConvertFrom-Json
if ($m.modName -ne 'LukesItemBrowser' -or $m.minMCMVersion -ne 1.63) { throw 'Invalid MCM metadata' }
$target = [IO.Path]::GetFullPath((Join-Path (Join-Path $package 'config') $m.saveFile))
$iniPath = [IO.Path]::GetFullPath((Join-Path $package 'NVSE/Plugins/LukesItemBrowser.ini'))
if ($target -ne $iniPath) { throw 'MCM and native settings paths differ' }
$ini = @{}
$section = ''
foreach ($line in Get-Content $iniPath) {
    if ($line -match '^\[(.+)\]$') { $section = $Matches[1] }
    elseif ($line -match '^([^;=]+)=(.*)$') { $ini["${section}:$($Matches[1])"] = $Matches[2] }
}
$expected = @{
    'Controls:ControllerOpenDirection' = @(1,4,1)
    'Controls:FunctionKey' = @(1,24,11)
    'Controls:MouseSpeed' = @(0.25,5,1.6)
    'Display:BackgroundOpacity' = @(20,100,72)
    'Display:RenderScale' = @(1,3,3)
    'Audio:MenuSounds' = @(0,1,1)
    'Audio:PickupSounds' = @(0,1,1)
    'Browser:ShowOverrides' = @(0,1,0)
}
$seen = @{}
foreach ($property in $m.submenus.'0'.options.PSObject.Properties) {
    $o = $property.Value
    if ($o.type -eq 7) { continue }
    if ($o.enable -ne 1 -or $o.vars.Count -ne 1) { throw 'Invalid option' }
    $v = $o.vars[0]
    $key = $v.configINI
    if (!$expected.ContainsKey($key) -or $seen.ContainsKey($key)) { throw "Unexpected/duplicate setting: $key" }
    $seen[$key] = $true
    if ($v.default -ne $expected[$key][2] -or [double]::Parse($ini[$key],[cultureinfo]::InvariantCulture) -ne $v.default) { throw "Default mismatch: $key" }
    if ($o.type -in @(2,2.5)) {
        if ($o.scale.valueMin -ne $expected[$key][0] -or $o.scale.valueMax -ne $expected[$key][1] -or $o.scale.valueIncrement -le 0) { throw "Range mismatch: $key" }
    } elseif ($o.type -ne 5) { throw "Unsupported option: $key" }
}
if ($seen.Count -ne 8) { throw 'Expected eight settings' }
$json = Get-Content (Join-Path $package 'MCM/LukesItemBrowser.json') -Raw
if ($json -match '"(?:call|callOpen|callClose|callLoop|snippet|console|iniPath)"') { throw 'Unexpected executable callback or path override' }
if (Get-ChildItem $package -Recurse -File | Where-Object Extension -in '.esp','.esm') { throw 'ESP/ESM found in ESP-free package' }
$runner = Get-Content (Join-Path $package 'NVSE/Plugins/scripts/ln_LukesItemBrowser.txt') -Raw
if ($runner -match 'CompileScript\s+"[^"]*MCM|IsModLoaded') { throw 'Optional MCM dependency found in startup runner' }
Write-Output 'PASS: ESP-free MCM metadata, eight shared settings, defaults, ranges and fixed INI path'
