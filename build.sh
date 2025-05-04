##
# \file build.sh
# \author Isaiah Lateer
# 
# Used to build the project.
# 

#!/bin/bash

directory=$(dirname "$0")

pushd $directory

mkdir -p obj
mkdir -p bin

source_files=$(find . -type f -name "*.c")

cflags="-c -Wall -Werror -DNDEBUG -Isrc -Iinclude"
lflags="-o bin/opengl.exe"

for source_file in $source_files; do
    object_file="obj/$(basename "${source_file%.c}.obj")"
    gcc $cflags $source_file -o $object_file
    object_files="${object_files} $object_file"
done

gcc $object_files $lflags

popd
