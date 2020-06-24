set prefix="C:/Users/D3G096/gridpack"
set LAPACK_DIR="%prefix%"
cmake -Wdev ^
     -G "Visual Studio 14 2015 Win64" ^
     -D BUILD_METIS:BOOL=NO ^
     -D LAPACK_DIR:BOOL=ON ^
     -D LAPACK_DIR:PATH="%prefix%" ^
     -D BLAS_DIR:PATH="%prefix%" ^
     -D LAPACK_LIBRARIES:STRING="%prefix%/lib/liblapack.lib" ^
     -D BLAS_LIBRARIES:STRING="%prefix%/lib/libblas.lib" ^
     -D CMAKE_INSTALL_PREFIX:PATH="%prefix%" ^
     ..
 cmake --build . --config Release
 cmake --build . --target install --config Release