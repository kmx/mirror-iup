/** \file
 * \brief iupgl control for Windows
 *
 * See Copyright Notice in iup.h
 */

#include <windows.h>
#include <GL/gl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_assert.h"
#include "iup_register.h"


struct _IcontrolData 
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */
  HDC device;
  HGLRC context;
  HPALETTE palette;
};

static int wGLCanvasDefaultResize_CB(Ihandle *ih, int width, int height)
{
  IupGLMakeCurrent(ih);
  glViewport(0,0,width,height);
  return IUP_DEFAULT;
}

static int wGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  free(ih->data); /* allocated by the iCanvasCreateMethod of IupCanvas */
  ih->data = iupALLOCCTRLDATA();
  IupSetCallback(ih, "RESIZE_CB", (Icallback)wGLCanvasDefaultResize_CB);
  return IUP_NOERROR;
}

static int wGLCanvasMapMethod(Ihandle* ih)
{
  Ihandle* ih_shared;
  int number;
  int isIndex = 0;
  int pixelFormat;
  PIXELFORMATDESCRIPTOR test_pfd;
  PIXELFORMATDESCRIPTOR pfd = { 
    sizeof(PIXELFORMATDESCRIPTOR),  /*  size of this pfd   */
      1,                     /* version number             */
      PFD_DRAW_TO_WINDOW |   /* support window             */
      PFD_SUPPORT_OPENGL,    /* support OpenGL             */
      PFD_TYPE_RGBA,         /* RGBA type                  */
      24,                    /* 24-bit color depth         */
      0, 0, 0, 0, 0, 0,      /* color bits ignored         */
      0,                     /* no alpha buffer            */
      0,                     /* shift bit ignored          */
      0,                     /* no accumulation buffer     */
      0, 0, 0, 0,            /* accum bits ignored         */
      16,                    /* 32-bit z-buffer             */
      0,                     /* no stencil buffer          */
      0,                     /* no auxiliary buffer        */
      PFD_MAIN_PLANE,        /* main layer                 */
      0,                     /* reserved                   */
      0, 0, 0                /* layer masks ignored        */
  };

  /* double or single buffer */
  if (iupStrEqualNoCase(IupGetAttribute(ih,"BUFFER"), "DOUBLE"))
    pfd.dwFlags |= PFD_DOUBLEBUFFER;

  /* stereo */
  if (IupGetInt(ih,"STEREO"))
    pfd.dwFlags |= PFD_STEREO;

  /* rgba or index */ 
  if (iupStrEqualNoCase(IupGetAttribute(ih,"COLOR"), "INDEX"))
  {
    isIndex = 1;
    pfd.iPixelType = PFD_TYPE_COLORINDEX;
    pfd.cColorBits = 8;  /* assume 8 bits when indexed */
    number = IupGetInt(ih,"BUFFER_SIZE");
    if (number > 0) pfd.cColorBits = (BYTE)number;
  }

  /* red, green, blue bits */
  number = IupGetInt(ih,"RED_SIZE");
  if (number > 0) pfd.cRedBits = (BYTE)number;
  pfd.cRedShift = 0;

  number = IupGetInt(ih,"GREEN_SIZE");
  if (number > 0) pfd.cGreenBits = (BYTE)number;
  pfd.cGreenShift = pfd.cRedBits;

  number = IupGetInt(ih,"BLUE_SIZE");
  if (number > 0) pfd.cBlueBits = (BYTE)number;
  pfd.cBlueShift = pfd.cRedBits + pfd.cGreenBits;

  number = IupGetInt(ih,"ALPHA_SIZE");
  if (number > 0) pfd.cAlphaBits = (BYTE)number;
  pfd.cAlphaShift = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits;

  /* depth and stencil size */
  number = IupGetInt(ih,"DEPTH_SIZE");
  if (number > 0) pfd.cDepthBits = (BYTE)number;

  /* stencil */
  number = IupGetInt(ih,"STENCIL_SIZE");
  if (number > 0) pfd.cStencilBits = (BYTE)number;

  /* red, green, blue accumulation bits */
  number = IupGetInt(ih,"ACCUM_RED_SIZE");
  if (number > 0) pfd.cAccumRedBits = (BYTE)number;

  number = IupGetInt(ih,"ACCUM_GREEN_SIZE");
  if (number > 0) pfd.cAccumGreenBits = (BYTE)number;

  number = IupGetInt(ih,"ACCUM_BLUE_SIZE");
  if (number > 0) pfd.cAccumBlueBits = (BYTE)number;

  number = IupGetInt(ih,"ACCUM_ALPHA_SIZE");
  if (number > 0) pfd.cAccumAlphaBits = (BYTE)number;

  pfd.cAccumBits = pfd.cAccumRedBits + pfd.cAccumGreenBits + pfd.cAccumBlueBits + pfd.cAccumAlphaBits;

  /* get a device context */
  ih->data->device = GetDC((HWND)IupGetAttribute(ih, "HWND"));

  /* choose pixel format */
  if ((pixelFormat = ChoosePixelFormat(ih->data->device, &pfd)) == 0) 
  {
    iupAttribSetStr(ih, "ERROR", "No appropriate pixel format.");
    return IUP_NOERROR;
  } 
  SetPixelFormat(ih->data->device,pixelFormat,&pfd);

  /* create rendering context */
  if ((ih->data->context = wglCreateContext(ih->data->device)) == NULL)
  {
    iupAttribSetStr(ih, "ERROR", "Could not create a rendering context.");
    return IUP_NOERROR;
  }

  ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared)
    wglShareLists(ih_shared->data->context, ih->data->context);

  DescribePixelFormat(ih->data->device, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &test_pfd);
  if ((pfd.dwFlags & PFD_STEREO) && !(test_pfd.dwFlags & PFD_STEREO))
  {
    iupAttribSetStr(ih, "ERROR", "Stereo not available.");
    return IUP_NOERROR;
  }

  /* create colormap for index mode */
  if (isIndex)
  {
    LOGPALETTE lp = {0x300,1,{255,255,255,PC_NOCOLLAPSE}};
    ih->data->palette = CreatePalette(&lp);
    ResizePalette(ih->data->palette,1<<pfd.cColorBits);
    SelectPalette(ih->data->device,ih->data->palette,FALSE);
    RealizePalette(ih->data->device);
  }

  iupAttribSetStr(ih, "COLORMAP", (char*)ih->data->palette);
  iupAttribSetStr(ih, "VISUAL", (char*)ih->data->device);
  iupAttribSetStr(ih, "CONTEXT", (char*)ih->data->context);

  return IUP_NOERROR;
}

static void wGLCanvasUnMapMethod(Ihandle* ih)
{
  if (ih->data->context)
  {
    if (ih->data->context == wglGetCurrentContext())
      wglMakeCurrent(NULL, NULL);

    wglDeleteContext(ih->data->context);
  }

  if (ih->data->palette)
    DeleteObject((HGDIOBJ)ih->data->palette);

  if (ih->data->device)
    ReleaseDC((HWND)IupGetAttribute(ih, "HWND"), ih->data->device);  /* can not use ih->handle, because it should work with GTK also */
}

static Iclass* wGlCanvasGetClass(void)
{
  Iclass* ic = iupClassNew(iupCanvasGetClass());

  ic->name = "glcanvas";
  ic->format = "A"; /* one optional callback name */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->Create = wGLCanvasCreateMethod;
  ic->Map = wGLCanvasMapMethod;
  ic->UnMap = wGLCanvasUnMapMethod;

  return ic;
}

/******************************************* Exported functions */

void IupGLCanvasOpen(void)
{
  iupRegisterClass(wGlCanvasGetClass());
}

Ihandle* IupGLCanvas(const char *action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("glcanvas", params);
}

int IupGLIsCurrent(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  /* must be a IupGLCanvas */
  if (!iupStrEqual(ih->iclass->name, "glcanvas"))
    return 0;

  /* must be mapped */
  if (!ih->handle)
    return 0;

  if (ih->data->context == wglGetCurrentContext())
    return 1;

  return 0;
}

void IupGLMakeCurrent(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be a IupGLCanvas */
  if (!iupStrEqual(ih->iclass->name, "glcanvas"))
    return;

  /* must be mapped */
  if (!ih->handle)
    return;

  wglMakeCurrent(ih->data->device, ih->data->context);
}

void IupGLSwapBuffers(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be a IupGLCanvas */
  if (!iupStrEqual(ih->iclass->name, "glcanvas"))
    return;

  /* must be mapped */
  if (!ih->handle)
    return;

  SwapBuffers(ih->data->device);
}

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be a IupGLCanvas */
  if (!iupStrEqual(ih->iclass->name, "glcanvas"))
    return;

  /* must be mapped */
  if (!ih->handle)
    return;

  /* must have a palette */
  if (ih->data->palette)
  {
    PALETTEENTRY entry;
    entry.peRed    = (BYTE)(r*255);
    entry.peGreen  = (BYTE)(g*255);
    entry.peBlue   = (BYTE)(b*255);
    entry.peFlags  = PC_NOCOLLAPSE;
    SetPaletteEntries(ih->data->palette,index,1,&entry);
    UnrealizeObject(ih->data->device);
    SelectPalette(ih->data->device,ih->data->palette,FALSE);
    RealizePalette(ih->data->device);
  }
}

