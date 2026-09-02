@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$process = Start-Process powershell.exe -Verb RunAs -Wait -PassThru -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""%~dp0tools\deploy\Deploy-FixPack.ps1"" -Interactive'; exit $process.ExitCode"
exit /b %errorlevel%
