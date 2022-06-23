@echo off
pushd .\out\CMAKE\
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
popd