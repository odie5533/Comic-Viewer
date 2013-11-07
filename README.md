# Please visit the website at
# [http://odie5533.github.io/Comic-Viewer/](http://odie5533.github.io/Comic-Viewer/)

Description
===========
Comic Viewer is a comic book reader which displays images contained within
compressed archives. Comic Viewer can load either cbz or cbr files, which are
zip and rar compressed archives, respectively. Comic Viewer is written in
Python and the Windows thumbnail generator is written in C++.

Acknowledgements
================
Thanks to the Pygame team, SDL team, py2exe team, Marko Kreen for rarfile,
and saivert for the NSIS script fileassoc.nsh.

The compile chain for this program uses ResHacker, an ancient piece of software
which still amazes me. ResHacker must be included in the PATH and is available
from <http://angusj.com/resourcehacker/>.

* setup.py uses the SDL font trick as described at
    <http://thadeusb.com/weblog/2009/4/15/pygame_font_and_py2exe>.
* SDL_VIDEO_WINDOW_POS trick was found at
    <http://www.pygame.org/wiki/SettingWindowPosition?parent=CookBook>.
* Eli Bendersky's tutorials were used as reference early on:
    <http://eli.thegreenplace.net/category/programming/python/pygame-tutorial/>.
* Peyton McCullough's PyGame example was used for his 'moving' method:
    <http://www.devshed.com/c/a/Python/A-PyGame-Working-Example-continued/3/>.
* rarfile.py was modified to include the win32process flags.
