@echo off
rem ---------------------------------------------------------------------------
rem  Baut den ZZA-Simulator mit dem Visual-Studio-Compiler (cl.exe).
rem  Die Spieldateien aus lib/ werden unveraendert mitkompiliert.
rem ---------------------------------------------------------------------------
setlocal enabledelayedexpansion

set "SIM=%~dp0"
set "SIM=%SIM:~0,-1%"
set "GAME=%SIM%\.."
set "BUILD=%SIM%\build"

if not exist "%GAME%\lib\Game\Game.cpp" (
    echo [Fehler] Spielquellen nicht gefunden unter "%GAME%\lib".
    echo          build.bat muss im Ordner simulator\ direkt neben lib\ liegen.
    exit /b 1
)

rem --- Visual-Studio-Umgebung suchen und laden ------------------------------
where cl.exe >nul 2>&1
if not errorlevel 1 goto :compiler_da

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [Fehler] Visual Studio wurde nicht gefunden.
    echo          Benoetigt: "Desktopentwicklung mit C++" ^(Visual Studio 2019/2022^).
    exit /b 1
)

set "VSPATH="
set "VSTMP=%TEMP%\zza_sim_vswhere.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSTMP%" 2>nul
if exist "%VSTMP%" set /p VSPATH=<"%VSTMP%"
del "%VSTMP%" >nul 2>&1
if not defined VSPATH (
    echo [Fehler] Keine C++-Werkzeuge in der Visual-Studio-Installation gefunden.
    exit /b 1
)

echo [Build] Visual Studio: %VSPATH%
rem 2>&1: vcvars64.bat gibt intern harmlose Meldungen auf stderr aus
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [Fehler] vcvars64.bat konnte nicht geladen werden.
    exit /b 1
)

:compiler_da
rem --- Laeuft der Simulator noch? Dann kann die .exe nicht ersetzt werden ----
tasklist /fi "IMAGENAME eq zza_sim.exe" 2>nul | find /i "zza_sim.exe" >nul
if not errorlevel 1 (
    echo.
    echo [Fehler] Der Simulator laeuft noch - bitte das Fenster schliessen.
    echo          Sonst kann der Linker zza_sim.exe nicht ueberschreiben
    echo          ^(Meldung "LNK1104: Datei kann nicht geoeffnet werden"^).
    exit /b 1
)

if not exist "%BUILD%" mkdir "%BUILD%"

rem --- Spielquellen einsammeln ----------------------------------------------
rem  Alle .cpp unter lib/ werden automatisch mitkompiliert - neue Module
rem  (z.B. lib/Sound/Sound.cpp) laufen ohne Aenderung an dieser Datei mit.
rem  Ausgenommen ist nur lib/Display: das spricht echte GPIOs an und wird
rem  durch src/sim_display.cpp ersetzt.
set "QUELLEN="
set "INCLUDES=/I"%SIM%\shim" /I"%SIM%\src" /I"%GAME%\include""

for /d %%d in ("%GAME%\lib\*") do set "INCLUDES=!INCLUDES! /I"%%d""
for /r "%GAME%\lib" %%f in (*.cpp) do call :quelleHinzu "%%f"

echo [Build] Uebersetze...
rem /MT bindet die C-Laufzeit fest ein: die fertige zza_sim.exe laeuft dann auf
rem jedem Windows-Rechner, ohne dass das "Visual C++ Redistributable" noetig ist.
cl /nologo /EHsc /O2 /W3 /std:c++17 /MT /D_CRT_SECURE_NO_WARNINGS /DARDUINO=10819 ^
   /wd4244 /wd4267 /wd4838 /wd4996 ^
   !INCLUDES! ^
   /Fo"%BUILD%\\" /Fd"%BUILD%\\" /Fe"%SIM%\zza_sim.exe" ^
   "%SIM%\src\sim_main.cpp" "%SIM%\src\sim_arduino.cpp" "%SIM%\src\sim_display.cpp" ^
   !QUELLEN! ^
   /link user32.lib gdi32.lib

if errorlevel 1 (
    echo.
    echo [Fehler] Uebersetzen fehlgeschlagen - siehe Meldungen oben.
    if exist "%SIM%\zza_sim.exe" (
        echo [Achtung] zza_sim.exe ist noch vom LETZTEN erfolgreichen Build.
        echo           Startest du sie jetzt, siehst du den alten Stand!
    )
    exit /b 1
)

echo [Build] Fertig: "%SIM%\zza_sim.exe"
exit /b 0

rem --- Unterprogramm: eine Spielquelle aufnehmen oder ueberspringen ----------
:quelleHinzu
rem Ordnername der Datei bestimmen (lib\Display\Display.cpp -> "Display")
for %%p in ("%~dp1.") do set "ORDNER=%%~nxp"
if /i "!ORDNER!"=="Display" (
    echo [Build]   - %~nx1 ^(Hardware, ersetzt durch sim_display.cpp^)
    goto :eof
)
echo [Build]   + %~nx1
set "QUELLEN=!QUELLEN! "%~1""
goto :eof
