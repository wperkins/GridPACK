set CFLAGS="/MD"
set CXXFLAGS="/MD"
set prefix="C:\Users\D3G096\gridpack"
cmake ^
     -G "Visual Studio 14 2015 Win64" ^
     -D BUILD_SHARED_LIBS:BOOL=NO ^
     -D METIS_INSTALL:BOOL=YES ^
     -D CMAKE_INSTALL_PREFIX:PATH=%prefix% ^
     ..
 cmake --build . --config Release
 cmake --build . --target install --config Release
