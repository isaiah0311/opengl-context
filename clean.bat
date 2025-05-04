::
: \file clean.bat
: \author Isaiah Lateer
: 
: Used to clean the workspace.
: 

@echo off

pushd "%~dp0"

rmdir /s /q obj
rmdir /s /q bin

popd
