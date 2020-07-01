REM set GridPACKDir="C:\Users\D3G096\gridpack"

set CFLAGS="/MD"
set CXXFLAGS="/MD"
cmake -Wdev --debug-trycompile ^
    -D BUILD_SHARED_LIBS:BOOL=OFF ^
    -G "Visual Studio 14 2015 Win64" ^
    -D CMAKE_INSTALL_PREFIX:PATH="%GridPACKDir%" ^
    ..
cmake --build . --config Release
cmake --build . --config Release --target install
