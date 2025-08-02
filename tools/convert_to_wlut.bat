@echo off
REM Convert PNG palette to WLUT format
REM Usage: convert_to_wlut.bat input.png [output_name]

if "%1"=="" (
    echo Usage: convert_to_wlut.bat input.png [output_name]
    echo.
    echo Converts PNG palette images to Wisp WLUT format
    echo.
    echo Examples:
    echo   convert_to_wlut.bat my_palette.png
    echo   convert_to_wlut.bat my_palette.png custom_name
    echo.
    echo Supported formats auto-detected:
    echo   - 64x64 PNG → LUT_64x64 (8KB)
    echo   - 32x32 PNG → LUT_32x32 (2KB)
    echo   - 16x1 PNG → PAL_16 (32 bytes)
    echo   - 64x1 PNG → PAL_64 (128 bytes)
    echo   - 256x1 PNG → PAL_256 (512 bytes)
    exit /b 1
)

set INPUT_FILE=%1
set OUTPUT_NAME=%2

if not exist "%INPUT_FILE%" (
    echo Error: Input file '%INPUT_FILE%' not found
    exit /b 1
)

echo Converting %INPUT_FILE% to WLUT format...
echo.

if "%OUTPUT_NAME%"=="" (
    py tools\wisp_palette_converter.py "%INPUT_FILE%" --preview
) else (
    py tools\wisp_palette_converter.py "%INPUT_FILE%" -o "%OUTPUT_NAME%" --preview
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Conversion completed successfully!
    echo Files generated:
    if "%OUTPUT_NAME%"=="" (
        for %%F in ("%INPUT_FILE%") do (
            echo   - %%~nF_data.h ^(C header^)
            echo   - %%~nF.wlut ^(binary format^)
        )
    ) else (
        echo   - %OUTPUT_NAME%_data.h ^(C header^)
        echo   - %OUTPUT_NAME%.wlut ^(binary format^)
    )
    echo.
    echo Use the WLUT verifier to check the format:
    if "%OUTPUT_NAME%"=="" (
        for %%F in ("%INPUT_FILE%") do echo   py tools\wlut_verifier.py %%~nF.wlut
    ) else (
        echo   py tools\wlut_verifier.py %OUTPUT_NAME%.wlut
    )
) else (
    echo.
    echo Conversion failed!
    exit /b 1
)
