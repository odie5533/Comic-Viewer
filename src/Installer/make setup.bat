REM make setup does not run py2exe compile if the dist folder already exists.
REM Thus it is somewhat like a 'make'

@echo off
set NSIS_DIR=%ProgramFiles(x86)%\NSIS
set SRC_DIR=..
set INSTALLER_DIR=%~dp0

cd "%SRC_DIR%"
IF NOT EXIST "dist" (
call compile_pyinstaller.bat
rmdir /S /Q build
)
cd "%INSTALLER_DIR%"
"%NSIS_DIR%\makensis" comic_viewer.nsi
pause
