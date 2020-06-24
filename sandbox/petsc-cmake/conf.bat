set CFLAGS=/D _ITERATOR_DEBUG_LEVEL=0
set CXXFLAGS=/D _ITERATOR_DEBUG_LEVEL=0
set prefix="C:\Users\D3G096\gridpack"
del CMakeCache.txt
cmake -Wno-dev --debug-trycompile ^
    -G "Visual Studio 14 2015 Win64" ^
    -D CMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDLL ^
    -D CMAKE_BUILD_TYPE=Release ^
    -D PETSC_DIR="%prefix%\src\petsc-3.10.5" ^
    -D PETSC_ARCH="mswin-c++-real" ^
    -D BOOST_ROOT:PATH=%prefix% ^
    -D Boost_USE_STATIC_LIBS:BOOL=ON ^
    -D BOOST_INCLUDEDIR=%prefix%\include\boost-1_68 ^
    ..
REM cmake --build . --config Release
