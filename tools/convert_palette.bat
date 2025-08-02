@echo off
REM Wisp Palette Converter - Windows Batch Helper
REM Usage: convert_palette.bat [input.png] [format]

cd /d "%~dp0"

if "%1"=="" (
    echo Wisp Palette Converter
    echo.
    echo Usage: convert_palette.bat input.png [format]
    echo.
    echo Formats:
    echo   auto   - Auto-detect best format (default)
    echo   lut64  - 64x64 color LUT (8KB)
    echo   lut32  - 32x32 color LUT (2KB)  
    echo   pal16  - 16 color palette (32 bytes)
    echo   pal64  - 64 color palette (128 bytes)
    echo   pal256 - 256 color palette (512 bytes)
    echo.
    echo Examples:
    echo   convert_palette.bat lut_palette.png
    echo   convert_palette.bat game_colors.png pal16
    echo   convert_palette.bat gradient.png lut64
    echo.
    pause
    exit /b 1
)

set INPUT_FILE=%1
set FORMAT=%2

if "%FORMAT%"=="" set FORMAT=auto

echo Converting %INPUT_FILE% to Wisp palette format...
echo Format: %FORMAT%
echo.

REM Check if Python is available
py --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found! Please install Python 3.x
    echo Download from: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM Check if input file exists
if not exist "%INPUT_FILE%" (
    echo Error: Input file "%INPUT_FILE%" not found!
    pause
    exit /b 1
)

REM Run the converter
if "%FORMAT%"=="auto" (
    py tools\wisp_palette_converter.py "%INPUT_FILE%" --preview
) else (
    py tools\wisp_palette_converter.py "%INPUT_FILE%" -f %FORMAT% --preview
)

if errorlevel 1 (
    echo.
    echo Conversion failed! Check error messages above.
    echo.
    echo Common issues:
    echo - Missing dependencies: pip install Pillow numpy
    echo - Invalid PNG file
    echo - Too many colors for selected format
    pause
    exit /b 1
)

echo.
echo Conversion completed successfully!
echo Files generated in current directory.
echo.
pause
