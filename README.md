# Source Repo for Orbit Compiler 

## Synopsis

This repository contains the companion compiler/analyzer for the orbit abstractions of 
the Obi-wan project. The tool analyzes a user-level program and instruments
the program to optimize the usability and performance of orbit tasks for developers.
The compiler is built on top of the LLVM framework.

## Requirement

* LLVM 5.0.1

## Build


### Compile the tool

```
mkdir build
cd build
cmake ..
make -j4
```

### Compile test programs

We include a set of simple test programs for testing the orbit compiler during 
development. For them to be used by the tool, we need to compile these test 
programs into bitcode files using `clang`, e.g.,

```
clang -c -emit-llvm test/loop1.c -o test/loop1.bc
```

For convenience, you can compile the bitcode files for all test programs with:
```
cd test
make
```

### Run the analysis on the test programs

Invoke the analysis either using the LLVM `opt` tool with the analysis library in 
the `lib` directory or using the executable in the `tools` directory.

```
cd build
opt -load lib/libLLVMDefUse.so -mydefuse < ../test/loop1.bc > /dev/nul
```

### Compile a large system for analysis

For large software, we need to modify its build system to use clang for compilation.
The most systematic way is using the [WLLVM](https://github.com/travitch/whole-program-llvm) 
wrapper.

Example instructions to be added.

## Usage

TBA


## Code Style

### Command-Line
We generally follow the [Google Cpp Style Guide](https://google.github.io/styleguide/cppguide.html#Formatting). 
There is a [.clang-format](.clang-format) file in the root directory that is derived from this style.
It can be used with the `clang-format` tool to reformat the source files, e.g.,

```
$ clang-format -style=file lib/DefUse/DefUse.cpp
```

This will use the `.clang-format` file to re-format the source file and print it to the console. 
To re-format the file in-place, add the `-i` flag.

```
$ clang-format -i -style=file lib/DefUse/DefUse.cpp
$ clang-format -i -style=file lib/*/*.cpp
```

### Make target
We defined a make target in the CMakeFiles to run `clang-format` on all source
files or only those source files that are changed.

```
make format
```

### IDE
If you are using Clion, the IDE supports `.clang-format` style. Go to `Settings/Preferences | Editor | Code Style`, 
check the box `Enable ClangFormat with clangd server`. 

### Vim
`clang-format` can also be integrated with vim [doc](http://clang.llvm.org/docs/ClangFormat.html#vim-integration).
