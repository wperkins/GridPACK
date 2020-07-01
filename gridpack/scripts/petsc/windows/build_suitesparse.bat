set GridPACKDir="C:/Users/D3G096/gridpack"
set CFLAGS="/MD"
set CXXFLAGS="/MD"
cmake -Wdev ^
     -G "Visual Studio 14 2015 Win64" ^
     -D BUILD_METIS:BOOL=NO ^
     -D LAPACK_DIR:PATH="%GridPACKDir%" ^
     -D BLAS_DIR:PATH="%GridPACKDir%" ^
     -D LAPACK_LIBRARIES:STRING="%GridPACKDir%/lib/lapack.lib" ^
     -D BLAS_LIBRARIES:STRING="%GridPACKDir%/lib/blas.lib;%GridPACKDir%/lib/libf2c.lib" ^
     -D CMAKE_INSTALL_PREFIX:PATH="%GridPACKDir%" ^
     ..
 cmake --build . --config Release
 cmake --build . --target install --config Release