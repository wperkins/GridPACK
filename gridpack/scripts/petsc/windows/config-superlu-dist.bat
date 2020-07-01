
# clone SuperLU_DIST from https://github.com/xiaoyeli/superlu_dist
# Checkout revision tagged "v6.1.1" (used by PETSc 3.10.5)
# This comes from config/BuildSystem/config/packages/SuperLU_DIST.py

REM set GridPACKDir="C:/Users/D3G096/gridpack"
set CFLAGS=/DWIN32 /D_WINDOWS /W3 /D_CRT_SECURE_NO_WARNINGS
set CXXFLAGS=/DWIN32 /D_WINDOWS /W3 /D_CRT_SECURE_NO_WARNINGS

del CMakeCache.txt

cmake -Wdev ^
     -G "Visual Studio 14 2015 Win64" ^
     -D CMAKE_INSTALL_PREFIX:PATH="%GridPACKDir%" ^
     -D CMAKE_BUILD_TYPE:STRING=Release ^
     -D enable_double:BOOL=ON ^
     -D enable_openmp:BOOL=OFF ^
     -D enable_tests:BOOL=OFF ^
     -D enable_examples:BOOL=OFF ^
     -D USE_XSDK_DEFAULTS:BOOL=TRUE ^
     -D XSDK_ENABLE_Fortran:BOOL=FALSE ^
     -D BUILD_SHARED_LIBS:BOOL=FALSE ^
     -D TPL_ENABLE_BLASLIB:BOOL=OFF ^
     -D TPL_ENABLE_COMBBLASLIB:BOOL=OFF ^
     -D BLAS_LIBRARIES:STRING="%GridPACKDir%/lib/blas.lib;%GridPACKDir%/lib/libf2c.lib" ^
     -D LAPACK_LIBRARIES:STRING="%GridPACKDir%/lib/lapack.lib" ^
     -D TPL_PARMETIS_INCLUDE_DIRS:STRING="%GridPACKDir%/include" ^
     -D TPL_PARMETIS_LIBRARIES:STRING="%GridPACKDir%/lib/parmetis.lib;%GridPACKDir%/lib/metis.lib" ^
     ..
REM cmake --build . --config Release
REM cmake --build . --config Release --target install
