@echo off
setlocal

rem Update submodules
git submodule update --init

rem Navigate to the build directory
cd build

rem Run the CMake command
cmake .. -G "Visual Studio 16 2019"

rem Wait for user input before closing the window
pause

endlocal