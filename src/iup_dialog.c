/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_dlglist.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"


static int dlg_popup_level = 1;

InativeHandle* iupDialogGetNativeParent(Ihandle* ih)
{
  Ihandle* parent = IupGetAttributeHandle(ih, "PARENTDIALOG");
  if (parent && parent->handle)
    return parent->handle;
  else
    return (InativeHandle*)iupAttribGetStr(ih, "NATIVEPARENT");
}
static void iDialogSetModal(Ihandle* ih_popup)
{
  Ihandle *ih;
  iupAttribSetStr(ih_popup, "MODAL", "YES");

  /* disable all visible dialogs, and mark popup level */
  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    if (ih != ih_popup && 
        ih->handle &&
        iupdrvIsVisible(ih) && 
        ih->data->popup_level == 0)
    {
      iupdrvSetActive(ih, 0);
      ih->data->popup_level = dlg_popup_level;
    }
  }

  dlg_popup_level++;
}

static void iDialogUnSetModal(Ihandle* ih_popup)
{
  Ihandle *ih;
  if (!iupAttribGetInt(ih_popup, "MODAL"))
    return;

  iupAttribSetStr(ih_popup, "MODAL", NULL);

  /* must enable all visible dialogs at the marked popup level */
  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    if (ih->handle)
    {
      if (ih->data->popup_level == dlg_popup_level-1)
      {
        iupdrvSetActive(ih, 1);
        ih->data->popup_level = 0;
      }
    }
  }

  dlg_popup_level--;
}

static int iDialogCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  iupDlgListAdd(ih);

  return IUP_NOERROR;
}

static void iDialogDestroyMethod(Ihandle* ih)
{
  iupDlgListRemove(ih);
}

static void iDialogComputeNaturalSizeMethod(Ihandle* ih)
{
  iupBaseContainerUpdateExpand(ih);

  /* always initialize the natural size using the user size */
  ih->naturalwidth = ih->userwidth;
  ih->naturalheight = ih->userheight;

  /* the dialog has only one child      */
  if (ih->firstchild)
  {
    int decorwidth, decorheight;
    Ihandle* child = ih->firstchild;

    iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

    /* update child natural size first */
    iupClassObjectComputeNaturalSize(child);

    ih->expand |= child->expand;
    ih->naturalwidth = iupMAX(ih->naturalwidth, child->naturalwidth + decorwidth);
    ih->naturalheight = iupMAX(ih->naturalheight, child->naturalheight + decorheight);
  }
}

static void iDialogSetCurrentSizeMethod(Ihandle* ih, int w, int h, int shrink)
{
  /* width and height parameters here are ignored, because they are always 0 for the dialog. */
  (void)w;
  (void)h;

  /* current size is zero before map and when reset by the application */
  /* after that the current size must follow the actual size of the dialog */

  if (!ih->currentwidth)
    ih->currentwidth = ih->naturalwidth;

  if (!ih->currentheight)
    ih->currentheight = ih->naturalheight;

  if (ih->firstchild)
  {
    int decorwidth, decorheight, client_width, client_height;

    if (shrink)
    {
      client_width = ih->currentwidth;
      client_height = ih->currentheight;
    }
    else
    {
      client_width = iupMAX(ih->naturalwidth, ih->currentwidth);
      client_height = iupMAX(ih->naturalheight, ih->currentheight);
    }

    iupDialogGetDecorSize(ih, &decorwidth, &decorheight);
    client_width -= decorwidth;
    client_height -= decorheight;

    iupClassObjectSetCurrentSize(ih->firstchild, client_width, client_height, shrink);
  }
}

static void iDialogCallShowCb(Ihandle* ih)
{
  IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (show_cb && show_cb(ih, ih->data->show_state) == IUP_CLOSE)
    IupExitLoop();

  if (ih->data->show_state == IUP_SHOW)
  {
    Ihandle *startfocus = IupGetAttributeHandle(ih, "STARTFOCUS");
    if (startfocus)
      IupSetFocus(startfocus);
    else
      IupNextField(ih);
  }
}

int iupDialogGetChildId(Ihandle* ih)
{
  int id;
  ih = IupGetDialog(ih);
  if (!ih) return -1;
  id = ih->data->child_id;
  if (id == 0) id = 100; /* initial number */
  ih->data->child_id = id+1;
  return id;
}

char* iupDialogGetChildIdStr(Ihandle* ih)
{
  char *str = iupStrGetMemory(50);
  Ihandle* dialog = IupGetDialog(ih);
  sprintf(str, "iup-%s-%d", ih->iclass->name, dialog->data->child_id);
  return str;
}

int iupDialogPopup(Ihandle* ih, int x, int y)
{
  int ret = iupClassObjectDlgPopup(ih, x, y);
  if (ret != IUP_INVALID) /* IUP_INVALID means it is not implemented */
    return ret;

  ih->data->show_state = IUP_SHOW;

  /* Update only the position */
  iupDialogAdjustPos(ih, &x, &y);
  iupdrvDialogSetPosition(ih, x, y);

  if (iupdrvIsVisible(ih)) /* already visible */
  {
    /* only re-show to raise the window */
    iupdrvDialogSetVisible(ih, 1);
    
    iDialogCallShowCb(ih);
    return IUP_NOERROR; 
  }

  if (iupAttribGetInt(ih, "MODAL")) /* already a popup */
    return IUP_NOERROR; 

  iDialogSetModal(ih);

  /* actually show the window */
  iupdrvDialogSetVisible(ih, 1);

  /* increment visible count */
  iupDlgListVisibleInc();
    
  iDialogCallShowCb(ih);

  /* interrupt processing here */
  IupMainLoop();

  /* if window is still valid (IupDestroy not called), 
     hide the dialog if still visible. */
  if (iupObjectCheck(ih))
  {
    iDialogUnSetModal(ih);

    IupHide(ih); 
  }

  return IUP_NOERROR;
}

int iupDialogShowXY(Ihandle* ih, int x, int y)
{
  int was_visible;

  if (iupAttribGetInt(ih, "MODAL")) /* already a popup */
    return IUP_NOERROR; 

  if (ih->data->popup_level != 0)
  {
    /* was disabled by a Popup, re-enable it */
    iupdrvSetActive(ih, 1);
    ih->data->popup_level = 0; /* Now it is at the current popup level */
  }

  /* save visible state before iupdrvDialogSetPlacement */
  /* because it can also show the window when changing placement. */
  was_visible = iupdrvIsVisible(ih); 

  /* Update the position and placement */
  if (!iupdrvDialogSetPlacement(ih, x, y))
  {
    iupDialogAdjustPos(ih, &x, &y);
    iupdrvDialogSetPosition(ih, x, y);
  }

  if (was_visible) /* already visible */
  {
    /* only re-show to raise the window */
    iupdrvDialogSetVisible(ih, 1);
    
    iDialogCallShowCb(ih);
    return IUP_NOERROR; 
  }
                          
  /* actually show the window */
  /* test if placement turn the dialog visible */
  if (!iupdrvIsVisible(ih))
    iupdrvDialogSetVisible(ih, 1);

  /* increment visible count */
  iupDlgListVisibleInc();
    
  iDialogCallShowCb(ih);

  return IUP_NOERROR;
}

void iupDialogHide(Ihandle* ih)
{
  /* hidden at the system and marked hidden in IUP */
  if (!iupdrvIsVisible(ih) && ih->data->show_state == IUP_HIDE) 
    return;

  /* marked hidden in IUP */
  ih->data->show_state = IUP_HIDE;

  /* if called IupHide for a Popup window */
  if (iupAttribGetInt(ih, "MODAL"))
  {
    iDialogUnSetModal(ih);
    IupExitLoop();
  }

  /* actually hide the window */
  iupdrvDialogSetVisible(ih, 0);

  /* decrement visible count */
  iupDlgListVisibleDec();

  if (iupDlgListVisibleCount() <= 0)
  {
    /* if this is the last window visible, 
       exit message loop except when LOCKLOOP==YES */
    if (!iupStrBoolean(IupGetGlobal("LOCKLOOP")))
      IupExitLoop();
  }

  iDialogCallShowCb(ih);
}


/****************************************************************/


static int iDialogSizeGetScale(const char* sz)
{
  if (!sz || sz[0] == 0) return 0;
  if (iupStrEqualNoCase(sz, "FULL"))     return 1;
  if (iupStrEqualNoCase(sz, "HALF"))     return 2;
  if (iupStrEqualNoCase(sz, "THIRD"))    return 3;
  if (iupStrEqualNoCase(sz, "QUARTER"))  return 4;
  if (iupStrEqualNoCase(sz, "EIGHTH"))   return 8;
  return 0;
}

static int iDialogSetSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    char *sh, sw[40];
    strcpy(sw, value);
    sh = strchr(sw, 'x');
    if (!sh)
      sh = "";
    else
    {
      *sh = '\0';  /* to mark the end of sw */
      ++sh;
    }

    {
      int charwidth, charheight;
      int screen_width, screen_height;
      int wscale = iDialogSizeGetScale(sw);
      int hscale = iDialogSizeGetScale(sh);

      int width = 0, height = 0; 
      iupStrToIntInt(value, &width, &height, 'x');
      if (width < 0) width = 0;
      if (height < 0) height = 0;

      iupdrvFontGetCharSize(ih, &charwidth, &charheight);

      /* desktop size, excluding task bars and menu bars */
      iupdrvGetScreenSize(&screen_width, &screen_height);

      if (wscale)
        width = screen_width/wscale;
      else
        width = iupWIDTH2RASTER(width, charwidth);

      if (hscale)
        height = screen_height/hscale;
      else
        height = iupHEIGHT2RASTER(height, charheight);

      ih->userwidth = width;
      ih->userheight = height;
    }
  }

  /* must reset the current size,  */
  /* so the user or the natural size will be used to resize the dialog */
  ih->currentwidth = 0;
  ih->currentheight = 0;

  return 0;
}

static int iDialogSetRasterSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int w = 0, h = 0;
    iupStrToIntInt(value, &w, &h, 'x');
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    ih->userwidth = w;
    ih->userheight = h;
  }

  /* must reset the current size also,  */
  /* so the user or the natural size will be used to resize the dialog */
  ih->currentwidth = 0;
  ih->currentheight = 0;

  return 0;
}

static int iDialogSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    IupShow(ih);
  else
    IupHide(ih);

  return 1;
}

void iupDialogUpdatePosition(Ihandle* ih)
{
  int x = iupAttribGetInt(ih, "_IUPDLG_X");
  int y = iupAttribGetInt(ih, "_IUPDLG_Y");
  iupdrvDialogUpdateSize(ih);
  iupDialogAdjustPos(ih, &x, &y);
  iupdrvDialogSetPosition(ih, x, y);
}

void iupDialogAdjustPos(Ihandle *ih, int *x, int *y)
{
  int cursor_x = 0, cursor_y = 0;
  int screen_width = 0, screen_height = 0;
  int current_x = 0, current_y = 0;
  int parent_x = 0, parent_y = 0;

  if (*x == IUP_CURRENT || *y == IUP_CURRENT)
    iupdrvDialogGetPosition(ih->handle, &current_x, &current_y);

  if (*x == IUP_CENTER || *y == IUP_CENTER ||
      *x == IUP_RIGHT  || *y == IUP_RIGHT ||
      *x == IUP_CENTERPARENT || *y == IUP_CENTERPARENT)
    iupdrvGetScreenSize(&screen_width, &screen_height);

  if (*x == IUP_CENTERPARENT || *y == IUP_CENTERPARENT)
  {
    InativeHandle* parent = iupDialogGetNativeParent(ih);
    if (parent)
    {
      iupdrvDialogGetPosition(parent, &parent_x, &parent_y);
      if (*x == IUP_CENTERPARENT && *y == IUP_CENTERPARENT)
        iupdrvDialogGetSize(parent, &screen_width, &screen_height);
      else if (*x == IUP_CENTERPARENT)
        iupdrvDialogGetSize(parent, &screen_width, NULL);
      else if (*y == IUP_CENTERPARENT)
        iupdrvDialogGetSize(parent, NULL, &screen_height);
    }
  }

  if (*x == IUP_MOUSEPOS || *y == IUP_MOUSEPOS)
    iupdrvGetCursorPos(&cursor_x, &cursor_y);

  if (IupGetInt(ih, "MDICHILD"))
  {
    Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
    if (client)
    {
      screen_width = client->currentwidth;
      screen_height = client->currentheight;

      iupdrvScreenToClient(client, &current_x, &current_y);
      iupdrvScreenToClient(client, &cursor_x, &cursor_y);
    }
  }

  switch (*x)
  {
  case IUP_CENTERPARENT:
    *x = (screen_width - ih->currentwidth)/2 + parent_x;
    break;
  case IUP_CENTER:
    *x = (screen_width - ih->currentwidth)/2;
    break;
  case IUP_LEFT:
    *x = 0;
    break;
  case IUP_RIGHT:
    *x = screen_width - ih->currentwidth;
    break;
  case IUP_MOUSEPOS:
    *x = cursor_x;
    break;
  case IUP_CURRENT:
    *x = current_x;
    break;
  }

  switch (*y)
  {
  case IUP_CENTERPARENT:
    *y = (screen_height - ih->currentheight)/2 + parent_y;
    break;
  case IUP_CENTER:
    *y = (screen_height - ih->currentheight)/2;
    break;
  case IUP_LEFT:
    *y = 0;
    break;
  case IUP_RIGHT:
    *y = screen_height - ih->currentheight;
    break;
  case IUP_MOUSEPOS:
    *y = cursor_y;
    break;
  case IUP_CURRENT:
    *y = current_y;
    break;
  }
}

void iupDialogGetDecorSize(Ihandle* ih, int *decorwidth, int *decorheight)
{
  int border, caption, menu;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  *decorwidth = 2*border;
  *decorheight = 2*border + caption + menu;
}

static void iDialogSetPositionMethod(Ihandle* ih, int x, int y)
{
  /* x and y are always 0 for the dialog. */
  ih->x = x;
  ih->y = y;

  if (ih->firstchild)
  {
    /* Child coordinates are relative to client left-top corner. */
    iupClassObjectSetPosition(ih->firstchild, 0, 0);
  }
}

static int iDialogSetHideTaskbarAttrib(Ihandle *ih, const char *value)
{
  iupdrvDialogSetVisible(ih, !iupStrBoolean(value));
  return 0;
}

static int iDialogSetMenuAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
  {
    Ihandle* menu = IupGetHandle(value);
    ih->data->menu = menu;
    return 1;
  }

  if (!value)
  {
    if (ih->data->menu && ih->data->menu->handle)
    {
      ih->data->ignore_resize = 1;
      IupUnmap(ih->data->menu);  /* this will remove the menu from the dialog */
      ih->data->ignore_resize = 0;

      ih->data->menu = NULL;
    }
  }
  else
  {
    Ihandle* menu = IupGetHandle(value);
    if (!menu || menu->iclass->nativetype != IUP_TYPEMENU || menu->parent)
      return 0;

    /* already the current menu and it is mapped */
    if (ih->data->menu && ih->data->menu==menu && menu->handle)
      return 1;

    /* the current menu is mapped, so unmap it */
    if (ih->data->menu && ih->data->menu->handle && ih->data->menu!=menu)
    {
      ih->data->ignore_resize = 1;
      IupUnmap(ih->data->menu);   /* this will remove the menu from the dialog */
      ih->data->ignore_resize = 0;
    }

    ih->data->menu = menu;

    menu->parent = ih;    /* use this to create a menu bar instead of a popup menu */

    ih->data->ignore_resize = 1;
    IupMap(menu);     /* this will automatically add the menu to the dialog */
    ih->data->ignore_resize = 0;
  }
  return 1;
}


/****************************************************************/


Ihandle* IupDialog(Ihandle* child)
{
  void *params[2];
  params[0] = child;
  params[1] = NULL;
  return IupCreatev("dialog", params);
}

Iclass* iupDialogGetClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "dialog";
  ic->format = "H"; /* one optional ihandle */
  ic->nativetype = IUP_TYPEDIALOG;
  ic->childtype = IUP_CHILD_ONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->Create = iDialogCreateMethod;
  ic->Destroy = iDialogDestroyMethod;
  ic->ComputeNaturalSize = iDialogComputeNaturalSizeMethod;
  ic->SetCurrentSize = iDialogSetCurrentSizeMethod;
  ic->SetPosition = iDialogSetPositionMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "SHOW_CB", "i");
  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");
  iupClassRegisterCallback(ic, "RESIZE_CB", "ii");

  /* Attribute functions */

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "SIZE", iupBaseGetSizeAttrib, iDialogSetSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RASTERSIZE", iupBaseGetRasterSizeAttrib, iDialogSetRasterSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSITION", iupBaseNoGetAttrib, iupBaseNoSetAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "VISIBLE", iupBaseGetVisibleAttrib, iDialogSetVisibleAttrib, "NO", IUP_MAPPED, IUP_NO_INHERIT); /* the only case where VISIBLE default is NO */

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "MENU", NULL, iDialogSetMenuAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURSOR", NULL, iupdrvBaseSetCursorAttrib, "ARROW", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETASKBAR", NULL, iDialogSetHideTaskbarAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, "YES", IUP_MAPPED, IUP_NO_INHERIT);

  iupdrvDialogInitClass(ic);

  return ic;
}
