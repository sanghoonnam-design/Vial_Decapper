@echo off
setlocal
pushd "%~dp0" || exit /b 1
if not exist ".venv\Scripts\python.exe" (
    echo The simulator uses the Vial Decapper Python environment.
    echo Set up .venv with the project's requirements.txt first.
    pause
    popd
    exit /b 1
)
".venv\Scripts\python.exe" -B -m simulator
set "result=%ERRORLEVEL%"
if not "%result%"=="0" pause
popd
exit /b %result%
