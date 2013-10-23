python setup.py py2exe
copy /Y static_dist\* dist\
ResHacker.exe -addoverwrite dist\comic_viewer.exe,dist\comic_viewer2.exe,comic.ico,ICONGROUP,0,0
IF EXIST "dist\comic_viewer2.exe" (
move dist\comic_viewer2.exe dist\comic_viewer.exe
) ELSE (
echo ResHacker has failed to create comic_viewer2.exe
)
