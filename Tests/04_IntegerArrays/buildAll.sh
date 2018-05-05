#!/bin/bash
#
# A script to compile everything named *.c in the current directory.
#
for f in `ls *.c` 
do 
    echo Compiling "${f}".c to "${f}".exe ...
    ./buildTests.sh "${f%.c}"
done
