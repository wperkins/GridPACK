REM set GridPACKDir=C:\Users\D3G096\gridpack
cmake -Wdev --debug-trycompile ^
    -D BUILD_SHARED_LIBS:BOOL=OFF ^
    -G "Visual Studio 14 2015 Win64" ^
    -D ENABLE_I8:BOOL=No ^
    -D ENABLE_BLAS:BOOL=No ^
    -D ENABLE_FORTRAN:BOOL=No ^
    -D ENABLE_CXX:BOOL=Yes ^
    -D GA_RUNTIME:STRING=MPI_TS ^
    -D CMAKE_INSTALL_PREFIX:PATH="C:\Users\D3G096\gridpack" ^
    ..
cmake --build . --config Release
cmake --build . --config Release --target install
