
set path=%path%;c:\cygwin64\bin
set prefix="C:\GridPACK"
set prefix="C:\Users\D3G096\gridpack"

set CFLAGS="/D _ITERATOR_DEBUG_LEVEL=0"
set CXXFLAGS="/D _ITERATOR_DEBUG_LEVEL=0"

del /Q CMakeCache.txt CMakeFiles

cmake -Wdev --debug-trycompile ^
    -G "Visual Studio 14 2015 Win64" ^
    -D USE_PROGRESS_RANKS:BOOL=OFF ^
    -D BOOST_ROOT:PATH=%prefix% ^
    -D Boost_USE_STATIC_LIBS:BOOL=ON ^
    -D BOOST_INCLUDEDIR=%prefix%\include\boost-1_68 ^
    -D PETSC_DIR="%prefix%\src\petsc-3.10.5" ^
    -D PETSC_ARCH="mswin-c++-real" ^
    -D GA_DIR:PATH="%prefix%" ^
    -D PARMETIS_DIR:PATH="%prefix%" ^
    -D PARMETIS_INCLUDE_DIR:PATH="%prefix%\include" ^
    -D PARMETIS_LIB_DIR:PATH="%prefix%\lib" ^
    -D MPIEXEC_MAX_NUMPROCS:STRING="2" ^
    -D GRIDPACK_TEST_TIMEOUT:STRING=20 ^
    ..

  
