/*
PyPanel v1.3 - Lightweight panel/taskbar for X11 window managers
Copyright (c) 2003-2004 Jon Gelo (ziljian@users.sourceforge.net)

This file is part of PyPanel.
PyPanel is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <Python.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#include <X11/xpm.h>
#include "ppicon.xpm"
#endif

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
XftDraw *draw;
XftFont *xf;
#else
XFontStruct *xf;
#endif

Display *dsp;
Pixmap def_icon, def_mask;
GC gc;
typedef unsigned long CARD32;
int scr;

/*----------------------------------------*/
int pperror(Display *dsp, XErrorEvent *e) {
/*----------------------------------------*/
    return 0;
}

/*---------------------------------------------------------*/
static PyObject * _ppshade(PyObject *self, PyObject *args) {
/*---------------------------------------------------------*/
    Window panel;
    XImage *img;
    Pixmap bgpm;
    CARD32 rmask, gmask, bmask;
    CARD32 *table, *rtable, *gtable, *btable;
    int i, shade, rsh, gsh, bsh, x1, x2, y1, y2;
    unsigned int upper;

    if (!PyArg_ParseTuple(args, "liiiii", &panel, &shade, &x1, &y1, &x2, &y2))
        return NULL;
    
    img  = XGetImage(dsp, RootWindow(dsp, scr), x1, y1, x2, y2, -1, ZPixmap);
    bgpm = XCreatePixmap(dsp, RootWindow(dsp, scr), x2, y2, DefaultDepth(dsp, scr)); 
    
    if (shade < 0) shade = 0;
    if (shade > 100) shade = 100;
    upper = (unsigned int)((((CARD32)0xffff)*((CARD32)shade))/100);
    
    switch (img->bits_per_pixel) {
        case 16: {
            rmask  = 0xf800;
            gmask  = 0x07e0;
            bmask  = 0x001f;
            table  = (CARD32*)malloc(sizeof(CARD32)*(32+64+32));
            rtable = table;
            gtable = table+32;
            btable = table+32+64;
            rsh    = 11;
            gsh    = 5;
            bsh    = 0;
            break;
        }
        case 32: {
            rmask  = 0xff0000;
            gmask  = 0x00ff00;
            bmask  = 0x0000ff;
            table  = (CARD32*)malloc(sizeof(CARD32)*(256+256+256));
            rtable = table;
            gtable = table+256;
            btable = table+256+256;
            rsh    = 16;
            gsh    = 8;
            bsh    = 0;
            break;
        }
        default:
            return Py_BuildValue("i", 0);
    }
    
    for (i = 0; i <= rmask>>rsh; i++) {
        CARD32 tmp;
        tmp = ((CARD32)i)*((CARD32)upper);
        rtable[i] = (tmp/0xffff)<<rsh;
    }
    for (i = 0; i <= gmask>>gsh; i++) {
        CARD32 tmp;
        tmp = ((CARD32)i)*((CARD32)upper);
        gtable[i] = (tmp/0xffff)<<gsh;
    }
    for (i = 0; i <= bmask>>bsh; i++) {
        CARD32 tmp;
        tmp = ((CARD32)i)*((CARD32)upper);
        btable[i] = (tmp/0xffff)<<bsh;
    }
    
    switch (img->bits_per_pixel) {
        case 16: {
            unsigned short *p1, *pf, *p, *pl;
            p1 = (unsigned short*)img->data;
            pf = (unsigned short*)(img->data + y2 * img->bytes_per_line);
            while (p1 < pf) {
                p = p1;
                pl = p1 + x2;
                for (; p < pl; p++) {
                    *p = rtable[(*p & rmask)>>11] |
                         gtable[(*p & gmask)>> 5] |
                         btable[(*p & bmask)];
                }
                p1 = (unsigned short*)((char*)p1 + img->bytes_per_line);
            }
          break;
        }
        case 32: {
            CARD32 *p1, *pf, *p, *pl;
            p1 = (CARD32*)img->data;
            pf = (CARD32*)(img->data + y2 * img->bytes_per_line);
            while (p1 < pf) {
                p = p1;
                pl = p1 + x2;
                for (; p < pl; p++) {
                    *p = rtable[(*p & rmask)>>16] |
                         gtable[(*p & gmask)>> 8] |
                         btable[(*p & bmask)]     |
                                (*p & ~0xffffff);
                }
                p1 = (CARD32*)((char*)p1 + img->bytes_per_line);
            }
            break;
        }
    }
    
    XPutImage(dsp, bgpm, gc, img, 0, 0, 0, 0, x2, y2);
    XSetWindowBackgroundPixmap(dsp, panel, bgpm);
    XDestroyImage(img);
    XFreePixmap(dsp, bgpm);
    free(table);
    return Py_BuildValue("i", 1);
}

/*--------------------------------------------------------*/
static PyObject * _ppfont(PyObject *self, PyObject *args) {
/*--------------------------------------------------------*/
#ifdef HAVE_XFT
    XftColor xftcol;
    XGlyphInfo ginfo;
    XRenderColor rcol;
    Visual *visual = DefaultVisual(dsp, scr);
    Colormap cmap  = DefaultColormap(dsp, scr);
#endif
    XColor xcol;
    Window win;
    char *text;
    int len, font_x, font_y, p_height, limit;
    unsigned long pixel;
    
    if (!PyArg_ParseTuple(args, "lliiis#", &win, &pixel, &font_x, &p_height,
                          &limit, &text, &len))
        return NULL;
    
    xcol.pixel = pixel;
    
#ifdef HAVE_XFT
    if (limit) {
        while (1) {
            XftTextExtentsUtf8(dsp, xf, text, len, &ginfo);
            if (ginfo.width >= limit)
                len--;
            else
                break;
        }
    }
        
    draw = XftDrawCreate(dsp, win, visual, cmap);
    font_y = xf->ascent+((p_height-(xf->ascent+xf->descent))/2);
    XQueryColor(dsp, cmap, &xcol);
    rcol.red   = xcol.red;
    rcol.green = xcol.green;
    rcol.blue  = xcol.blue;
    rcol.alpha = 0xffff;
    XftColorAllocValue(dsp, visual, cmap, &rcol, &xftcol);
    XftDrawStringUtf8(draw, &xftcol, xf, font_x, font_y, text, len);
    XftColorFree(dsp, visual, cmap, &xftcol);
    XftDrawDestroy(draw);
#else
    if (limit) {
        while (XTextWidth(xf, text, len) >= limit)
            len--;
    }
    XSetForeground(dsp, gc, pixel);
    font_y = xf->ascent+((p_height-xf->ascent)/2);
    XDrawString(dsp, win, gc, font_x, font_y, text, len);
#endif
    XSync(dsp, False);
    return Py_BuildValue("i", 1);
}

/*------------------------------------------------------------*/
static PyObject * _ppfontsize(PyObject *self, PyObject *args) {
/*------------------------------------------------------------*/
#ifdef HAVE_XFT
    XGlyphInfo ginfo;
#endif
    char *text;
    int len;
    
    if (!PyArg_ParseTuple(args, "s#", &text, &len))
        return NULL;
    
#ifdef HAVE_XFT
    XftTextExtents8(dsp, xf, text, len, &ginfo);
    return Py_BuildValue("i", ginfo.width);
#else
    return Py_BuildValue("i", (int)XTextWidth(xf, text, len));
#endif
}

/*--------------------------------------------------------*/
static PyObject * _ppicon(PyObject *self, PyObject *args) {
/*--------------------------------------------------------*/
    Window win, panel, root;
    XWMHints *hints;
    XGCValues gcv;
    GC mgc;
    Pixmap src_icon = None;
    Pixmap src_mask = None;
    Pixmap trg_icon = None;
    Pixmap trg_mask = None;
    int x2, y2, x, y, w, h, d, bw, i_x, i_y, i_width, i_height; 
    int depth = DefaultDepth(dsp, scr);
    
    if (!PyArg_ParseTuple(args, "lliiii", &panel, &win, &i_x, &i_y, &i_width,
                          &i_height))
        return NULL;
	
    if ((hints = XGetWMHints(dsp, win))) {
        if (hints->flags & IconPixmapHint) {
            src_icon = hints->icon_pixmap;
            if (hints->flags & IconMaskHint)
                src_mask = hints->icon_mask;
        }
        XFree(hints);
    }
    else
        return Py_BuildValue("i", 0);
    
    if (src_icon == None) {
        src_icon = def_icon;
        src_mask = def_mask;
        if (src_icon == None) 
            return Py_BuildValue("i", 0);
    }
    
    if (!XGetGeometry(dsp, src_icon, &root, &x, &y, &w, &h, &bw, &d))
        return Py_BuildValue("i", 0);
    trg_icon = XCreatePixmap(dsp, panel, i_width, i_height, depth);
    
    if (src_mask != None) {
        gcv.graphics_exposures = False;
        gcv.subwindow_mode = IncludeInferiors;
        trg_mask = XCreatePixmap(dsp, panel, i_width, i_height, 1);
        mgc = XCreateGC(dsp, trg_mask, GCGraphicsExposures|GCSubwindowMode, &gcv);
    }
    
    /* Scaling routine courtesy of fspanel */
    for (y = i_height - 1; y >= 0; y--) {
        y2 = (y * h) / i_height;
        for (x = i_width - 1; x >= 0; x--) {
            x2 = (x * w) / i_width;
            if (d != depth) 
                XCopyPlane(dsp, src_icon, trg_icon, gc, x2, y2, 1, 1, x, y, 1);
            else
                XCopyArea(dsp, src_icon, trg_icon, gc, x2, y2, 1, 1, x, y);
            if (src_mask != None)
                XCopyArea(dsp, src_mask, trg_mask, mgc, x2, y2, 1, 1, x, y);
        }
    }
    
    XSetForeground(dsp, gc, WhitePixel(dsp, scr));
    XSetClipMask(dsp, gc, trg_mask);
	XSetClipOrigin(dsp, gc, i_x, i_y);
	XCopyArea(dsp, trg_icon, panel, gc, 0, 0, i_width, i_height, i_x, i_y);
	XSetClipMask(dsp, gc, None);
    XFreePixmap(dsp, trg_icon);
    
    if (src_mask != None) {
        XFreePixmap(dsp, trg_mask);
        XFreeGC(dsp, mgc);
    }
    
    return Py_BuildValue("i", 1);
}

/*--------------------------------------------------------*/
static PyObject * _ppinit(PyObject *self, PyObject *args) {
/*--------------------------------------------------------*/
    XGCValues gcv;
    char *font;
    int len;
    
    XSetErrorHandler(pperror);
    gcv.graphics_exposures = False;
    dsp = XOpenDisplay(NULL);
    scr = DefaultScreen(dsp);
    
#ifdef HAVE_XPM
    XpmCreatePixmapFromData(dsp, RootWindow(dsp, scr), ppicon,
                            &def_icon, &def_mask, NULL);
#else
    def_icon = None;
    def_mask = None;
#endif
    
    if (!PyArg_ParseTuple(args, "s#", &font, &len))
        return NULL;
    
#ifdef HAVE_XFT
    if (font[0] == '-')
        xf = XftFontOpenXlfd(dsp, scr, font);
    else
        xf = XftFontOpenName(dsp, scr, font);
    gc = XCreateGC(dsp, RootWindow(dsp, scr), GCGraphicsExposures, &gcv);
#else
    xf = XLoadQueryFont(dsp, font);
    if (!xf)
        xf = XLoadQueryFont(dsp, "fixed");
    gcv.font = xf->fid;
    gc = XCreateGC(dsp, RootWindow(dsp, scr), GCFont|GCGraphicsExposures, &gcv);
#endif
    return Py_BuildValue("i", 1);
}

/*-------------------------------*/
static PyMethodDef PPMethods[] = {
/*-------------------------------*/
    {"ppshade",    _ppshade,    METH_VARARGS, "PyPanel Shading Function"},
    {"ppinit",     _ppinit,     METH_VARARGS, "PyPanel Init Function"},
    {"ppfont",     _ppfont,     METH_VARARGS, "PyPanel Xft Font Function"},
    {"ppfontsize", _ppfontsize, METH_VARARGS, "PyPanel Xft Fontsize Function"},
    {"ppicon",     _ppicon,     METH_VARARGS, "PyPanel Icon Function"},
    {NULL, NULL, 0, NULL}
};

/*------------------*/
/*  PyMODINIT_FUNC  */
void initppmodule() {
/*------------------*/
    (void)Py_InitModule("ppmodule", PPMethods);
}
