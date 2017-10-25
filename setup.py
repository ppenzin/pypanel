from distutils.core import Extension, setup
from distutils import sysconfig

try:
    from Xlib import X, display, Xatom, Xutil
except:
    print "\nPyPanel requires the Python X library -"
    print "http://sourceforge.net/projects/python-xlib/"
    import sys
    sys.exit()
#----------------------------------------------------------------------------
# Building PyPanel with XPM support will allow you to use a default icon
# (src/ppicon.xpm) for applications which don't provide one (e.g. Xterm).
#
# Adjust the include_dirs and library_dirs below to suit your system.  To 
# build without Xft or Xpm support, remove the appropriate string from
# 'libraries' and the appropriate tuple from 'define_macros'.
#----------------------------------------------------------------------------
module = Extension(
    "ppmodule",
    sources       = ["src/ppmodule.c"],
    include_dirs  = ["/usr/X11R6/include",
                     "/usr/include/freetype2",],
    library_dirs  = ["/usr/X11R6/lib"],
    libraries     = ["X11", "Xft", "Xpm"],  
    define_macros = [("HAVE_XFT", 1), ("HAVE_XPM", 1)],                    
    )

install_dir = sysconfig.get_python_lib() + "/pypanel"
files       = ["COPYING", "README", "pypanelrc"]
script      = "src/pypanel"

setup (name             = "PyPanel",
       author           = "Jon Gelo",
       author_email     = "ziljian@users.sourceforge.net",
       version          = "1.3",
       license          = "GPL",
       platforms        = "POSIX",
       description      = "Lightweight panel/taskbar for X11 Window Managers",
       long_description = "See README for more information",
       url              = "http://pypanel.sourceforge.net",
       data_files       = [(install_dir, files)],
       scripts          = [script],
       ext_modules      = [module])
