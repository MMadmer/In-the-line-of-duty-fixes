@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process powershell.exe -Verb RunAs -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""%~dp0tools\deploy\Deploy-FixPack.ps1"" -Interactive'"
exit /b %errorlevel%
