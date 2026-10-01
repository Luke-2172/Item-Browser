# Luke's Item Browser 1.0

Coded with GPT-6 Astra

An ESP-free Fallout: New Vegas item browser with plugin, item and detail columns,
category filters, separate item/plugin searches and inventory addition through JIP LN.
F11 opens the browser by default. Open/close cues are `ui_vats_move` / `ui_vats_ready`.

## Install

Players should install `Lukes-Item-Browser-FNV-1.0.zip` with Mod Organizer 2.
The archive root belongs in `Data`: it contains `NVSE` and `MCM`.
See [player instructions](package/README.txt) for controls and limitations.
Requires New Vegas 1.4.0.525, xNVSE 6.3.5+ and JIP LN 56.95+.

The optional settings page requires [MCM](https://www.nexusmods.com/newvegas/mods/42507)
and [MCM Extender 1.63+](https://github.com/Stentorious/MCMExtender), with their
dependencies (including JohnnyGuitar NVSE, ShowOff xNVSE and UIO).
Frameworks are installed separately. The browser works without either MCM framework.
The JSON integration uses no ESP, dummy form or load-order slot.

Replace earlier browser installations; disable/remove the old `LukesItemBrowserMCM.esp`.
Do not merge the old MCM registration scripts into this version.

## Build and package

Windows with Visual Studio 2019+ C++ x86 build tools and a Windows SDK is required.
`build.cmd` discovers Visual Studio through `vswhere`, or uses an active x86 Native
Tools prompt. This is a 32-bit plugin, including on 64-bit Windows.
PowerShell 7 is required for the generated compressed-record fixtures.

```powershell
.\build.cmd
pwsh -NoProfile -File .\test.ps1
pwsh -NoProfile -File .\package.ps1
```

The native outputs are generated under `package/NVSE/Plugins`; developer executables
are generated under `build`. Packaging writes `dist/Lukes-Item-Browser-FNV-1.0.zip`.
No game assets or installed game directory are required to build or run these tests.
Upload the contents of this source folder to GitHub. Generated files are ignored.

## Source layout

- `src/browser.cpp`: xNVSE entry point, D3D9/DirectInput hooks, UI and shared settings.
- `src/catalog.h`: bounded TES4 record parser and search.
- `src/importhook.h`: import interception helpers.
- `src/zlibwrap.cpp`: compression-library wrapper.
- Other files under `src`: standalone parser, render, layout and input test tools.
- `package/NVSE`: INI and JIP script bridge assets used by the compiled DLL.
- `package/MCM/LukesItemBrowser.json`: optional MCM Extender settings definition.
- `third_party`: x86 static zlib dependency and its license.

## Settings integration

MCM Extender reads `Data/MCM/LukesItemBrowser.json`. Its `saveFile` is relative to
`Data/config`, so `../NVSE/Plugins/LukesItemBrowser.ini` reaches the same fixed file
as the native renderer. No command callbacks or extra script compilation are used.
Extender flushes its INI cache when leaving the pause menu. The browser polls
preferences every second. Text resolution and font changes require a restart.

`Controls:FunctionKey` stores 1-24 (F1-F24). Old Windows virtual-key `Hotkey` values
are supported only when `FunctionKey` is absent. When upgrading a custom INI,
replace `Hotkey=N` with `FunctionKey=N-111` before using MCM.

The JSON schema was checked against the author's documentation and installed 1.63
scripts. In-game MCM display, cache flushing and save/reopen behavior still require
a gameplay test; automated tests do not emulate the GECK runtime.

## Validation and security

`test.ps1` covers MCM paths/defaults/ranges, parser fixtures and malicious records,
hook chaining, input handling, request serialization and numeric configuration bounds.
Rendering hooks use mock COM objects in the automated harness. Actual GPU rendering,
device resets, overlay compatibility and inventory operations require in-game testing.
See [review scope and limits](package/SECURITY-REVIEW.txt).

## Third-party material

`third_party/libz.a` is the existing MSYS2 x86 static zlib build dependency.
Its required license is included both here and with the generated runtime wrapper.
The wrapper is modified integration code, not an unmodified upstream zlib DLL.
No Bethesda assets or MCM framework source is redistributed.
No project-wide open-source license has been selected; the zlib license applies
only to the third-party zlib material.
