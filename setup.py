import fileinput, os, sys
from distutils.core import Extension, setup
from distutils import sysconfig

# Full paths to imlib2-config and freetype-config, adjust as needed -
configs = ["/usr/bin/freetype-config", "/usr/bin/imlib2-config"]

# Adjust or add any additional include directories here -
idirs   = ["/usr/X11R6/include"]

# Add any additional library directories here -
ldirs   = []

# Add any additional compile options here -
cargs   = ["-Wall"]

# Full path to libImlib2 without the extension
imlib2  = "/usr/lib/libImlib2"

#------------------------------------------------------------------------------
# The rest of this script should not need to be modified! 
#------------------------------------------------------------------------------
libs        = [] # libraries (listed without -l)
largs       = [] # extra link arguments
defs        = [] # define macros
install_dir = sysconfig.get_python_lib() + "/pypanel"
files       = ["COPYING", "README", "ppicon.png", "pypanelrc"]
imlib2_fix  = 0
imlib2_str  = """
    import dl
    imlib2 = dl.open("%s%s", dl.RTLD_NOW|dl.RTLD_GLOBAL)
    imlib2.close()
    del imlib2""" 

try:
    from Xlib import X, display, Xatom, Xutil
except:
    print "\nPyPanel requires the Python X library -"
    print "http://sourceforge.net/projects/python-xlib/"
    sys.exit()

for config in configs:
    package = os.path.split(config)[1]
    
    if os.path.isfile(config):
        for cflag in os.popen("%s --cflags" % config).read().strip().split(): 
            flag = cflag[:2]
            opt  = cflag[2:]    
            
            if flag == "-I" and opt not in idirs:
                idirs.append(opt)
            else:
                if cflag not in cargs:
                    cargs.append(cflag)    
                
        for lib in os.popen("%s --libs" % config).read().strip().split():
            flag = lib[:2]
            opt  = lib[2:]
            
            if flag == "-L" and opt not in ldirs:
                ldirs.append(opt)
            elif flag == "-l":
                if opt not in libs:
                    libs.append(opt)
            else:
                if lib not in largs:
                    largs.append(lib)
                
        if package == "freetype-config":
            defs.append(("HAVE_XFT", 1))
            if "Xft" not in libs:
                libs.append("Xft")
                
        if package == "imlib2-config":
            # Add the workaround for Imlib2 version 1.2.x and up -
            # Python dlopens libImlib2 with RTLD_LOCAL by default.  To avoid
            # undefined symbols, dlopen it first with the RTLD_GLOBAL flag.
            version = os.popen("%s --version" % config).read().strip()
            if float(version[:3]) >= 1.2:
                import imp
                for ext, mode, typ in imp.get_suffixes():
                    if typ == imp.C_EXTENSION:
                        imlib2_fix = 1
                        imlib2_str = imlib2_str % (imlib2, os.path.splitext(ext)[1]) 
                        break
    else:
        if package == "imlib2-config":
            print "\nPyPanel requires the Imlib2 library -"
            print "http://www.enlightenment.org/pages/imlib2.html"
            sys.exit()

# Fix the shebang and add the Imlib2 workaround if necessary
if len(sys.argv) > 1 and sys.argv[1] != "sdist":
    for line in fileinput.input(["pypanel"], inplace=1):
        if fileinput.isfirstline():
            print "#!%s -O" % sys.executable
        elif imlib2_fix and line.strip() == "#<Imlib2 workaround>":
            print imlib2_str
        else:
            print line,        

module = Extension("ppmodule",
                   sources            = ["ppmodule.c"],
                   include_dirs       = idirs,
                   library_dirs       = ldirs,
                   libraries          = libs,  
                   extra_compile_args = cargs,
                   extra_link_args    = largs,
                   define_macros      = defs,                    
                  )  

setup(name             = "PyPanel",
      author           = "Jon Gelo",
      author_email     = "ziljian@users.sourceforge.net",
      version          = "2.2",
      license          = "GPL",
      platforms        = "POSIX",
      description      = "Lightweight panel/taskbar for X11 Window Managers",
      long_description = "See README for more information",
      url              = "http://pypanel.sourceforge.net",
      data_files       = [(install_dir, files)],
      scripts          = ["pypanel"],
      ext_modules      = [module]
     )
