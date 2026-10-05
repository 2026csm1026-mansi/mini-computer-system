@echo off
echo Building CS527 Lab 5...
mingw32-make clean
mingw32-make
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)
echo Build successful: cs527_lab5.exe
pause
