REM rebuild_all cleans any old files and compiles everything from scratch
REM producing the final setup executable.

set NSIS_DIR=%ProgramFiles(x86)%\NSIS
set SRC_DIR=..
set INSTALLER_DIR=%~dp0

cd "%SRC_DIR%"
rmdir /S /Q build
rmdir /S /Q dist
call compile_pyinstaller.bat
cd "%INSTALLER_DIR%"
"%NSIS_DIR%\makensis" comic_viewer.nsi
pause
