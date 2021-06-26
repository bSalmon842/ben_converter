@echo off

set searchFiles=*.cpp

echo -----

echo Errant Structs Found:
findstr -s -n -b -l "struct " %searchFiles%

echo -----