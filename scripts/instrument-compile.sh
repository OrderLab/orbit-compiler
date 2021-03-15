#!/bin/bash

# The Obi-wan Project
#
# Copyright (c) 2021, Johns Hopkins University - Order Lab.
#
#    All rights reserved.
#    Licensed under the Apache License, Version 2.0 (the "License");

function display_usage()
{
  cat <<EOF
Usage:
  $0 [options] SOURCE_BC_FILE

  -h, --help:       display this message

      --printf:     instrument using regular printf (w/o runtime lib)

  -l, --link:       additional link flags to pass to GCC

  -r, --runtime:    path to libOrbitTracker.a/.so runtime 

      --dry-run:    dry run, do not

  -o, --output:     output file name
EOF
}

function parse_args()
{
  args=()
  while [[ "$#" -gt 0 ]]
  do
    case $1 in
      -o|--output)
        output="$2"
        shift 2
        ;;
      --printf)
        with_runtime=0
        shift 1
        ;;
      -l|--link)
        link_flags="$2"
        shift 2
        ;;
      --dry-run)
        maybe=echo
        shift
        ;;
      -r|--runtime)
        runtime_path="$2"
        shift 2
        ;;
      -h|--help)
        display_usage
        exit 0
        ;;
      *)
        args+=("$1")
        shift
        ;;
    esac
  done
}

this_dir=$(cd "$(dirname "${BASH_SOURCE-$0}")"; pwd)
build_dir=$(cd "${this_dir}/../build"; pwd)
output=
plugin_path=
instrumenter=
maybe=
runtime_path=
link_libs=
with_runtime=1
plugin_args=""
link_flags=""

parse_args "$@"
set -- "${args[@]}"
if [ $# -ne 1 ]; then
  display_usage
  exit 1
fi
source_bc_file=$1
if [[ $source_bc_file != *.bc ]]; then
  echo "Source file must be in .bc format"
  exit 1
fi
if [ ! -f $source_bc_file ]; then
  echo "Input bitcode file $source_bc_file does not exist"
  exit 1
fi
if [ -z "$plugin_path" ]; then
  plugin_path=${build_dir}/lib/libLLVMInstrument.so
fi
instrumenter=${build_dir}/bin/instrumentor
if [ ! -x $instrumenter ]; then
  echo "Instrumenter $instrumenter not found"
  exit 1
fi
if [ -z "$runtime_path" ]; then
  runtime_path=${build_dir}/runtime
fi
if [ ! -d "$runtime_path" ]; then
  echo "Runtime path $runtime_path does not exist"
  exit 1
fi
runtime_lib="$runtime_path"/libOrbitTracker.a
if [ ! -f  $runtime_lib ]; then
  echo "Runtime library $runtime_lib not found"
  exit 1
fi 
if [ -z "$output" ]; then
  source_base=$(basename $source_bc_file)
  output_base="${source_base%.*}"-instrumented
  output_bc=${output_base}.bc
  output_asm=${output_base}.s
  output_exe=${output_base}
else
  output_bc=${output}.bc
  output_asm=${output}.s
  output_exe=${output}
fi

if [ $with_runtime -eq 0 ]; then
  instrumenter_args="$instrumenter_args -use-printf"
fi

$maybe $instrumenter $instrumenter_args $source_bc_file -o $output_bc

# Newer clang can directly compile bitcode to executable, Yay!
if [ $with_runtime -eq 0 ]; then
  # when using regular printf for instrumentation, we do not need to link 
  # with the runtime tracker library
  $maybe clang -o $output_exe $output_bc
else
  # otherwise, we need to link with the library to produce the executable
  # here we are linking with static lib, which is less flexible but faster
  $maybe clang -o $output_exe -L $runtime_path -l:libOrbitTracker.a $output_bc
  # another way is to link with the shared lib, which is flexible but slower
  # $maybe clang -o $output_exe -L $runtime_path -lOrbitTracker $output_bc
fi

# Old way of producing executable through the assembly

# $maybe llc -O0 -disable-fp-elim -filetype=asm -o $output_asm $output_bc
# $maybe llvm-dis $output_bc
# # linking with shared runtime lib, flexible but slower
# # $maybe gcc -no-pie -O0 -fno-inline -o $output_exe $output_asm -L $runtime_path -lAddrTracker
# # linking with static runtime lib, less flexible but faster
# $maybe g++ -no-pie -pg -O0 -g -fno-inline -o $output_exe $output_asm -L $runtime_path -l:libAddrTracker.a $link_flags

echo "$(tput setaf 2)Instrumented executable is saved at $output_exe $(tput sgr 0)"
