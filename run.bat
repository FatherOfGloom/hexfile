@REM @echo off

set TARGET_NAME=hexfile.exe
set ROOT_FOLDER=D:\Users\aauhabin1\dev\c\hexfile\
set CLI_ARGS=%*

pushd %ROOT_FOLDER%
call ./build.bat

if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo:
@REM echo %CLI_ARGS%

pushd bin
%TARGET_NAME% %CLI_ARGS%
popd
popd