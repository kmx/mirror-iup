/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int y, x, bpp, bgcolor_depend = 0,
      width = ih->currentwidth,
      height = ih->currentheight;
  unsigned char *imgdata = (unsigned char*)ih->handle;
  Pixmap pixmap;
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  GC gc;
  Pixel color2pixel[256];

  bpp = iupAttribGetInt(ih, "BPP");

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  if (bpp == 8)
  {
    int i, colors_count = 0;
    iupColor colors[256];

    iupImageInitColorTable(ih, colors, &colors_count);

    for (i=0;i<colors_count;i++)
    {
      if (colors[i].a == 0)
      {
        colors[i].r = bg_r;
        colors[i].g = bg_g;
        colors[i].b = bg_b;
        colors[i].a = 255;
        bgcolor_depend = 1;
      }

      if (make_inactive)
          iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), 
                                    bg_r, bg_g, bg_b);

      color2pixel[i] = iupmotColorGetPixel(colors[i].r, colors[i].g, colors[i].b);
    }
  }

  pixmap = XCreatePixmap(iupmot_display,
          RootWindow(iupmot_display,iupmot_screen),
          width, height, iupdrvGetScreenDepth());
  if (!pixmap)
    return NULL;

  gc = XCreateGC(iupmot_display,pixmap,0,NULL);
  for (y=0;y<height;y++)
  {
    for(x=0;x<width;x++)
    {
      unsigned long p;
      if (bpp == 8)
        p = color2pixel[imgdata[y*width+x]];
      else
      {
        int channels = (bpp==24)? 3: 4;
        unsigned char *pixel_data = imgdata + y*width*channels + x*channels;
        unsigned char r = *(pixel_data),
                      g = *(pixel_data+1),
                      b = *(pixel_data+2);

        if (bpp == 32)
        {
          unsigned char a = *(pixel_data+3);
          if (a != 255)
          {
            r = iupALPHABLEND(r, bg_r, a);
            g = iupALPHABLEND(g, bg_g, a);
            b = iupALPHABLEND(b, bg_b, a);
            bgcolor_depend = 1;
          }
        }

        if (make_inactive)
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);

        p = iupmotColorGetPixel(r, g, b);
      }

      XSetForeground(iupmot_display,gc,p);
      XDrawPoint(iupmot_display,pixmap,gc,x,y);
    }
  }
  XFreeGC(iupmot_display,gc);

  if (bgcolor_depend || make_inactive)
    iupAttribSetStr(ih, "BGCOLOR_DEPEND", "1");

 return (void*)pixmap;
}

void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle *ih)
{
  int bpp,y,x,hx,hy,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)ih->handle;
  char *sbits, *mbits, *sb, *mb;
  Pixmap source, mask;
  XColor fg, bg;
  unsigned char r, g, b;
  Cursor cursor;

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  sbits = (char*)malloc(2*size_bytes);
  if (!sbits) return (Cursor)NULL;
  memset(sbits, 0, 2*size_bytes);
  mbits = sbits + size_bytes;

  sb = sbits;
  mb = mbits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int cor = (int)imgdata[y*width+x];
      if (cor == 1)
        sb[byte] = (char)(sb[byte] | (1<<bit));
      if (cor != 0)
        mb[byte] = (char)(mb[byte] | (1<<bit));
    }

    sb += line_size;
    mb += line_size;
  }

  r = 255; g = 255; b = 255;
  iupStrToRGB(iupAttribGetStr(ih, "1"), &r, &g, &b );
  fg.red   = iupCOLOR8TO16(r);
  fg.green = iupCOLOR8TO16(g);
  fg.blue  = iupCOLOR8TO16(b);
  fg.flags = DoRed | DoGreen | DoBlue;

  r = 0; g = 0; b = 0;
  iupStrToRGB(iupAttribGetStr(ih, "2"), &r, &g, &b );
  bg.red   = iupCOLOR8TO16(r);
  bg.green = iupCOLOR8TO16(g);
  bg.blue  = iupCOLOR8TO16(b);
  bg.flags = DoRed | DoGreen | DoBlue;

  hx=0; hy=0;
  iupStrToIntInt(iupAttribGetStr(ih, "HOTSPOT"), &hx, &hy, ':');

  source = XCreateBitmapFromData(iupmot_display, 
             RootWindow(iupmot_display,iupmot_screen),
             sbits, width, height);
  mask   = XCreateBitmapFromData(iupmot_display, 
             RootWindow(iupmot_display,iupmot_screen),
             mbits, width, height);

  cursor = XCreatePixmapCursor(iupmot_display, source, mask, &fg, &bg, hx, hy);

  free(sbits);
  return (void*)cursor;
}

void* iupdrvImageLoad(const char* name, int type)
{
  if (type == IUPIMAGE_CURSOR)
  {
    Cursor cursor = 0;
    int id;
    if (iupStrToInt(name, &id))
      cursor = XCreateFontCursor(iupmot_display, id);
    return (void*)cursor;
  }
  else
    return NULL;
}

void iupdrvImageGetInfo(void* image, int *w, int *h, int *bpp)
{
  Pixmap pixmap = (Pixmap)image;
  Window root;
  int x, y;
  unsigned int width, height, b, depth;
  XGetGeometry(iupmot_display, pixmap, &root, &x, &y, &width, &height, &b, &depth);
  if (w) *w = width;
  if (h) *h = height;
  if (bpp) *bpp = depth;
}

void iupdrvImageDestroy(void* image, int type)
{
  if (type == IUPIMAGE_CURSOR)
    XFreeCursor(iupmot_display, (Cursor)image);
  else
    XFreePixmap(iupmot_display, (Pixmap)image);
}
