@echo off

set searchFiles=*.h *.cpp

echo -----

echo RESEARCH LIST:
findstr -s -n -i -l "RESEARCH(" %searchFiles%

echo -----