@echo off
rem Create a test executable for the filename passed on the command line.
rem There are no extensions required, so:
rem
rem     .\buildTest 00_Scope
rem
echo.
echo USAGE: .\buildTest filename (with no extension)
echo EXAMPLE:       .\buildTest 00_Scope
echo.
rem
gcc -o %1.exe -I ..\..\SBLocal %1.c ..\..\SBLocal\SBLocal.c

