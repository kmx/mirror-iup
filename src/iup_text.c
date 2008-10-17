/** \file
 * \brief Text Control.
 *
 * See Copyright Notice in iup.ih
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_assert.h"


static void iTextDestroyFormatTags(Ihandle* ih)
{
  /* called if the element was destroyed before it was mapped */
  int i, count = iupArrayCount(ih->data->formattags);
  Ihandle** tag_array = (Ihandle**)iupArrayGetData(ih->data->formattags);
  for (i = 0; i < count; i++)
    IupDestroy(tag_array[i]);
  iupArrayDestroy(ih->data->formattags);
  ih->data->formattags = NULL;
}

static void iTextUpdateValueAttrib(Ihandle* ih)
{
  char* value = iupAttribGetStr(ih, "VALUE");
  if (value)
  {
    int inherit;
    iupClassObjectSetAttribute(ih, "VALUE", value, &inherit);

    iupAttribSetStr(ih, "VALUE", NULL); /* clear hash table */
  }
}

char* iupTextGetNCAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(100);
  sprintf(str, "%d", ih->data->nc);
  return str;
}

void iupTextUpdateFormatTags(Ihandle* ih)
{
  /* called when the element is mapped */
  int i, count = iupArrayCount(ih->data->formattags);
  Ihandle** tag_array = (Ihandle**)iupArrayGetData(ih->data->formattags);

  /* must update VALUE before updating the format */
  iTextUpdateValueAttrib(ih);

  for (i = 0; i < count; i++)
  {
    iupdrvTextAddFormatTag(ih, tag_array[i]);
    IupDestroy(tag_array[i]);
  }
  iupArrayDestroy(ih->data->formattags);
  ih->data->formattags = NULL;
}

static int iTextSetAddFormatTagHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* formattag = (Ihandle*)value;
  if (!iupObjectCheck(formattag))
    return 0;

  if (ih->handle)
  {
    /* must update VALUE before updating the format */
    iTextUpdateValueAttrib(ih);

    iupdrvTextAddFormatTag(ih, formattag);
    IupDestroy(formattag);
  }
  else
  {
    Ihandle** tag_array;
    int i;

    if (!ih->data->formattags)
      ih->data->formattags = iupArrayCreate(10, sizeof(Ihandle*));

    i = iupArrayCount(ih->data->formattags);
    tag_array = (Ihandle**)iupArrayInc(ih->data->formattags);
    tag_array[i] = formattag;
  }
  return 0;
}

static int iTextSetAddFormatTagAttrib(Ihandle* ih, const char* value)
{
  return iTextSetAddFormatTagHandleAttrib(ih, (char*)IupGetHandle(value));
}

static char* iTextGetMaskDataAttrib(Ihandle* ih)
{
  /* Used only by the OLD iupmask API */
  return (char*)ih->data->mask;
}

static int iTextSetMaskAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    if (ih->data->mask)
      iupMaskDestroy(ih->data->mask);
  }
  else
  {
    int casei = iupAttribGetInt(ih, "MASKCASEI");
    Imask* mask = iupMaskCreate(value,casei);
    if (mask)
    {
      if (ih->data->mask)
        iupMaskDestroy(ih->data->mask);

      ih->data->mask = mask;
      return 1;
    }
  }

  return 0;
}

static int iTextSetMaskIntAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    if (ih->data->mask)
      iupMaskDestroy(ih->data->mask);

    iupAttribSetStr(ih, "MASK", NULL);
  }
  else
  {
    Imask* mask;
    int min, max;

    if (iupStrToIntInt(value, &min, &max, ':')!=2)
      return 0;

    mask = iupMaskCreateInt(min,max);

    if (ih->data->mask)
      iupMaskDestroy(ih->data->mask);

    ih->data->mask = mask;

    if (min < 0)
      iupAttribSetStr(ih, "MASK", IUPMASK_INT);
    else
      iupAttribSetStr(ih, "MASK", IUPMASK_UINT);
  }

  return 0;
}

static int iTextSetMaskFloatAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    if (ih->data->mask)
      iupMaskDestroy(ih->data->mask);

    iupAttribSetStr(ih, "MASK", NULL);
  }
  else
  {
    Imask* mask;
    float min, max;

    if (iupStrToFloatFloat(value, &min, &max, ':')!=2)
      return 0;

    mask = iupMaskCreateFloat(min,max);

    if (ih->data->mask)
      iupMaskDestroy(ih->data->mask);

    ih->data->mask = mask;

    if (min < 0)
      iupAttribSetStr(ih, "MASK", IUPMASK_FLOAT);
    else
      iupAttribSetStr(ih, "MASK", IUPMASK_UFLOAT);
  }

  return 0;
}

static int iTextSetMultilineAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->is_multiline = 1;
    ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* reset SCROLLBAR to YES */
  }
  else
    ih->data->is_multiline = 0;

  return 0;
}

static char* iTextGetMultilineAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
    return "YES";
  else
    return "NO";
}

static int iTextSetAppendNewlineAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->append_newline = 1;
  else
    ih->data->append_newline = 0;
  return 0;
}

static char* iTextGetAppendNewlineAttrib(Ihandle* ih)
{
  if (ih->data->append_newline)
    return "YES";
  else
    return "NO";
}

static int iTextSetScrollbarAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle || !ih->data->is_multiline)
    return 0;

  if (!value)
    value = "YES";    /* default, if multiline, is YES */

  if (iupStrEqualNoCase(value, "YES"))
    ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;
  else if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->sb = IUP_SB_HORIZ;
  else if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->sb = IUP_SB_VERT;
  else
    ih->data->sb = IUP_SB_NONE;

  return 0;
}

static char* iTextGetScrollbarAttrib(Ihandle* ih)
{
  if (ih->data->sb == (IUP_SB_HORIZ | IUP_SB_VERT))
    return "YES";
  if (ih->data->sb &= IUP_SB_HORIZ)
    return "HORIZONTAL";
  if (ih->data->sb == IUP_SB_VERT)
    return "VERTICAL";
  return "NO";   /* IUP_SB_NONE */
}

char* iupTextGetPaddingAttrib(Ihandle* ih)
{
  char *str = iupStrGetMemory(50);
  sprintf(str, "%dx%d", ih->data->horiz_padding, ih->data->vert_padding);
  return str;
}


/********************************************************************/


static int iTextCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribStoreStr(ih, "ACTION", (char*)(params[0]));
  }
  ih->data = iupALLOCCTRLDATA();
  ih->data->append_newline = 1;
  return IUP_NOERROR;
}

static int iMultilineCreateMethod(Ihandle* ih, void** params)
{
  iTextCreateMethod(ih, params);
  ih->data->is_multiline = 1;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;
  return IUP_NOERROR;
}

static void iTextComputeNaturalSizeMethod(Ihandle* ih)
{
  /* always initialize the natural size using the user size */
  ih->naturalwidth = ih->userwidth;
  ih->naturalheight = ih->userheight;

  /* if user size is not defined, then calculate the natural size */
  if ((ih->naturalwidth <= 0  && !(ih->expand & IUP_EXPAND_WIDTH)) || 
      (ih->naturalheight <= 0 && !(ih->expand & IUP_EXPAND_HEIGHT)))
  {
    int natural_w = 0, 
        natural_h = 0,
        visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS"),
        visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    /* must use IupGetAttribute to check from the native implementation */
    char* value = IupGetAttribute(ih, "VALUE");
    if (!value || value[0]==0) value = "WWWWW";
    if (ih->data->is_multiline)
    {
      if (visiblecolumns && visiblelines)
      {
        iupdrvFontGetCharSize(ih, &natural_w, &natural_h);
        natural_w = visiblecolumns*natural_w;
        natural_h = visiblelines*natural_h;
      }
      else if (visiblecolumns || visiblelines)
      {
        iupdrvFontGetCharSize(ih, &natural_w, &natural_h);
        if (visiblecolumns)
        {
          natural_w = visiblecolumns*natural_w;
          natural_h = iupStrLineCount(value)*natural_h;
        }
        else
        {
          iupdrvFontGetMultiLineStringSize(ih, value, &natural_w, NULL);
          natural_h = visiblelines*natural_h;
        }
      }
      else
        iupdrvFontGetMultiLineStringSize(ih, value, &natural_w, &natural_h);
    }
    else
    {
      iupdrvFontGetCharSize(ih, &natural_w, &natural_h);
      if (visiblecolumns)
        natural_w = visiblecolumns*natural_w;
      else
        natural_w = iupdrvFontGetStringWidth(ih, value);
    }

    /* compute the borders space */
    if (IupGetInt(ih, "BORDER"))              /* Use IupGetInt for inheritance */
      iupdrvTextAddBorders(&natural_w, &natural_h);

    natural_w += 2*ih->data->horiz_padding;
    natural_h += 2*ih->data->vert_padding;

    /* add scrollbar */
    if (ih->data->is_multiline && ih->data->sb)
    {
      int sb_size = iupdrvGetScrollbarSize();
      if (ih->data->sb & IUP_SB_HORIZ)
        natural_w += sb_size;
      if (ih->data->sb & IUP_SB_VERT)
        natural_h += sb_size;
    }

    /* For IupText only update the natural size 
       if user size is not defined and expand is NOT set. */
    if (ih->naturalwidth <= 0  && !(ih->expand & IUP_EXPAND_WIDTH)) ih->naturalwidth = natural_w;
    if (ih->naturalheight <= 0 && !(ih->expand & IUP_EXPAND_HEIGHT)) ih->naturalheight = natural_h;
  }
}

static void iTextDestroyMethod(Ihandle* ih)
{
  if (ih->data->formattags)
    iTextDestroyFormatTags(ih);
  if (ih->data->mask)
    iupMaskDestroy(ih->data->mask);
}


/******************************************************************************/


void IupTextConvertXYToChar(Ihandle* ih, int x, int y, int *lin, int *col, int *pos)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!ih->handle)
    return;
    
  if (iupStrEqual(ih->iclass->name, "text") ||
      iupStrEqual(ih->iclass->name, "multiline"))
  {
    int txt_lin, txt_col, txt_pos;
    iupdrvTextConvertXYToChar(ih, x, y, &txt_lin, &txt_col, &txt_pos);
    if (lin) *lin = txt_lin;
    if (col) *col = txt_col;
    if (pos) *pos = txt_pos;
  }
}

Ihandle* IupText(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("text", params);
}

Ihandle* IupMultiLine(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("multiline", params);
}

Iclass* iupTextGetClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "text";
  ic->format = "S"; /* one optional strings */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->Create = iTextCreateMethod;
  ic->Destroy = iTextDestroyMethod;
  ic->ComputeNaturalSize = iTextComputeNaturalSizeMethod;

  ic->SetCurrentSize = iupBaseSetCurrentSizeMethod;
  ic->SetPosition = iupBaseSetPositionMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "CARET_CB", "iii");
  iupClassRegisterCallback(ic, "ACTION", "is");
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");
  iupClassRegisterCallback(ic, "BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "SPIN_CB", "i");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupText only */
  iupClassRegisterAttribute(ic, "SCROLLBAR", iTextGetScrollbarAttrib, iTextSetScrollbarAttrib, NULL, IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", iTextGetMultilineAttrib, iTextSetMultilineAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPENDNEWLINE", iTextGetAppendNewlineAttrib, iTextSetAppendNewlineAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASK", NULL, iTextSetMaskAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKINT", NULL, iTextSetMaskIntAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MASKFLOAT", NULL, iTextSetMaskFloatAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "_IUPMASK_DATA", iTextGetMaskDataAttrib, iupBaseNoSetAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iTextSetAddFormatTagAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iTextSetAddFormatTagHandleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINAUTO", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);

  iupdrvTextInitClass(ic);

  return ic;
}

Iclass* iupMultilineGetClass(void)
{
  Iclass* ic = iupTextGetClass();
  ic->Create = iMultilineCreateMethod;
  ic->name = "multiline";   /* register the multiline name, so LED will work */
  return ic;
}
