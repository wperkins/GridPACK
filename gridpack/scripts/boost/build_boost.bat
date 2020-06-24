REM Probably needs to be done as administrator
REM set msmpi
REM bootstrap.bat
REM Add "using mpi ;" to project-config.jam
.\b2 ^
   --prefix="C:\Users\D3G096\gridpack" ^
   --with-mpi ^
   --with-serialization ^
   --with-random ^
   --with-filesystem ^
   --with-system ^
   --build-type=complete ^
   threading=single ^
   address-model=64 ^
   link=static runtime-link=shared ^
   install