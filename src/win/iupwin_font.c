/** \file
 * \brief Windows Font mapping
 *
 * See Copyright Notice in iup.h
 */


#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_array.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"


typedef struct IwinFont_
{
  char standardfont[200];
  HFONT hfont;
  int charwidth, charheight;
} IwinFont;

static Iarray* win_fonts = NULL;

static IwinFont* winFindFont(const char *standardfont)
{
  HFONT hfont;
  int height_pixels;
  char typeface[50] = "";
  int height = 8;
  int is_bold = 0,
    is_italic = 0, 
    is_underline = 0,
    is_strikeout = 0;
  int res = iupwinGetScreenRes();
  int i, count = iupArrayCount(win_fonts);
  const char* mapped_name;

  /* Check if the standardfont already exists in cache */
  IwinFont* fonts = (IwinFont*)iupArrayGetData(win_fonts);
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(standardfont, fonts[i].standardfont))
      return &fonts[i];
  }

  /* parse the old format first */
  if (!iupFontParseWin(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParsePango(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return NULL;
  }

  mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  /* get size in pixels */
  if (height < 0)
    height_pixels = height;  /* already in pixels */
  else
    height_pixels = -IUPWIN_PT2PIXEL(height, res);

  if (height_pixels == 0)
    return NULL;

  hfont = CreateFont(height_pixels,
                        0,0,0,
                        (is_bold) ? FW_BOLD : FW_NORMAL,
                        is_italic,
                        is_underline,
                        is_strikeout,
                        DEFAULT_CHARSET,OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                        FF_DONTCARE|DEFAULT_PITCH,
                        typeface);
  if (!hfont)
    return NULL;

  /* create room in the array */
  fonts = (IwinFont*)iupArrayInc(win_fonts);

  strcpy(fonts[i].standardfont, standardfont);
  fonts[i].hfont = hfont;

  {
    TEXTMETRIC tm;
    HDC hdc = GetDC(NULL);
    HFONT oldfont = SelectObject(hdc, hfont);

    GetTextMetrics(hdc, &tm);

    SelectObject(hdc, oldfont);
    ReleaseDC(NULL, hdc);
    
    fonts[i].charwidth = tm.tmAveCharWidth; 
    fonts[i].charheight = tm.tmHeight;
  }

  return &fonts[i];
}

static void winFontFromLogFont(LOGFONT* logfont, char * font)
{
  int is_bold = (logfont->lfWeight == FW_NORMAL)? 0: 1;
  int is_italic = logfont->lfItalic;
  int is_underline = logfont->lfUnderline;
  int is_strikeout = logfont->lfStrikeOut;
  int height_pixels = logfont->lfHeight;
  int res = iupwinGetScreenRes();
  int height = IUPWIN_PIXEL2PT(-height_pixels, res);

  sprintf(font, "%s, %s%s%s%s %d", logfont->lfFaceName, 
                                   is_bold?"Bold ":"", 
                                   is_italic?"Italic ":"", 
                                   is_underline?"Underline ":"", 
                                   is_strikeout?"Strikeout ":"", 
                                   height);
}

char* iupdrvGetSystemFont(void)
{
  static char systemfont[200] = "";
  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, FALSE))
    winFontFromLogFont(&ncm.lfMessageFont, systemfont);
  else
    strcpy(systemfont, "Tahoma, 10");
  return systemfont;
}

HFONT iupwinFontGetNativeFont(const char* value)
{
  IwinFont* winfont = winFindFont(value);
  if (!winfont)
    return NULL;
  else
    return winfont->hfont;
}

static IwinFont* winFontCreateNativeFont(Ihandle *ih, const char* value)
{
  IwinFont* winfont = winFindFont(value);
  if (!winfont)
  {
    iupERROR1("Failed to create Font: %s", value); 
    return NULL;
  }

  iupAttribSetStr(ih, "_IUP_WINFONT", (char*)winfont);
  return winfont;
}

static IwinFont* winFontGet(Ihandle *ih)
{
  IwinFont* winfont = (IwinFont*)iupAttribGetStr(ih, "_IUP_WINFONT");
  if (!winfont)
    winfont = winFontCreateNativeFont(ih, iupGetFontAttrib(ih));
  return winfont;
}

char* iupwinGetHFontAttrib(Ihandle *ih)
{
  IwinFont* winfont = winFontGet(ih);
  if (!winfont)
    return NULL;
  else
    return (char*)winfont->hfont;
}

int iupdrvSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  IwinFont* winfont = winFontCreateNativeFont(ih, value);
  if (!winfont)
    return 1;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateSizeAttrib(ih);

  /* FONT attribute must be able to be set before mapping, 
      so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
    SendMessage(ih->handle, WM_SETFONT, (WPARAM)winfont->hfont, MAKELPARAM(TRUE,0));

  return 1;
}

static HDC winFontGetDC(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID)
    return GetDC(NULL);
  else
    return GetDC(ih->handle);  /* handle can be NULL here */
}

static void winFontReleaseDC(Ihandle* ih, HDC hdc)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID)
    ReleaseDC(NULL, hdc);
  else
    ReleaseDC(ih->handle, hdc);  /* handle can be NULL here */
}

void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  int num_lin, max_w;

  IwinFont* winfont = winFontGet(ih);
  if (!winfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = winfont->charheight * 1;
    return;
  }

  max_w = 0;
  num_lin = 1;
  if (str[0])
  {
    SIZE size;
    int len;
    const char *nextstr;
    const char *curstr = str;

    HDC hdc = winFontGetDC(ih);
    HFONT oldhfont = SelectObject(hdc, winfont->hfont);

    do
    {
      nextstr = iupStrNextLine(curstr, &len);
      GetTextExtentPoint32(hdc, curstr, len, &size);
      max_w = iupMAX(max_w, size.cx);

      curstr = nextstr;
      if (*nextstr)
        num_lin++;
    } while(*nextstr);

    SelectObject(hdc, oldhfont);
    winFontReleaseDC(ih, hdc);
  }

  if (w) *w = max_w;
  if (h) *h = winfont->charheight*num_lin;
}

int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  HDC hdc;
  HFONT oldhfont, hfont;
  SIZE size;
  int len;
  char* line_end;

  if (!str || str[0]==0)
    return 0;

  hfont = (HFONT)iupwinGetHFontAttrib(ih);
  if (!hfont)
    return 0;

  hdc = winFontGetDC(ih);
  oldhfont = SelectObject(hdc, hfont);

  line_end = strchr(str, '\n');
  if (line_end)
    len = line_end-str;
  else
    len = strlen(str);

  GetTextExtentPoint32(hdc, str, len, &size);

  SelectObject(hdc, oldhfont);
  winFontReleaseDC(ih, hdc);

  return size.cx;
}

void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
  IwinFont* winfont = winFontGet(ih);
  if (!winfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charwidth)  *charwidth = winfont->charwidth; 
  if (charheight) *charheight = winfont->charheight;
}

void iupdrvFontInit(void)
{
  win_fonts = iupArrayCreate(50, sizeof(IwinFont));
}

void iupdrvFontFinish(void)
{
  int i, count = iupArrayCount(win_fonts);
  IwinFont* fonts = (IwinFont*)iupArrayGetData(win_fonts);
  for (i = 0; i < count; i++)
  {
    DeleteObject(fonts[i].hfont);
    fonts[i].hfont = NULL;
  }
  iupArrayDestroy(win_fonts);
}
