@echo off
setlocal enabledelayedexpansion

set "EXE=C:\Proga\PROJECTS\RSMT_Steiner\build\rsmt_steiner.exe"

if not exist "%EXE%" (
    echo [ERROR] Executable "%EXE%" not found! 1>&2
    exit /b 1
)

echo Running first pass...
(
    for /l %%i in (5,1,40) do (
        set "num=000%%i"
        set "num=!num:~-4!"
        set "file=dat\!num!_0000.json"
        if not exist "!file!" (
            echo NO SUCH ^<^!file!^> FILE 1>&2
        ) else (
            for /f "usebackq delims=" %%a in (`""%EXE%" "!file!""`) do echo !file!: %%a
        )
    )
) > log.log

echo Running second pass with -m...
(
    for /l %%i in (5,1,40) do (
        set "num=000%%i"
        set "num=!num:~-4!"
        set "file=dat\!num!_0000.json"
        if not exist "!file!" (
            echo NO SUCH ^<^!file!^> FILE 1>&2
        ) else (
            for /f "usebackq delims=" %%a in (`""%EXE%" -m "!file!""`) do echo !file!: %%a
        )
    )
) > log-m.log

echo Done.
