set prefix="C:\Users\D3G096\gridpack"
del CMakeCache.txt
cmake -Wno-dev ^
    -G "NMake Makefiles" ^
    -D CMAKE_BUILD_TYPE=Release ^
    -D BOOST_ROOT:PATH=%prefix% ^
    -D Boost_USE_STATIC_LIBS:BOOL=ON ^
    -D BOOST_INCLUDEDIR=%prefix%\include\boost-1_68 ^
    ..
cmake --build . --config Release
