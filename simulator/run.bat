@echo off
rem Baut den Simulator neu und startet ihn. Zusaetzliche Argumente werden
rem durchgereicht, z.B.:  run.bat --ms 6000 --keys dd
setlocal
set "SIM=%~dp0"
call "%SIM%build.bat"
if errorlevel 1 exit /b 1
"%SIM%zza_sim.exe" %*
