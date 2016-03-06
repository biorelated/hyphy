HyPhy - Hypothesis testing using Phylogenies
============================================

[![Build Status](https://travis-ci.org/veg/hyphy.svg)](https://travis-ci.org/veg/hyphy)

Introduction
------------
HyPhy is an open-source software package for the analysis of genetic sequences using techniques in phylogenetics, molecular evolution, and machine learning. It features a complete graphical user interface (GUI) and a rich scripting language for limitless customization of analyses. Additionally, HyPhy features support for parallel computing environments (via message passing interface (MPI)) and it can be compiled as a shared library and called from other programming environments such as Python and R. 

Installation
------------

Hyphy's build system depends on CMake version 3.0 and above. It is also important to note that Hyphy is dependent on other development libraries for example
`libcurl` and `libpthread`. `Libcurl` requires development libraries such as `crypto++` and `openssl` ( or gnutls depending on your configuration). Ubuntu provides `libcurl-dev`, `libcrypto++-dev` and `libssl-dev`.

To install, first clone this repository 

`git clone git@github.com:veg/hyphy.git`

or download a specific release from here https://github.com/veg/hyphy/releases

Change your directory to the newly cloned or source directory

`cd hyphy`

Configure the project using CMake.

`cmake .`

cmake supports other build systems for example Xcode. To use a specific build system,
run cmake with the -G switch

`cmake -G Xcode .`

CMake supports a number of build system generators,
feel free to peruse these and use them if you wish.

By default, HyPhy installs into `/usr/local`. Use the `DINSTALL_PREFIX` to specify an installation location.

`cmake -DINSTALL_PREFIX=/location/of/choice`

To install hyphy at /opt/hyphy, run the following commands

`mkdir -p /opt/hyphy`

`cmake -DINSTALL_PREFIX=/opt/hyphy .`

You can specify which OS X SDK to use if you are on an OS X platform using the following command

`cmake -DCMAKE_OSX_SYSROOT=/Developer/SDKs/MacOSX10.9.sdk/ .`

If you're on a UNIX-compatible system, and you're comfortable with GNU make,
then run `make` with one of the following build targets:

+   `MAC` - build a Mac Carbon application
+   `HYPHYGTK` - HYPHY with GTK
+   `SP` - build a HyPhy executable (HYPHYSP) without multiprocessing
+   `MP2` - build a HyPhy executable (HYPHYMP) using pthreads to do multiprocessing
+   `MPI` - build a HyPhy executable (HYPHYMPI) using MPI to do multiprocessing
+   `HYPHYMPI` - build a HyPhy executable (HYPHYMPI) using openMPI 
+   `LIB` - build a HyPhy library (libhyphy_mp) using pthreads to do multiprocessing
-   `GTEST` - build HyPhy's gtest testing executable (HYPHYGTEST)

For example to create a MPI build of HYPHY using openMPI ensure that you 
have openmpi installed and available on your  path. You can check if this
is the case after running 
`cmake .` you should see something similar to this in your output

`-- Found MPI_C:` 

`-- Found MPI_CXX:`

To compile the MPI build of HyPhy, run

`make HYPHYMPI`

And to install the software,run

`make install`

Depending on your build parameters and configuration , 

+   HYPHYMP(I) will be installed at  `/location/of/choice/bin`
+   libhyphy_mp.(so/dylib/dll) will be installed at `/location/of/choice/lib`
+   HyPhy's standard library of batchfiles will go into `/location/of/choice/lib/hyphy`


Running tests
------
To test HyPhy, build with the GTEST target and run ./HYPHYGTEST from the source directory.
For example 

`make GTEST`

`./HYPHYGTEST`
