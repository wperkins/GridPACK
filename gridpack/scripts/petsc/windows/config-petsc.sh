prefix="/cygdrive/c/Users/D3G096/gridpack"
export PETSC_DIR="/cygdrive/c/Users/D3G096/gridpack/src/petsc-3.8.4"
# export PETSC_DIR="/cygdrive/c/Users/D3G096/gridpack/src/petsc-3.10.5"
python ./configure \
    PETSC_ARCH=mswin-c++-real \
    --with-cc="win32fe cl" --with-fc=0 --with-cxx="win32fe cl" \
    --with-clanguage=c++ \
    --with-cxx-dialect=C++11 \
    --with-c++-support=1 \
    --with-precision=double \
    --with-scalar-type=real \
    --with-shared-libraries=0 \
    --with-debugging=0 \
    --with-mpi-include='[/cygdrive/c/PROGRA~2/MIA713~1/MPI/Include,/cygdrive/c/PROGRA~2/MIA713~1/MPI/Include/x64]' \
    --with-mpi-include=/cygdrive/c/Program\ Files\ \(x86\)/Microsoft\ SDKs/MPI/Include \
    --with-mpi-lib=['/cygdrive/c/Program\ Files\ \(x86\)/Microsoft\ SDKs/MPI/Lib/x64/msmpi.lib'] \
    --with-mpiexec='/cygdrive/c/Program\ Files/Microsoft\ MPI/Bin/mpiexec.exe' \
    --with-blaslapack-lib=[$prefix/lib/lapack.lib,$prefix/lib/blas.lib,$prefix/lib/libf2c.lib] \
    --with-metis=1 \
    --with-metis-include=${prefix}/include \
    --with-metis-lib=[${prefix}/lib/metis.lib] \
    --with-parmetis=1 \
    --with-parmetis-include=${prefix}/include \
    --with-parmetis-lib=[${prefix}/lib/parmetis.lib] \
    --with-suitesparse=1 \
    --with-suitesparse-include="${prefix}/include/suitesparse" \
    --with-suitesparse-lib=[${prefix}/lib/libumfpack.lib,${prefix}/lib/libamd.lib,${prefix}/lib/libbtf.lib,${prefix}/lib/libcamd.lib,${prefix}/lib/libccolamd.lib,${prefix}/lib/libcolamd.lib,${prefix}/lib/libcholmod.lib,${prefix}/lib/libcxsparse.lib,${prefix}/lib/libklu.lib,${prefix}/lib/libspqr.lib,${prefix}/lib/libldl.lib,${prefix}/lib/suitesparseconfig.lib] \
    --with-superlu_dist=1 \
    --with-superlu_dist-include="${prefix}/include" \
    --with-superlu_dist-lib="${prefix}/lib/superlu_dist.lib" \
    CFLAGS="-O2 -MD -wd4996" \
    CXXFLAGS="-O2 -MD -wd4996 -EHsc"

exit

