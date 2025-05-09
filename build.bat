::
: \file build.bat
: \author Isaiah Lateer
: 
: Used to build the project.
: 

@echo off

setlocal enabledelayedexpansion

pushd "%programfiles%"

for /r %%a in (*vcvarsall.bat) do (
    set devcmd="%%a"
)

popd

if not defined devcmd (
    pushd "%programfiles(x86)%"

    for /r %%a in (*vcvarsall.bat) do (
        set devcmd="%%a"
    )

    popd
)

if not defined devcmd (
    echo [ERROR] Failed to find developer command prompt.
    exit
)

pushd "%~dp0"

if not exist obj mkdir obj
if not exist obj\%target% mkdir obj\%target%

if not exist bin mkdir bin
if not exist bin\%target% mkdir bin\%target%

for /r %%a in (*.c) do (
    if /i "%%~xa"==".c" (
        set source_files=!source_files! %%a
        set object_files=!object_files! obj\%%~na.obj
    )
)

call set "source_files=%%source_files:%~dp0=%%"
set source_files=!source_files:~1!

set cflags=/nologo /c /EHsc /fp:precise /sdl /GS /W4 /WX /MD /DNDEBUG ^
    /D_UNICODE /DUNICODE /Isrc /Iinclude /Foobj\
set lflags=/nologo user32.lib gdi32.lib opengl32.lib /out:bin\opengl_context.exe

call %devcmd% x64

cl %cflags% %source_files%
link %lflags% %object_files%

popd
