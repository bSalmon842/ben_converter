@echo off

set searchFiles=*.h *.cpp

echo -----

echo Double Semicolons Found:
findstr -s -n -i -l ";;" %searchFiles%

echo -----