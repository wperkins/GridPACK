# GridPACK

## Description

GridPACK is a software framework consisting of a set of modules
designed to simplify the development of programs that model the power
grid and run on parallel, high performance computing platforms. The
modules are available as a library and consist of components for
setting up and distributing power grid networks, support for modeling
the behavior of individual buses and branches in the network,
converting the network models to the corresponding algebraic
equations, and parallel routines for manipulating and solving large
algebraic systems. Additional modules support input and output as well
as basic profiling and error management.  

See the [GridPACK home page](https://www.gridpack.org) for more information.

## Cloning

The GridPACK git repository uses submodules, so extra steps are
required to clone:

1. Clone the repository as you would normally
```
git clone -b windoze https://github.com/wperkins/GridPACK.git gridpack
```
2. Then update the submodules in source tree
```
cd gridpack
git submodules init
git submodules update 
```

