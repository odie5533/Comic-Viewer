cd /d "%~dp0"
set PATH=%PATH%;C:\Python27
call .virt1\Scripts\activate.bat
python "%~dp0comic_viewer.py" %1
pause