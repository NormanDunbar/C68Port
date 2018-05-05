#!/bin/bash
#
# Create a test executable for the filename passed on the command line.
# There are no extensions required, so:
#
#     ./buildTest 00_Scope
#
echo
echo "USAGE:      ./buildTest filename (with no extension)"
echo EXAMPLE:    ./buildTest 00_Scope
echo
#
gcc -o "${1}" -I ../../SBLocal/ "${1}".c ../../SBLocal/SBLocal.c

