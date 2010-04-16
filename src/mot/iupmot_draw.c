/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <Xm/Xm.h>
#include <X11/Xlib.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_draw.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


struct _IdrawCanvas{
  Ihandle* ih;
  int w, h;

  Window wnd;
  Pixmap pixmap;
  GC pixmap_gc, gc;
};

static void motDrawGetGeometry(Display *dpy, Drawable wnd, int *_w, int *_h, int *_d)
{
  Window root;
  int x, y;
  unsigned int w, h, b, d;
  XGetGeometry(dpy, wnd, &root, &x, &y, &w, &h, &b, &d);
  *_w = w;
  *_h = h;
  *_d = d;
}

IdrawCanvas* iupDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  int depth;

  dc->wnd = XtWindow(ih->handle);
  dc->gc = XCreateGC(iupmot_display, dc->wnd, 0, NULL);

  motDrawGetGeometry(iupmot_display, dc->wnd, &dc->w, &dc->h, &depth);

  dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
  dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);

  return dc;
}

void iupDrawKillCanvas(IdrawCanvas* dc)
{
  XFreeGC(iupmot_display, dc->pixmap_gc);
  XFreePixmap(iupmot_display, dc->pixmap);
  XFreeGC(iupmot_display, dc->gc);

  free(dc);
}

void iupDrawFlush(IdrawCanvas* dc)
{
  XCopyArea(iupmot_display, dc->pixmap, dc->wnd, dc->gc, 0, 0, dc->w, dc->h, 0, 0);
}

void iupDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

void iupDrawParentBackground(IdrawCanvas* dc)
{
  unsigned char r=0, g=0, b=0;
  char* color = iupBaseNativeParentGetBgColorAttrib(dc->ih);
  iupStrToRGB(color, &r, &g, &b);
  iupDrawRectangle(dc, 0, 0, dc->w-1, dc->h-1, r, g, b, 1);
}

void iupDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b, int filled)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(r, g, b));
  if (filled)
    XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2-x1+1, y2-y1+1);
  else
    XDrawRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2-x1+1, y2-y1+1);
}
