@echo off
setlocal
if /i "%VSCMD_ARG_TGT_ARCH%"=="x86" goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
 echo Install Visual Studio C++ x86 build tools and Windows SDK, or use an x86 Native Tools prompt.
 exit /b 1
)
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "BROWSER_VS=%%I"
if not defined BROWSER_VS exit /b 1
call "%BROWSER_VS%\VC\Auxiliary\Build\vcvars32.bat"
if errorlevel 1 exit /b 1
:compiler_ready
cd /d "%~dp0"
if not exist build mkdir build
if not exist package\NVSE\Plugins mkdir package\NVSE\Plugins
if not exist package\NVSE\Plugins\LukesItemBrowser mkdir package\NVSE\Plugins\LukesItemBrowser
cl /nologo /LD /MT src\zlibwrap.cpp /Fobuild\zlibwrap.obj /Fepackage\NVSE\Plugins\LukesItemBrowser\zlib1.dll /link third_party\libz.a
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE /LD src\browser.cpp /Fobuild\browser.obj /Fepackage\NVSE\Plugins\LukesItemBrowser.dll /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86 /OUT:package\NVSE\Plugins\LukesItemBrowser.dll
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS src\check.cpp /Fobuild\check.obj /Febuild\CatalogCheck.exe /link user32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\preview.cpp /Fobuild\preview.obj /Febuild\LayoutPreview.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT src\inputprobe.cpp /Fobuild\inputprobe.obj /Febuild\InputProbe.exe /link dinput8.lib dxguid.lib user32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\rendercheck.cpp /Fobuild\rendercheck.obj /Febuild\RenderCheck.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\hookcheck.cpp /Fobuild\hookcheck.obj /Febuild\HookCheck.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\perfcheck.cpp /Fobuild\perfcheck.obj /Febuild\PerfCheck.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\rendermock.cpp /Fobuild\rendermock.obj /Febuild\RenderMock.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\chaincheck.cpp /Fobuild\chaincheck.obj /Febuild\ChainCheck.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /GS /DUNICODE /D_UNICODE src\controllercheck.cpp /Fobuild\controllercheck.obj /Febuild\ControllerCheck.exe /link /DYNAMICBASE /NXCOMPAT d3d9.lib dinput8.lib dxguid.lib user32.lib gdi32.lib /MACHINE:X86
exit /b %errorlevel%




