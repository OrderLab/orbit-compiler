# Source Repo for Orbit Compiler 

## Synopsis

This repository contains the companion compiler/analyzer for the orbit abstractions of 
the Obi-wan project. The tool analyzes a user-level program and instruments
the program to optimize the usability and performance of orbit tasks for developers.
The compiler is built on top of the LLVM framework.

## Requirement

* LLVM 5.0.x

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
opt -load lib/libLLVMDefUse.so -mydefuse < ../test/loop1.bc > /dev/null
```

### Compile a large system for analysis

For large software, we need to modify its build system to use clang for compilation.
The most systematic way is using the [WLLVM](https://github.com/travitch/whole-program-llvm) 
wrapper.

```
$ pip install wllvm
```

The basic idea is to replace the regular C/C++ compiler call (e.g., `gcc`/`g++`) 
with `wllvm`/`wllvm++` wrapper call, which will take care of the details for using
LLVM and clang to produce the bitcode.

Large systems usually use Makefile or CMake as the build system. In these cases,
it is fairly simple: define environment variables `CC` (and `CXX`) to be `wllvm` 
(and `wllvm++`), without changing the Makefiles. The compilation will be 
transparent. In the end, invoke the `extract-bc` command on the built
executable, which will produce the bitcode file.

Below is an example of compiling MySQL for analysis.

A. Download MySQL source code:

MySQL - 5.5.59

```
$ mkdir -p target-sys/mysql-build
$ cd target-sys
$ wget -nc https://downloads.mysql.com/archives/mysql-5.5/mysql-5.5.59.tar.gz
$ tar xzvf mysql-5.5.59.tar.gz
```

MySQL - 5.7.31

```
$ mkdir -p target-sys/mysql-build
$ cd target-sys
$ wget -nc https://downloads.mysql.com/archives/mysql-5.7/mysql-5.7.31.tar.gz
$ tar xzvf mysql-5.7.31.tar.gz
```


B. Compile with `wllvm`:

```
$ cd mysql-build
$ CC=wllvm CXX=wllvm++ cmake ../mysql-5.5.59 -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" -DMYSQL_MAINTAINER_MODE=false
$ make -j$(nproc)
$ extract-bc sql/mysqld
```

MySQL - 5.7.31

```
$ cd mysql-build
$ export LLVM_COMPILER=clang
$ CC=wllvm CXX=wllvm++ cmake ../mysql-5.7.31 -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" -DMYSQL_MAINTAINER_MODE=false
$ make -j$(nproc)
$ extract-bc sql/mysqld

```
TODO

You should see a `mysqld.bc` in the `sql` directory (where the normal `mysqld` 
executable resides). This bitcode file will be the target file for analysis 
and instrumentation.

## Usage

### Instrumentation

#### Instrumenting test program

```
$ cd test && make
$ cd ../build
$ ../scripts/instrument-compile.sh --printf ../test/alloc.bc
```

This will instrument all the `malloc` calls in the `alloc.c` test program
and insert a `printf` to output the malloc size and return pointer address.
The resulted instrumented executable is `alloc-instrumented`, which can be 
directly executed.

```
$ ./alloc-instrumented
orbit alloc: 16 => 0x12f1260
orbit alloc: 16 => 0x12f1690
orbit alloc: 16 => 0x12f16b0
orbit alloc: 4 => 0x12f16d0
foo(5)=30
orbit alloc: 16 => 0x12f16b0
orbit alloc: 16 => 0x12f1690
orbit alloc: 16 => 0x12f1260
orbit alloc: 4 => 0x12f16d0
foo(15)=90
orbit alloc: 16 => 0x12f1260
orbit alloc: 16 => 0x12f1690
orbit alloc: 16 => 0x12f16b0
orbit alloc: 4 => 0x12f16d0
foo(20)=120
```

A more complex instrumentation will involve instrumenting calls to a runtime 
library (`OrbitTracker`) in `runtime` directory. This allows decoupling of the
instrumented logic to the library. Otherwise, doing everything in raw LLVM 
IR is tedious and error-prone.

```
$ ../scripts/instrument-compile.sh ../test/alloc.bc
```

This will also produce `alloc-instrumented`. But the difference is that 
this instrumented binary will call our custom tracking function `void __orbit_track_gobj(char *addr, size_t size)`
in the runtime library (instead of simple `printf`), which is linked with the executable.

Now try running this instrumented binary:

```
$ ./alloc-instrumented
opening orbit tracker output file orbit_gobj_pid_985.dat
foo(5)=30
foo(15)=90
foo(20)=120

$ cat orbit_gobj_pid_985.dat
16 => 0x15b9490
16 => 0x15ba4c0
16 => 0x15ba4e0
4 => 0x15ba500
16 => 0x15ba4e0
16 => 0x15ba4c0
16 => 0x15b9490
4 => 0x15ba500
16 => 0x15b9490
16 => 0x15ba4c0
16 => 0x15ba4e0
4 => 0x15ba500
```

As shown above, the runtime tracking library saves the information to a file 
as the program runs. Note that the last part of the trace file is PID (`orbit_gobj_pid_xxx.dat`), 
which will change in different runs.


#### Instrumenting MySQL

Running ObiWanAnalysis
TODO

```
opt -load lib/libObiWanAnalysisPass.so -obi-wan-analysis -target-functions DeadlockChecker::check_and_resolve < ../target-sys/mysql-build/sql/mysqld.bc > /dev/null
```

```
clang deadlock-instrument.bc -o test-instrumented -L /home/ubuntu/orbit-compiler-temp/build/runtime -l:libOrbitTracker.a -lstdc++
```

#### Current Limitations
TODO

## Code Styles

Please refer to the [code style](codeStyle.md) for the coding convention and practice.
