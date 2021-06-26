@echo off

set searchFiles=*.h *.cpp

echo -----

echo UPCOMING LIST:
findstr -s -n -i -l "UPCOMING(" %searchFiles%

echo -----