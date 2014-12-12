## modcc

Compiler the NMODL domain specific language used in Neuron.

## aims

The NMODL DSL is used by Neuron to describe the processes in a single mechanism. The current compiler in Neuron is based on Bison, and generates C code.

The C code that is not human readable, and the process used to generate it has reached the limits of its design, so that extending it would be very challenging.

This project aims to improve the parsing of the NMODL language, to make it simple to generate 

There are three main components in the Compiler front end:
- lexer
- parser
- semantic analysis

Robust handling of errors is a high priority, with errors emitted at all compilation stages.
- there is no attempt to recover when the lexer or parser encounter errors. The error message is printed, and the compiler exits.
- during semantic analysis errors are marked in the AST, and analysis continues for the entire application. All errors are then printed by a Visitor that walks the AST.

## running

The project should be straightforward to compile using the provided CMake script. A compiler that supports C++11 is required, and CMake 2.8 or later.

There are two targets
- `unittests.exe` which runs the unit tests.
- `modcc` which is the stand alone compiler.

### unittests.exe

The unit tests are implemented using Google test, which is included in the repository (no separate compilation required).

### unittests.exe

Currently it only parses and performs semantic analysis on an input file. Sample input files can be found in `modfiles/*.mod`.

An example of successful compilation is
```
> ./modcc modfiles/KdShu2007.mod 
[parsing]
[semantic analysis]
```

To get more verbose output of the symbol table and AST of functions and procedures, uncomment the `#defined VERBOSE` in `modcc.cpp`.

An example of errors encountered in compilation is
```
> ./modcc modfiles/test.mod 
[parsing]
[semantic analysis]
error: modfiles/test.mod:(line 79,col 5) 
  the symbol 'okcinf' is a function/procedure, not a variable
error: modfiles/test.mod:(line 46,col 5) 
  semantic() has not been implemented for this expression

there were 2 errors in the semantic analysis
```
Note that for both of these errors the code is valid, and we need further semantic analysis rules to handle them.

## colour output

The compiler makes use of gratuitous colour output, which in the opinion of the author makes it easier to understand. If you disagree, comment out `#define COLOR_PRINTING` in `util.h`
