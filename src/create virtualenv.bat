set PATH=%PATH%;C:\Python27;C:\Python27\Scripts
virtualenv .virt1
call .virt1\Scripts\activate
easy_install ..\redist\pycairo-1.10.0.win32-py2.7.exe
easy_install ..\redist\pygame-1.9.2a0.win32-py2.7.exe
easy_install ..\redist\pywin32-218.4.win32-py2.7.exe