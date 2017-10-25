import fileinput, os, sys
from distutils.core import Extension, setup
from distutils import sysconfig

xft         = []
cargs       = []
largs       = []
install_dir = sysconfig.get_python_lib() + "/pypanel"
files       = ["COPYING", "README", "ppicon.png", "pypanelrc"]

try:
    from Xlib import X, display, Xatom, Xutil
except:
    print "\nPyPanel requires the Python X library -"
    print "http://sourceforge.net/projects/python-xlib/"
    sys.exit()


for config in ("freetype-config", "imlib2-config"):
    cflags = os.popen("%s --cflags" % config).read().strip().split()
    if cflags:
        for cflag in cflags:
            if cflag not in cargs:
                cargs.append(cflag)
    
        if config == "freetype-config":
            xft.append(("HAVE_XFT",1))
            largs.append("-lXft")
           
        libs = os.popen("%s --libs" % config).read().strip().split()
        if libs:
            for lib in libs:
                if lib not in largs:
                    largs.append(lib)
    else:
        if config == "imlib2-config":
            print "\nPyPanel requires the Imlib2 library -"
            print "http://www.enlightenment.org/pages/imlib2.html"
            sys.exit()
    
cargs.sort()
largs.sort()

# Fix the shebang
for line in fileinput.input(["pypanel"], inplace=1):
    if fileinput.isfirstline():
        print "#!%s -O" % sys.executable
    else:
        print line,        

module = Extension("ppmodule",
                   sources            = ["ppmodule.c"],
                   include_dirs       = ["/usr/X11R6/include"],
                   library_dirs       = [],
                   libraries          = [],  
                   extra_compile_args = cargs,
                   extra_link_args    = largs,
                   define_macros      = xft,                    
                  )  

setup(name             = "PyPanel",
      author           = "Jon Gelo",
      author_email     = "ziljian@users.sourceforge.net",
      version          = "2.0",
      license          = "GPL",
      platforms        = "POSIX",
      description      = "Lightweight panel/taskbar for X11 Window Managers",
      long_description = "See README for more information",
      url              = "http://pypanel.sourceforge.net",
      data_files       = [(install_dir, files)],
      scripts          = ["pypanel"],
      ext_modules      = [module]
     )
     
