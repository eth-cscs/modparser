# modparser

A source to source compiler for the NMODL domain specific language.

## Build and Test

### check out

first, check out the repository
```
git clone git@github.com:eth-cscs/modparser.git
```

There are no external dependencies. The project uses external projects, which are part of the repository.
* **Google Test** is used for the unit testing framework. The files for Google Test are part of the modparser repository, in ```tests/gtest```. BSD license.
* **TCLap (Templatized C++ Command Line Parser)** is used for command line parsing, is stored in `external/tclap`. MIT license.

### build

CMake and a C++11 compliant compiler are all that are required.

```make all``` will build two targets: the ```bin/modcc``` executable for the compiler, and the unit tests ```tests/test_compiler```.

### test

To run the unit tests, simply run ```tests/test_compiler```.

To test the compiler itself, first check that you can get help

```./bin/modcc -help```

There are some mod files in the ```tests/modfiles``` path, that can be used to generate some compiler output. Both an input file name and a target are required:

```
./bin/modcc tests/modfiles/KdShu2007.mod  -t gpu
./bin/modcc tests/modfiles/KdShu2007.mod  -t cpu
```

If no ouput file is specified, as above, the generated code is written to stdout. An output file can be specified using the ```-o``` flag

```
./bin/modcc tests/modfiles/KdShu2007.mod  -t gpu -o KdShu.h
``
