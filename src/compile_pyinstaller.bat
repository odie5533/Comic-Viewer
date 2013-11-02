pyinstaller-2.0\pyinstaller.py --onefile --noconsole -i comic.ico comic_viewer.py
REM Copy unrar.exe so the program works immediately and can be debugged
copy /Y static_dist\* dist\
pause

ResHacker.exe -addoverwrite dist\comic_viewer.exe,dist\comic_viewer2.exe,comic.ico,ICONGROUP,101,0
IF EXIST "dist\comic_viewer2.exe" (
move dist\comic_viewer2.exe dist\comic_viewer.exe
) ELSE (
echo ResHacker has failed to create comic_viewer2.exe
)
