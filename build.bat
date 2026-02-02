@echo off
REM Build script for ArcDPS Killstreak Plugin
REM Requires Visual Studio Build Tools or Visual Studio with C++ workload

echo Building ArcDPS Killstreak Plugin...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake (Visual Studio 2022, adjust if using different version)
cmake -G "Visual Studio 17 2022" -A x64 ..

REM Build Release configuration
cmake --build . --config Release

REM Copy the DLL to a convenient location
if exist Release\arcdps_killstreak.dll (
    copy Release\arcdps_killstreak.dll ..\arcdps_killstreak.dll
    echo.
    echo Build successful!
    echo Output: arcdps_killstreak.dll
    echo.
    echo Installation:
    echo   Copy arcdps_killstreak.dll to your Guild Wars 2 bin64 folder
    echo   (same folder as arcdps d3d11.dll)
) else (
    echo.
    echo Build failed! Check the error messages above.
)

cd ..
pause
