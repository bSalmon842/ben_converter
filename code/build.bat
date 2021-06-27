@echo off

REM -MTd for debug build
set commonFlagsCompiler=-MTd -nologo -Gm- -GR- -fp:fast -EHa- -O2 -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4996 -FC -Z7 -DCONVERTER_INTERNAL=1 -DCONVERTER_SLOW=1
set commonFlagsLinker= -incremental:no -opt:ref

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl %commonFlagsCompiler% ..\code\ben_converter.cpp /link %commonFlagsLinker%
popd
