# Benchmark test for mod2c

## Step 1 : compile the compiler!

You first have to generate the mechanisms for the benchmark. These have to be compiled using the mod2c compiler, which you have to build first using the modparser project:

git@github.com:eth-cscs/modparser.git

Follow the build instructions in the README for the mod2c compiler

## Step 2 : generate the mechanism headers

You will need to have the path with the modcc executable (generated in the ```modparser/bin``` path when you compiled modparser) in your PATH:

```
export PATH=<path to modcc bin>:$PATH
```

Then you can use the generate script to generate the headers:

```
cd mod2c-perf/modfiles
./generate.sh
```

This will generate optimized headers for each of the C++, CUDA and Cyme back ends in ```mod2c-perf/mechanisms```. Have a look at them to see what the differences/simularities between the backends are.

## Step 3 : update the git submodules

The benchmark framework uses some external git repositories from github.
- The vector library provides some containers that:
    - are aware of different memory spaces (host and device for CUDA)
    - automagically handle alignment of allocated memory
- The cudastreams library provides some abstractions that make it easy to handle cuda streams and events (used for benchmarking the cuda backend)

To get the repositories:

```
git submodule init
git submodule update
```

**note** to use the cyme backend, you might have to go to the external/vector path and swap to the experimental ```cyme``` branch.

A modified version of cyme is distributed in the externals path.

## step 4: compile

The test and benchmark driver are in the ```src``` path

```
cd mod2c-perf/src
```

Things get a bit messy here because there is no CMake (CMake insists on inserting unwanted flags for some compilers like intel, which do not play nice with the cray compiler wrappers).

The easiest way to build is to make the gnu version. Make sure that you have a version of gcc that supports c++11. Have a look in the makefile.gnu, and modify the flags for the target system (for these tests I use a 12-core Haswell with ```-O3 -march=core-avx2```.

The makefile will make two targets
- ```test.gnu``` some basic unit tests. These need to be fleshed out in ```test.cpp```
- ```perf.gnu``` the performance benchmark, see ```driver.cpp```.

Running the benchmark on one socket of Haswell gives the following output:

```
> OMP_NUM_THREADS=12 aprun -cc numa_node ./perf.gnu
OpenMP     : on   (12 threads)
Target     : cpu
group size : 47516 nodes, 112 cells
cell  size : 424 nodes (mean)
memory     : 522 kB/cell
  ion         3.0 MB
  matrix      2.0 MB
  mech       52.1 MB
    Ca_HVA             0 %
    Im                 4 %
    Ih                 6 %
    NaTs2_t            5 %
    ProbAMPANMDA_EMS  60 %
    ProbGABAAB_EMS    25 %
  total      57.1 MB
-------------------------------
Mechanism             time
-------------------------------
Ca_HVA                0.001187
Im                    0.037712
Ih                    0.062669
NaTs2_t               0.062624
ProbAMPANMDA_EMS      0.246873
ProbGABAAB_EMS        0.086696
matrix.solve          0.011348
matrix.other          0.009008
-------------------------------
mech.total            0.497761
matrix.total          0.0203563
Total                 0.518118  4323
Total_nosolve         0.50677   4420
-------------------------------
```

The table of times starts with the time spent in each mechanism (the time in the state, current, init and jacob kernels is accumulated for each mechanism). Note that the ```ProbAMPANMDA_EMS``` and ```ProbGABAAB_EMS``` mechanisms use an older version of the mod file which does not include the optimizations James implemented to minimise time spent in the integration step (we have to teach the compiler to make those optimizations for itself).

Total time spent in mechanism and matrix updates is also reported, along with the total time. The number 4323 reported after the Total is the number of cell steps per second (number of time steps per second multiplied by the total number of cells), which is a measure of _throughput_.

**note** we use 112 cells in the input data set. When using the OpenMP implementation this means that there will be 112 cells on each thread, for a total of 12x112 cells with 12 threads. The number of cells can be specified as a command line argument, which must be a multiple of 112 (basically the same cell group of 112 cells is replicated N times per thread).

### CYME back end
Building the CYME back end is a bit more involved, because it requires Boost. To get best performance you also need to use the Intel compiler.
- **pro tip** the Intel C++ compiler piggy backs on the gcc standard library, so you have to ensure that it can find a C++11 compliant version of the standard library headers (but not gcc 4.9.x, which is too recent).

You will have to play around with your systems settings, but the following works for me on a Cray system:
```
icpc -std=c++11 -O3 -fopenmp -march=core-avx2 -DWITH_CYME -D__CYME_SIMD_VALUE__=avx ... -isystem /apps/brisi/boost/1.56.0/gnu_482/include ... -DPROFILING_CHRONO -pthread
```

When I run the ```perf.cyme``` executable I get a significant speedup over the original gnu version (not that the matrix updates are now slower, but the mechanisms are computed much faster):
```
> OMP_NUM_THREADS=12 aprun -cc numa_node ./perf.cyme
    ... snip ...
-------------------------------
Mechanism             time
-------------------------------
Ca_HVA                0.001350
Im                    0.021363
Ih                    0.030578
NaTs2_t               0.030209
ProbAMPANMDA_EMS      0.142819
ProbGABAAB_EMS        0.055810
matrix.solve          0.012956
matrix.other          0.022073
-------------------------------
mech.total            0.282128
matrix.total          0.0350292
Total                 0.317157  7062
Total_nosolve         0.304201  7363
-------------------------------
```

### CUDA back end

The CUDA back end is left as an exercise for the reader. Note that the linear system solution is very slow, because it is performed sub-optimally on the GPU. To solve it efficiently, we have to use an AoSoA layout for the matrix (***not*** for the mechanisms, they must still be SoA), or transfer the matrices to the host for solution.

