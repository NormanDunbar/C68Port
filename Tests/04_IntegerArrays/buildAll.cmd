@echo off
rem
rem A script to compile everything named *.c in the current directory.
rem
for /f "tokens=1 delims=." %%f in ('dir /b *.c') do (
    echo Compiling %%f.c to %%f.exe ...
    buildTests %%f
)

pause