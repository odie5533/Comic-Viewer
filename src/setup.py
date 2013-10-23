from distutils.core import setup
import py2exe
import glob
import os

origIsSystemDLL = py2exe.build_exe.isSystemDLL
def isSystemDLL(pathname):
       if os.path.basename(pathname).lower() in ["sdl_ttf.dll"]:
               return 0
       return origIsSystemDLL(pathname)
py2exe.build_exe.isSystemDLL = isSystemDLL


setup(
    name = "Comic Viewer",
    windows = [
        {
            'script':'comic_viewer.py',
            'icon_resources':[(0, 'comic.ico')]
        },
    ],

    options = {
        'py2exe': {
			"compressed":1,
			"optimize":2,
                        "bundle_files":2,
                        "packages":"encodings",
                        "includes":"cairo,pango,pangocairo,"\
                               "atk,gobject,gio,pygame,win32ui",
                        "excludes":"pyreadline,difflib,doctest,locale,"\
                               "pickle,calendar,httplib,urllib,expat",
                        "dll_excludes":"libglib-2.0-0.dll,smpeg.dll,zlib1.dll,libgtk-win32-2.0-0.dll,libcairo-2.dll,libgdk-win32-2.0-0.dll,libgio-2.0-0.dll,libpango-1.0-0.dll,libpangowin32-1.0-0.dll,libpangocairo-1.0-0.dll,libexpat-1.dll,libgdk_pixbuf-2.0-0.dll,libpangoft2-1.0-0.dll,libgobject-2.0-0.dll,libatk-1.0-0.dll,libgthread-2.0-0.dll"
            }
        },
    zipfile = None
)
