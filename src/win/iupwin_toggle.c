/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in iup.h
 */

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"


void iupdrvToggleAddCheckBox(int *x, int *y)
{
  (*x) += 16+6;
  if ((*y) < 16) (*y) = 16; /* minimum height */
}

static int winToggleIsActive(Ihandle* ih)
{
  return iupAttribGetInt(ih, "_IUPWIN_ACTIVE");
}

static void winToggleSetBitmap(Ihandle* ih, const char* name, int make_inactive, const char* attrib_name)
{
  if (name)
  {
    HBITMAP bitmap = iupImageGetImage(name, ih, make_inactive, attrib_name);
    SendMessage(ih->handle, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bitmap);
  }
  else
    SendMessage(ih->handle, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);  /* if not defined */
}

static void winToggleUpdateImage(Ihandle* ih, int active, int check)
{
  /* called only when (ih->data->type == IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6) */
  char* name;

  if (!active)
  {
    name = iupAttribGetStr(ih, "IMINACTIVE");
    if (name)
      winToggleSetBitmap(ih, name, 0, "IMINACTIVE");
    else
    {
      /* if not defined then automaticaly create one based on IMAGE */
      name = iupAttribGetStr(ih, "IMAGE");
      winToggleSetBitmap(ih, name, 1, "IMINACTIVE"); /* make_inactive */
    }
  }
  else
  {
    /* must restore the normal image */
    if (check)
    {
      name = iupAttribGetStr(ih, "IMPRESS");
      if (name)
        winToggleSetBitmap(ih, name, 0, "IMPRESS");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        name = iupAttribGetStr(ih, "IMAGE");
        winToggleSetBitmap(ih, name, 0, "IMPRESS");
      }
    }
    else
    {
      name = iupAttribGetStr(ih, "IMAGE");
      if (name)
        winToggleSetBitmap(ih, name, 0, "IMAGE");
    }
  }
}

static void winToggleGetAlignment(Ihandle* ih, int *horiz_alignment, int *vert_alignment)
{
  char value1[30]="", value2[30]="";

  iupStrToStrStr(iupAttribGetStr(ih, "ALIGNMENT"), value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    *horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    *horiz_alignment = IUP_ALIGN_ALEFT;
  else /* "ACENTER" */
    *horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    *vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    *vert_alignment = IUP_ALIGN_ATOP;
  else /* "ACENTER" */
    *vert_alignment = IUP_ALIGN_ACENTER;
}

static void winToggleDrawImage(Ihandle* ih, HDC hDC, int rect_width, int rect_height, int border, UINT itemState)
{
  int xpad = ih->data->horiz_padding + border, 
      ypad = ih->data->vert_padding + border;
  int horiz_alignment, vert_alignment;
  int x, y, width, height, bpp, shift = 1;
  HBITMAP hBitmap;
  char *name, *attrib_name;
  int make_inactive = 0;

  if (itemState & ODS_DISABLED)
  {
    attrib_name = "IMINACTIVE";
    name = iupAttribGetStr(ih, "IMINACTIVE");
    if (!name)
    {
      name = iupAttribGetStr(ih, "IMAGE");
      make_inactive = 1;
    }
  }
  else
  {
    name = iupAttribGetStr(ih, "IMPRESS");
    if (itemState & ODS_SELECTED && name)
    {
      attrib_name = "IMPRESS";
      shift = 0;
    }
    else
    {
      attrib_name = "IMAGE";
      name = iupAttribGetStr(ih, "IMAGE");
    }
  }

  hBitmap = iupImageGetImage(name, ih, make_inactive, attrib_name);
  if (!hBitmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(hBitmap, &width, &height, &bpp);

  winToggleGetAlignment(ih, &horiz_alignment, &vert_alignment);
  if (horiz_alignment == IUP_ALIGN_ARIGHT)
    x = rect_width - (width + 2*xpad);
  else if (horiz_alignment == IUP_ALIGN_ACENTER)
    x = (rect_width - (width + 2*xpad))/2;
  else  /* ALEFT */
    x = 0;

  if (vert_alignment == IUP_ALIGN_ABOTTOM)
    y = rect_height - (height + 2*ypad);
  else if (vert_alignment == IUP_ALIGN_ATOP)
    y = 0;
  else  /* ACENTER */
    y = (rect_height - (height + 2*ypad))/2;

  x += xpad;
  y += ypad;

  if (itemState & ODS_SELECTED && !iupwin_comctl32ver6 && shift)
  {
    x++;
    y++;
  }

  iupwinDrawBitmap(hDC, hBitmap, x, y, width, height, bpp);
}

static void winToggleDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  int width, height, border = 4, check;
  HDC hDC;
  iupwinBitmapDC bmpDC;

  width = drawitem->rcItem.right - drawitem->rcItem.left;
  height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, width, height);

  iupwinDrawParentBackground(ih, hDC, &drawitem->rcItem);

  check = SendMessage(ih->handle, BM_GETCHECK, 0, 0L);
  if (check)
    drawitem->itemState |= ODS_SELECTED;
  else
    drawitem->itemState |= ODS_DEFAULT;  /* use default mark for NOT checked */

  iupwinDrawButtonBorder(ih->handle, hDC, &drawitem->rcItem, drawitem->itemState);

  winToggleDrawImage(ih, hDC, width, height, border, drawitem->itemState);

  if (drawitem->itemState & ODS_FOCUS)
  {
    border--;
    iupdrvDrawFocusRect(ih, hDC, border, border, width-2*border, height-2*border);
  }

  iupwinDrawDestroyBitmapDC(&bmpDC);
}


/***********************************************************************************************/


static int winToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMAGE"))
      iupAttribSetStr(ih, "IMAGE", (char*)value);

    if (iupwin_comctl32ver6)
      iupwinRedrawNow(ih);
    else
    {
      int check = SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMINACTIVE"))
      iupAttribSetStr(ih, "IMINACTIVE", (char*)value);

    if (iupwin_comctl32ver6)
      iupwinRedrawNow(ih);
    else
    {
      int check = SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMPRESS"))
      iupAttribSetStr(ih, "IMPRESS", (char*)value);

    if (iupwin_comctl32ver6)
      iupwinRedrawNow(ih);
    else
    {
      int check = SendMessage(ih->handle, BM_GETCHECK, 0L, 0L);
      winToggleUpdateImage(ih, winToggleIsActive(ih), check);
    }
    return 1;
  }
  else
    return 0;
}

static int winToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *radio;
  int check;

  if (iupStrEqualNoCase(value,"NOTDEF"))
    check = BST_INDETERMINATE;
  else if (iupStrBoolean(value))
    check = BST_CHECKED;
  else
    check = BST_UNCHECKED;

  /* This is necessary because Windows does not handle the radio state 
     when a toggle is programatically changed. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    int oldcheck = (int)SendMessage(ih->handle, BM_GETCHECK, 0, 0L);

    Ihandle* last_tg = (Ihandle*)iupAttribGetStr(radio, "_IUPWIN_LASTTOGGLE");
    if (check)
    {
      if (iupObjectCheck(last_tg) && last_tg != ih)
          SendMessage(last_tg->handle, BM_SETCHECK, BST_UNCHECKED, 0L);
      iupAttribSetStr(radio, "_IUPWIN_LASTTOGGLE", (char*)ih);
    }

    if (last_tg != ih && oldcheck != check)
      SendMessage(ih->handle, BM_SETCHECK, check, 0L);
  }
  else
    SendMessage(ih->handle, BM_SETCHECK, check, 0L);

  if (ih->data->type == IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6)
    winToggleUpdateImage(ih, winToggleIsActive(ih), check);

  return 0;
}

static char* winToggleGetValueAttrib(Ihandle* ih)
{
  int check = (int)SendMessage(ih->handle, BM_GETCHECK, 0, 0L);
  if (check == BST_INDETERMINATE)
    return "NOTDEF";
  else if (check == BST_CHECKED)
    return "ON";
  else
    return "OFF";
}

static int winToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (iupwin_comctl32ver6)
    {
      iupBaseSetActiveAttrib(ih, value);
      iupwinRedrawNow(ih);
      return 0;
    }
    else
    {
      int active = iupStrBoolean(value);
      int check = SendMessage(ih->handle, BM_GETCHECK, 0, 0L);
      if (active)
        iupAttribSetStr(ih, "_IUPWIN_ACTIVE", "YES");
      else
        iupAttribSetStr(ih, "_IUPWIN_ACTIVE", "NO");
      winToggleUpdateImage(ih, active, check);
      return 0;
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static char* winToggleGetActiveAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6)
    return iupAttribGetStr(ih, "_IUPWIN_ACTIVE");
  else
    return iupBaseGetActiveAttrib(ih);
}

static int winToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    if (!value)
      value = "";
    SetWindowText(ih->handle, value);
  }
  return 0;
}

static int winToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && iupwin_comctl32ver6 && ih->data->type == IUP_TOGGLE_IMAGE)
    iupwinRedrawNow(ih);

  return 0;
}

static int winToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    /* update internal image cache for controls that have the IMAGE attribute */
    iupImageUpdateParent(ih);
    iupwinRedrawNow(ih);
  }
  return 1;
}

static char* winToggleGetBgColorAttrib(Ihandle* ih)
{
  /* the most important use of this is to provide
     the correct background for images */
  if (iupwin_comctl32ver6 && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    COLORREF cr;
    if (iupwinDrawGetThemeButtonBgColor(ih->handle, &cr))
    {
      char* str = iupStrGetMemory(20);
      sprintf(str, "%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
      return str;
    }
  }

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
    return IupGetGlobal("DLGBGCOLOR");
}


/****************************************************************************************/


static int winToggleCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  COLORREF cr;

  SetBkMode(hdc, TRANSPARENT);

  if (iupwinGetColorRef(ih, "FGCOLOR", &cr))
    SetTextColor(hdc, cr);

  if (iupwinGetParentBgColor(ih, &cr))
  {
    SetDCBrushColor(hdc, cr);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static int winToggleWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  if (msg_info->code == NM_CUSTOMDRAW)
  {
    /* called only when iupwin_comctl32ver6 AND (ih->data->type==IUP_TOGGLE_IMAGE) */
    NMCUSTOMDRAW *customdraw = (NMCUSTOMDRAW*)msg_info;

    if (customdraw->dwDrawStage==CDDS_PREERASE)
    {
      DRAWITEMSTRUCT drawitem;
      drawitem.itemState = 0;

      if (customdraw->uItemState & CDIS_DISABLED)
        drawitem.itemState |= ODS_DISABLED;
      else if (customdraw->uItemState & CDIS_SELECTED)
        drawitem.itemState |= ODS_SELECTED;
      else if (customdraw->uItemState & CDIS_HOT)
        drawitem.itemState |= ODS_HOTLIGHT;
      else if (customdraw->uItemState & CDIS_DEFAULT)
        drawitem.itemState |= ODS_DEFAULT;

      if (customdraw->uItemState & CDIS_FOCUS)
        drawitem.itemState |= ODS_FOCUS;

      drawitem.hDC = customdraw->hdc;
      drawitem.rcItem = customdraw->rc;

      winToggleDrawItem(ih, (void*)&drawitem);  /* Simulate a WM_DRAWITEM */

      *result = CDRF_SKIPDEFAULT;
      return 1;
    }
  }

  return 0; /* result not used */
}

static int winToggleProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  (void)lp;
  (void)wp;

  switch (msg)
  {
    case WM_MOUSEACTIVATE:
      if (!winToggleIsActive(ih))
      {
        *result = MA_NOACTIVATEANDEAT;
        return 1;
      }
      break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_ACTIVATE:
    case WM_SETFOCUS:
      if (!winToggleIsActive(ih)) 
      {
        *result = 0;
        return 1;
      }
      break;
  }

  if (msg == WM_LBUTTONDOWN)
    winToggleUpdateImage(ih, 1, 1);
  else if (msg == WM_LBUTTONUP)
    winToggleUpdateImage(ih, 1, 0);

  return iupwinBaseProc(ih, msg, wp, lp, result);
}

static int winToggleWmCommand(Ihandle* ih, WPARAM wp, LPARAM lp)
{
  (void)lp;

  switch (HIWORD(wp))
  {
  case BN_DOUBLECLICKED:
  case BN_CLICKED:
    {
      Ihandle *radio;
      IFni cb;
      int check = SendMessage(ih->handle, BM_GETCHECK, 0, 0L);

      if (ih->data->type == IUP_TOGGLE_IMAGE && !iupwin_comctl32ver6)
      {
        int active = winToggleIsActive(ih);
        winToggleUpdateImage(ih, active, check);
        if (!active)
          return 0;
      }

      /* This is necessary because Windows does not send a message
         when a toggle is unchecked in a Radio. 
         Also if the toggle is already checked in a radio, 
         a click will call the callback again. */
      radio = iupRadioFindToggleParent(ih);
      if (radio)
      {
        if (check)  /* check here will be always 1, but just in case */
        {
          Ihandle* last_tg = (Ihandle*)iupAttribGetStr(radio, "_IUPWIN_LASTTOGGLE");
          if (iupObjectCheck(last_tg) && last_tg != ih)
          {
            cb = (IFni) IupGetCallback(last_tg, "ACTION");
            if (cb && cb(last_tg, 0) == IUP_CLOSE)
                IupExitLoop();

            /* not necessary, but does no harm, just to make sure */
            SendMessage(last_tg->handle, BM_SETCHECK, BST_UNCHECKED, 0L);
          }
          iupAttribSetStr(radio, "_IUPWIN_LASTTOGGLE", (char*)ih);

          if (last_tg != ih)
          {
            cb = (IFni)IupGetCallback(ih, "ACTION");
            if (cb && cb (ih, 1) == IUP_CLOSE)
                IupExitLoop();
          }
        }
      }
      else
      {
        if (check == BST_INDETERMINATE)
          check = -1;

        cb = (IFni)IupGetCallback(ih, "ACTION");
        if (cb && cb (ih, check) == IUP_CLOSE)
            IupExitLoop();
      }
    }
  }


  return 0; /* not used */
}

static int winToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;
  DWORD dwStyle = WS_CHILD | 
                  BS_NOTIFY; /* necessary because of the base messages */

  if (!ih->parent)
    return IUP_ERROR;

  if (radio)
    ih->data->radio = 1;

  value = iupAttribGetStr(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_TOGGLE_IMAGE;
    dwStyle |= BS_BITMAP|BS_PUSHLIKE;
  }
  else
  {
    ih->data->type = IUP_TOGGLE_TEXT;
    dwStyle |= BS_TEXT|BS_MULTILINE;

    if (iupStrBoolean(iupAttribGetStr(ih, "RIGHTBUTTON")))
      dwStyle |= BS_RIGHTBUTTON;
  }

  if (radio)
  {
    dwStyle |= BS_AUTORADIOBUTTON;
    /* Do not set TABSTOP for toggles in a radio, the system will automaticaly set them. */

    if (!iupAttribGetStr(radio, "_IUPWIN_LASTTOGGLE"))
    {
      /* this is the first toggle in the radio, and the last toggle with VALUE=ON */
      iupAttribSetStr(ih, "VALUE","ON");

      dwStyle |= WS_TABSTOP;
      dwStyle |= WS_GROUP; /* this is specified only for the first toggle in the radio. But necessary. */
                           /* this affects keyboard navigation in the dialog for the arrow keys */
                           /* it will form a group up to the next WS_GROUP. */
                           /* this can be ignored only if the toggles are inside a frame using BS_GROUPBOX */
    }
  }
  else
  {
    if (iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
      dwStyle |= WS_TABSTOP;

    if (ih->data->type == IUP_TOGGLE_TEXT && iupAttribGetIntDefault(ih, "3STATE"))
      dwStyle |= BS_AUTO3STATE;
    else
      dwStyle |= BS_AUTOCHECKBOX;
  }

  /* used WS_EX_TRANSPARENT because RADIO in GROUPBOX showed a black background */
  if (!iupwinCreateWindowEx(ih, "BUTTON", WS_EX_TRANSPARENT, dwStyle))
    return IUP_ERROR;

  /* Process WM_COMMAND */
  IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winToggleWmCommand);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winToggleCtlColor);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (iupwin_comctl32ver6)
      IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winToggleWmNotify);  /* Process WM_NOTIFY */
    else
    {
      IupSetCallback(ih, "_IUPWIN_CTRLPROC_CB", (Icallback)winToggleProc);
      iupAttribSetStr(ih, "_IUPWIN_ACTIVE", "YES");
    }
  }

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winToggleMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", winToggleGetActiveAttrib, winToggleSetActiveAttrib, "YES", IUP_MAPPED, IUP_INHERIT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", winToggleGetBgColorAttrib, winToggleSetBgColorAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_INHERIT);  

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, "DLGFGCOLOR", IUP_NOT_MAPPED, IUP_INHERIT);  /* usually black */  
  iupClassRegisterAttribute(ic, "TITLE", iupdrvBaseGetTitleAttrib, winToggleSetTitleAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, NULL, "ACENTER:ACENTER", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winToggleSetImageAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, winToggleSetImInactiveAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winToggleSetImPressAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", winToggleGetValueAttrib, winToggleSetValueAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, winToggleSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);

  if (!iupwin_comctl32ver6)  /* Used by iupdrvImageCreateImage */
    iupClassRegisterAttribute(ic, "FLAT_ALPHA", NULL, NULL, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
}
