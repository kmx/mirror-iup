/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in iup.h
 */

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_info.h"


#define IWIN_TRAY_NOTIFICATION 102

static int WM_HELPMSG;

static int winDialogSetBgColorAttrib(Ihandle* ih, const char* value);
static int winDialogSetTrayAttrib(Ihandle *ih, const char *value);

/****************************************************************
                     Utilities
****************************************************************/

void iupdrvDialogUpdateSize(Ihandle* ih)
{
  RECT rect;
  GetWindowRect(ih->handle, &rect);
  ih->currentwidth = rect.right-rect.left;
  ih->currentheight = rect.bottom-rect.top;
}

void iupdrvDialogGetSize(InativeHandle* handle, int *w, int *h)
{
  RECT rect;
  GetWindowRect(handle, &rect);
  if (w) *w = rect.right-rect.left;
  if (h) *h = rect.bottom-rect.top;
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  ShowWindow(ih->handle, visible? ih->data->cmd_show: SW_HIDE);
}

void iupdrvDialogGetPosition(InativeHandle* handle, int *x, int *y)
{
  RECT rect;
  GetWindowRect(handle, &rect);
  if (x) *x = rect.left;
  if (y) *y = rect.top;
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  /* Only moves the window and places it at the top of the Z order. */
  SetWindowPos(ih->handle, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  if (ih->data->menu)
    *menu = iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    *menu = 0;

  if (ih->handle)
  {
    iupdrvGetWindowDecor(ih->handle, border, caption);

    if (*menu)
      *caption -= *menu;
  }
  else
  {
    int has_titlebar = iupAttribGetIntDefault(ih, "MAXBOX")  ||
                      iupAttribGetIntDefault(ih, "MINBOX")  ||
                      iupAttribGetIntDefault(ih, "MENUBOX") || 
                      IupGetAttribute(ih, "TITLE");  /* must use IupGetAttribute to check from the native implementation */

    *caption = 0;
    if (has_titlebar)
    {
      if (iupAttribGetInt(ih, "TOOLBOX") && iupAttribGetStr(ih, "PARENTDIALOG"))
        *caption = GetSystemMetrics(SM_CYSMCAPTION); /* tool window */
      else
        *caption = GetSystemMetrics(SM_CYCAPTION);   /* normal window */
    }

    *border = 0;
    if (iupAttribGetIntDefault(ih, "RESIZE"))
    {
      *border = GetSystemMetrics(SM_CXFRAME);     /* Thickness of the sizing border around the perimeter of a window  */
    }                                             /* that can be resized, in pixels.                                  */
    else if (has_titlebar)
    {
      *border = GetSystemMetrics(SM_CXFIXEDFRAME);  /* Thickness of the frame around the perimeter of a window        */
    }                                               /* that has a caption but is not sizable, in pixels.              */
    else if (iupAttribGetIntDefault(ih, "BORDER"))
    {
      *border = GetSystemMetrics(SM_CXBORDER);
    }
  }
}

int iupdrvDialogSetPlacement(Ihandle* ih, int x, int y)
{
  char* placement;

  ih->data->cmd_show = SW_SHOWNORMAL;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetInt(ih, "FULLSCREEN"))
    return 1;
  
  placement = iupAttribGetStr(ih, "PLACEMENT");
  if (!placement)
  {
    if (IsIconic(ih->handle) || IsZoomed(ih->handle))
      ih->data->show_state = IUP_RESTORE;
    return 0;
  }

  if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->cmd_show = SW_SHOWMAXIMIZED;
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->cmd_show = SW_SHOWMINIMIZED;
    ih->data->show_state = IUP_MINIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height;
    int caption, border, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* the dialog will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    /* position the decoration and menu outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    width += 2*border;
    height += 2*border + caption + menu;

    /* set the new size and position */
    /* WM_SIZE will update the layout */
    SetWindowPos(ih->handle, HWND_TOP, x, y, width, height, 0);

    if (IsIconic(ih->handle) || IsZoomed(ih->handle))
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSetStr(ih, "PLACEMENT", NULL); /* reset to NORMAL */

  return 1;
}


/****************************************************************************
                            WindowProc
****************************************************************************/


static Ihandle* winMinMaxHandle = NULL;

static int winDialogCheckMinMaxInfo(Ihandle* ih, MINMAXINFO* minmax)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */

  iupStrToIntInt(iupAttribGetStr(ih, "MINSIZE"), &min_w, &min_h, 'x');
  iupStrToIntInt(iupAttribGetStr(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  minmax->ptMinTrackSize.x = min_w;
  minmax->ptMinTrackSize.y = min_h;
  minmax->ptMaxTrackSize.x = max_w;
  minmax->ptMaxTrackSize.y = max_h;

  if (winMinMaxHandle == ih)
    winMinMaxHandle = NULL;

  return 1;
}

static void winDialogResize(Ihandle* ih, int width, int height)
{
  IFnii cb;

  iupdrvDialogUpdateSize(ih);

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb) cb(ih, width, height);  /* width and height here are for the client area */

  ih->data->ignore_resize = 1;

  IupRefresh(ih);

  if (iupwinIsVista())
    RedrawWindow(ih->handle,NULL,NULL,RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_INTERNALPAINT|RDW_ALLCHILDREN);

  ih->data->ignore_resize = 0;
}

static int winDialogMDICloseChildren(Ihandle* client)
{
  HWND hwndT;

  /* As long as the MDI client has a child, close it */
  while ((hwndT = GetWindow(client->handle, GW_CHILD)) != NULL)
  {
    Ihandle* child;

    /* Skip the icon title windows */
    while (hwndT && GetWindow (hwndT, GW_OWNER))
      hwndT = GetWindow(hwndT, GW_HWNDNEXT);

    if (!hwndT)
        break;

    child = iupwinHandleGet(hwndT); 
    if (child)
    {
      Icallback cb = IupGetCallback(child, "CLOSE_CB");
      if (cb)
      {
        int ret = cb(child);
        if (ret == IUP_IGNORE)
          return 0;
        if (ret == IUP_CLOSE)
          IupExitLoop();
        IupDestroy(child);
      }
    }
  }

  return 1;
}

static int winDialogBaseProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  if (iupwinBaseContainerProc(ih, msg, wp, lp, result))
    return 1;

  iupwinMenuDialogProc(ih, msg, wp, lp);

  switch (msg)
  {
  case WM_GETDLGCODE:
    *result = DLGC_WANTALLKEYS;
    return 1;
  case WM_GETMINMAXINFO:
    {
      if (winDialogCheckMinMaxInfo(ih, (MINMAXINFO*)lp))
      {
        *result = 0;
        return 1;
      }
      break;
    }
  case WM_SIZE:
    {
      if (ih->data->ignore_resize)
        break;

      switch(wp)
      {
      case SIZE_MINIMIZED:
        {
          if (ih->data->show_state != IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            if (show_cb && show_cb(ih, IUP_MINIMIZE) == IUP_CLOSE)
              IupExitLoop();
            ih->data->show_state = IUP_MINIMIZE;
          }
          break;
        }
      case SIZE_MAXIMIZED:
        {
          if (ih->data->show_state != IUP_MAXIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            if (show_cb && show_cb(ih, IUP_MAXIMIZE) == IUP_CLOSE)
              IupExitLoop();
            ih->data->show_state = IUP_MAXIMIZE;
          }

          winDialogResize(ih, LOWORD(lp), HIWORD(lp));
          break;
        }
      case SIZE_RESTORED:
        {
          if (ih->data->show_state == IUP_MAXIMIZE || ih->data->show_state == IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            if (show_cb && show_cb(ih, IUP_RESTORE) == IUP_CLOSE)
              IupExitLoop();
            ih->data->show_state = IUP_RESTORE;
          }

          winDialogResize(ih, LOWORD(lp), HIWORD(lp));
          break;
        }
      }

      break;
    }
  case WM_USER+IWIN_TRAY_NOTIFICATION:
    {
      int dclick  = 0;
      int button  = 0;
      int pressed = 0;

      switch (lp)
      {
      case WM_LBUTTONDOWN:
        pressed = 1;
        button  = 1;
        break;
      case WM_MBUTTONDOWN:
        pressed = 1;
        button  = 2;
        break;
      case WM_RBUTTONDOWN:
        pressed = 1;
        button  = 3;
        break;
      case WM_LBUTTONDBLCLK:
        dclick = 1;
        button = 1;
        break;
      case WM_MBUTTONDBLCLK:
        dclick = 1;
        button = 2;
        break;
      case WM_RBUTTONDBLCLK:
        dclick = 1;
        button = 3;
        break;
      case WM_LBUTTONUP:
        button = 1;
        break;
      case WM_MBUTTONUP:
        button = 2;
        break;
      case WM_RBUTTONUP:
        button = 3;
        break;
      }

      if (button != 0)
      {
        IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
        if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
          IupExitLoop();
      }

      break;
    }
  case WM_CLOSE:
    {
      Icallback cb = IupGetCallback(ih, "CLOSE_CB");
      if (cb)
      {
        int ret = cb(ih);
        if (ret == IUP_IGNORE)
        {
          *result = 0;
          return 1;
        }
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }

      /* child mdi is automatically destroyed */
      if (iupAttribGetInt(ih, "MDICHILD"))
        IupDestroy(ih);
      else
      {
        Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
        if (client)
        {
          if (!winDialogMDICloseChildren(client))
          {
            *result = 0;
            return 1;
          }
        }

        IupHide(ih); /* IUP default processing */
      }

      *result = 0;
      return 1;
    }
  case WM_SETCURSOR: 
    { 
      if (ih->handle == (HWND)wp && LOWORD(lp)==HTCLIENT)
      {
        HCURSOR hCur = (HCURSOR)iupAttribGetStr(ih, "_IUPWIN_HCURSOR");
        if (hCur)
        {
          SetCursor(hCur); 
          *result = 1;
          return 1;
        }
        else if (iupAttribGetStr(ih, "CURSOR"))
        {
          SetCursor(NULL); 
          *result = 1;
          return 1;
        }
      }
      break; 
    } 
  case WM_ERASEBKGND:
    {
      HBITMAP hBitmap = (HBITMAP)iupAttribGetStr(ih, "_IUPWIN_BACKGROUND_BITMAP");
      if (hBitmap) 
      {
        RECT rect;
        HDC hdc = (HDC)wp;

        HBRUSH hBrush = CreatePatternBrush(hBitmap);
        GetClientRect(ih->handle, &rect); 
        FillRect(hdc, &rect, hBrush); 
        DeleteObject(hBrush);

        /* return non zero value */
        *result = 1;
        return 1; 
      }
      else
      {
        unsigned char r, g, b;
        char* color = iupAttribGetStr(ih, "_IUPWIN_BACKGROUND_COLOR");
        if (iupStrToRGB(color, &r, &g, &b))
        {
          RECT rect;
          HDC hdc = (HDC)wp;

          SetDCBrushColor(hdc, RGB(r,g,b));
          GetClientRect(ih->handle, &rect); 
          FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH)); 

          /* return non zero value */
          *result = 1;
          return 1;
        }
      }
      break;
    }
  case WM_DESTROY:
    {
      /* Since WM_CLOSE changed the Windows default processing                            */
      /* WM_DESTROY is NOT received by IupDialogs                                         */
      /* Except when they are children of other IupDialogs and the parent is destroyed.   */
      /* So we have to destroy the child dialog.                                          */
      /* The application is responsable for destroying the children before this happen.   */
      IupDestroy(ih);
      break;
    }
  }

  if (msg == (UINT)WM_HELPMSG)
  {
    Ihandle* child = NULL;
    DWORD* struct_ptr = (DWORD*)lp;
    if (*struct_ptr == sizeof(CHOOSECOLOR))
    {
      CHOOSECOLOR* choosecolor = (CHOOSECOLOR*)lp;
      child = (Ihandle*)choosecolor->lCustData;
    }
    if (*struct_ptr == sizeof(CHOOSEFONT))
    {
      CHOOSEFONT* choosefont = (CHOOSEFONT*)lp;
      child = (Ihandle*)choosefont->lCustData;
    }

    if (child)
    {
      Icallback cb = IupGetCallback(child, "HELP_CB");
      if (cb && cb(child) == IUP_CLOSE)
        EndDialog((HWND)iupAttribGetStr(child, "HWND"), IDCANCEL);
    }
  }

  return 0;
}

static LRESULT CALLBACK winDialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!ih)
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  return DefWindowProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK winDialogMDIChildProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!ih)
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefMDIChildProc(hwnd, msg, wp, lp);
  }

  switch (msg)
  {
  case WM_MDIACTIVATE:
    {
      HWND hNewActive = (HWND)lp;
      if (hNewActive == ih->handle)
      {
        Icallback cb = (Icallback)IupGetCallback(ih, "MDIACTIVATE_CB");
        if (cb) cb(ih);
      }
      break;
    }
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  return DefMDIChildProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK winDialogMDIFrameProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  HWND hWndClient = NULL;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!ih)
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefFrameProc(hwnd, hWndClient, msg, wp, lp);
  }

  {
    Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
    if (client) hWndClient = client->handle;
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  return DefFrameProc(hwnd, hWndClient, msg, wp, lp);
}

static void winDialogRegisterClass(int mdi)
{
  char* name;
  WNDCLASS wndclass;
  WNDPROC winproc;
  ZeroMemory(&wndclass, sizeof(WNDCLASS));
  
  if (mdi == 2)
  {
    name = "IupDialogMDIChild";
    winproc = (WNDPROC)winDialogMDIChildProc;
  }
  else if (mdi == 1)
  {
    name = "IupDialogMDIFrame";
    winproc = (WNDPROC)winDialogMDIFrameProc;
  }
  else
  {
    if (mdi == -1)
      name = "IupDialogControl";
    else
      name = "IupDialog";
    winproc = (WNDPROC)winDialogProc;
  }

  wndclass.hInstance      = iupwin_hinstance;
  wndclass.lpszClassName  = name;
  wndclass.lpfnWndProc    = (WNDPROC)winproc;
  wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);

  /* To use a standard system color, must increase the background-color value by one */
  if (mdi == 1)
    wndclass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE+1);  
  else
    wndclass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);

  if (mdi == 0)
    wndclass.style |= CS_SAVEBITS;

  if (mdi == -1)
    wndclass.style |=  CS_HREDRAW | CS_VREDRAW;
    
  RegisterClass(&wndclass);
}


/****************************************************************
                     dialog class functions
****************************************************************/


static int winDialogMapMethod(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int childid = 0;
  DWORD dwStyle = WS_CLIPSIBLINGS, 
        dwStyleEx = 0;
  int has_titlebar = 0,
      has_border = 0;
  char* classname = "IupDialog";

  char* title = iupAttribGetStr(ih, "TITLE"); 
  if (title)
    has_titlebar = 1;

  if (iupAttribGetInt(ih, "DIALOGFRAME")) 
  {
    iupAttribSetStr(ih, "RESIZE", "NO");
    iupAttribSetStr(ih, "MINBOX", "NO");
  }

  if (iupAttribGetIntDefault(ih, "RESIZE"))
    dwStyle |= WS_THICKFRAME;
  else
    iupAttribSetStr(ih, "MAXBOX", "NO");  /* Must also remove this to RESIZE=NO work */

  if (iupAttribGetIntDefault(ih, "MAXBOX"))
  {
    dwStyle |= WS_MAXIMIZEBOX;
    has_titlebar = 1;
  }

  if (iupAttribGetIntDefault(ih, "MINBOX"))
  {
    dwStyle |= WS_MINIMIZEBOX;
    has_titlebar = 1;
  }

  if (iupAttribGetIntDefault(ih, "MENUBOX"))
  {
    dwStyle |= WS_SYSMENU;
    has_titlebar = 1;
  }

  if (iupAttribGetIntDefault(ih, "BORDER") || has_titlebar)
    has_border = 1;

  if (iupAttribGetInt(ih, "MDICHILD"))
  {
    static int mdi_child_id = IUP_MDICHILD_START;
    char name[50];
    Ihandle *client = IupGetAttributeHandle(ih, "MDICLIENT");

    /* store the MDICLIENT handle in each MDICHILD also */
    iupAttribSetStr(ih, "MDICLIENT_HANDLE", (char*)client);

    classname = "IupDialogMDIChild";

    parent = client->handle;

    dwStyle |= WS_CHILD;
    if (has_titlebar)
      dwStyle |= WS_CAPTION;
    else if (has_border)
      dwStyle |= WS_BORDER;

    dwStyleEx |= WS_EX_MDICHILD;

    sprintf(name, "mdichild%d", mdi_child_id - (IUP_MDICHILD_START));
    IupSetHandle(name, ih);

    childid = mdi_child_id;
    mdi_child_id++;
  }
  else
  {
    if (parent)
    {
      dwStyle |= WS_POPUP;

      if (has_titlebar)
        dwStyle |= WS_CAPTION;
      else if (has_border)
        dwStyle |= WS_BORDER;
    }
    else
    {
      if (has_titlebar)
      {
        dwStyle |= WS_OVERLAPPED;
      }
      else 
      {
        if (has_border)
          dwStyle |= WS_POPUP | WS_BORDER;
        else
          dwStyle |= WS_POPUP;

        dwStyleEx |= WS_EX_NOACTIVATE; /* this will hide it from the taskbar */ 
      }
    }

    if (iupAttribGetStr(ih, "MDIMENU"))
      classname = "IupDialogMDIFrame";
  }

  if (iupAttribGetInt(ih, "TOOLBOX") && parent)
    dwStyleEx |= WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;

  if (iupAttribGetInt(ih, "DIALOGFRAME") && parent)
    dwStyleEx |= WS_EX_DLGMODALFRAME;  /* this will hide the MENUBOX but not the close button */

  if (iupAttribGetIntDefault(ih, "COMPOSITED"))
    dwStyleEx |= WS_EX_COMPOSITED;
  else
    dwStyle |= WS_CLIPCHILDREN;

  if (iupAttribGetInt(ih, "HELPBUTTON"))
    dwStyleEx |= WS_EX_CONTEXTHELP;

  if (iupAttribGetInt(ih, "CONTROL") && parent) 
  {
    /* TODO: this were used by LuaCom to create embeded controls, 
       don't know if it is still working */
    dwStyleEx |= WS_EX_CONTROLPARENT;
    dwStyle = WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN;
    classname = "IupDialogControl";
  }

  /* CreateWindowEx will send WM_GETMINMAXINFO before Ihandle is associated with HWND */
  if (iupAttribGetStr(ih, "MINSIZE") || iupAttribGetStr(ih, "MAXSIZE"))
    winMinMaxHandle = ih;

  /* size will be updated in IupRefresh -> winDialogLayoutUpdate */
  /* position will be updated in iupDialogShowXY              */

  ih->handle = CreateWindowEx(dwStyleEx,          /* extended styles */
                              classname,          /* class */
                              title,              /* title */
                              dwStyle,            /* style */
                              0,                  /* x-position */
                              0,                  /* y-position */
                              100,                /* horizontal size - set this to avoid size calculation problems */
                              100,                /* vertical size */
                              parent,             /* owner window */
                              (HMENU)childid,     /* Menu or child-window identifier */
                              iupwin_hinstance,   /* instance of app. */
                              NULL);              /* no creation parameters */
  if (!ih->handle)
    return IUP_ERROR;

  /* associate HWND with Ihandle*, all Win32 controls must call this. */
  iupwinHandleSet(ih);

  if (iupStrEqual(classname, "IupDialogMDIChild")) /* hides the mdi child */
    ShowWindow(ih->handle, SW_HIDE);

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSetStr(ih, "DRAGDROP", "YES");

  /* Reset attributes handled during creation that */
  /* also can be changed later, and can be consulted from the native system. */
  iupAttribSetStr(ih, "TITLE", NULL); 
  iupAttribSetStr(ih, "BORDER", NULL);

  /* Ignore VISIBLE before mapping */
  iupAttribSetStr(ih, "VISIBLE", NULL);

  /* Set the default CmdShow for ShowWindow */
  ih->data->cmd_show = SW_SHOWNORMAL;

  return IUP_NOERROR;
}

static void winDialogUnMapMethod(Ihandle* ih)
{
  if (ih->data->menu) 
  {
    ih->data->menu->handle = NULL; /* the dialog will destroy the native menu */
    IupDestroy(ih->data->menu);  
  }

  if (iupAttribGetStr(ih, "_IUPDLG_HASTRAY"))
    winDialogSetTrayAttrib(ih, NULL);

  /* remove the association before destroying */
  iupwinHandleRemove(ih);

  /* Destroys the window, so we can destroy the class */
  if (iupAttribGetInt(ih, "MDICHILD")) 
  {
    /* for MDICHILDs must send WM_MDIDESTROY, instead of calling DestroyWindow */
    Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
    SendMessage(client->handle, WM_MDIDESTROY, (WPARAM)ih->handle, 0);
  }
  else
    DestroyWindow(ih->handle); /* this will destroy the Windows children also. */
                               /* but IupDestroy already destroyed the IUP children */
                               /* so it is safe to call DestroyWindow */
}

static void winDialogLayoutUpdateMethod(Ihandle *ih)
{
  if (ih->data->ignore_resize)
    return;

  ih->data->ignore_resize = 1;

  /* for dialogs the position is not updated here */
  SetWindowPos(ih->handle, 0, 0, 0, ih->currentwidth, ih->currentheight,
               SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING);

  ih->data->ignore_resize = 0;
}

static void winDialogReleaseMethod(Iclass* ic)
{
  (void)ic;
  if (iupwinClassExist("IupDialog"))
  {
    UnregisterClass("IupDialog", iupwin_hinstance);
    UnregisterClass("IupDialogControl", iupwin_hinstance);
    UnregisterClass("IupDialogMDIChild", iupwin_hinstance);
    UnregisterClass("IupDialogMDIFrame", iupwin_hinstance);
  }
}



/****************************************************************************
                                   Attributes
****************************************************************************/


static int winDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupAttribStoreStr(ih, "_IUPWIN_BACKGROUND_COLOR", value);
    iupAttribSetStr(ih, "_IUPWIN_BACKGROUND_BITMAP", NULL);
    RedrawWindow(ih->handle, NULL, NULL, RDW_ERASE|RDW_ERASENOW); /* force a WM_ERASEBKGND now */
    return 1;
  }
  return 0;
}

static int winDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (winDialogSetBgColorAttrib(ih, value))
    return 1;
  else
  {
    HBITMAP hBitmap = iupImageGetImage(value, ih, 0, "BACKGROUND");
    if (hBitmap)
    {
      iupAttribSetStr(ih, "_IUPWIN_BACKGROUND_COLOR", NULL);
      iupAttribSetStr(ih, "_IUPWIN_BACKGROUND_BITMAP", (char*)hBitmap);
      RedrawWindow(ih->handle, NULL, NULL, RDW_ERASE|RDW_ERASENOW); /* force a WM_ERASEBKGND now */
      return 1;
    }
  }

  return 0;
}

static HWND iupwin_mdifirst = NULL;
static HWND iupwin_mdinext = NULL;

static char* winDialogGetMdiActiveAttrib(Ihandle *ih)
{
  Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    HWND hchild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);
    Ihandle* child = iupwinHandleGet(hchild); 
    if (child)
    {
      iupwin_mdinext = NULL;
      iupwin_mdifirst = hchild;
      return IupGetName(child);
    }
  }

  iupwin_mdifirst = NULL;
  iupwin_mdinext = NULL;
  return NULL;
}

static char* winDialogGetMdiNextAttrib(Ihandle *ih)
{
  Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    Ihandle* child;
    HWND hchild = iupwin_mdinext? iupwin_mdinext: iupwin_mdifirst;

    /* Skip the icon title windows */
    while (hchild && GetWindow (hchild, GW_OWNER))
      hchild = GetWindow(hchild, GW_HWNDNEXT);

    if (!hchild || hchild == iupwin_mdifirst)
    {
      iupwin_mdinext = NULL;
      return NULL;
    }

    child = iupwinHandleGet(hchild); 
    if (child)
    {
      iupwin_mdinext = hchild;
      return IupGetName(child);
    }
  }

  iupwin_mdinext = NULL;
  return NULL;
}

/* define this here, so we do not need to define _WIN32_WINNT=0x0500 */
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#endif

#ifndef LWA_ALPHA
#define LWA_ALPHA               0x00000002
#endif

typedef BOOL (WINAPI*winSetLayeredWindowAttributes)(
  HWND hwnd,
  COLORREF crKey,
  BYTE bAlpha,
  DWORD dwFlags);

static int winDialogSetLayeredAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    SetWindowLong(ih->handle, GWL_EXSTYLE, GetWindowLong(ih->handle, GWL_EXSTYLE) | WS_EX_LAYERED);
  else
    SetWindowLong(ih->handle, GWL_EXSTYLE, GetWindowLong(ih->handle, GWL_EXSTYLE) & ~WS_EX_LAYERED);
  RedrawWindow(ih->handle, NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN); /* invalidate everything and all children */
  return 1;
}

static int winDialogSetLayerAlphaAttrib(Ihandle *ih, const char *value)
{
  if (value && iupAttribGetInt(ih, "LAYERED"))
  {
    static winSetLayeredWindowAttributes mySetLayeredWindowAttributes = NULL;

    int alpha;
    if (!iupStrToInt(value, &alpha))
      return 0;

    if (!mySetLayeredWindowAttributes)
    {
      HMODULE hinstDll = LoadLibrary("user32.dll");
      if (hinstDll)
        mySetLayeredWindowAttributes = (winSetLayeredWindowAttributes)GetProcAddress(hinstDll, "SetLayeredWindowAttributes");
    }

    if (mySetLayeredWindowAttributes)
      mySetLayeredWindowAttributes(ih->handle, 0, (BYTE)alpha, LWA_ALPHA);
  }
  return 1;
}

static int winDialogSetMdiArrangeAttrib(Ihandle *ih, const char *value)
{
  Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    UINT msg = 0;
    WPARAM wp = 0;

    if (iupStrEqualNoCase(value, "TILEHORIZONTAL"))
    {
      msg = WM_MDITILE;
      wp = MDITILE_HORIZONTAL;
    }
    else if (iupStrEqualNoCase(value, "TILEVERTICAL"))
    {
      msg = WM_MDITILE;
      wp = MDITILE_VERTICAL;
    }
    else if (iupStrEqualNoCase(value, "CASCADE"))
      msg = WM_MDICASCADE;
    else if (iupStrEqualNoCase(value, "ICON")) 
      msg = WM_MDIICONARRANGE;

    if (msg)
      SendMessage(client->handle, msg, wp, 0);
  }
  return 0;
}

static int winDialogSetMdiActivateAttrib(Ihandle *ih, const char *value)
{
  Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    Ihandle* child = IupGetHandle(value);
    if (child)
      SendMessage(client->handle, WM_MDIACTIVATE, (WPARAM)child->handle, 0);
    else
    {
      HWND hchild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);
      if (iupStrEqualNoCase(value, "NEXT"))
        SendMessage(client->handle, WM_MDINEXT, (WPARAM)hchild, TRUE);
      else if (iupStrEqualNoCase(value, "PREVIOUS"))
        SendMessage(client->handle, WM_MDINEXT, (WPARAM)hchild, FALSE);
    }
  }
  return 0;
}

static int winDialogSetMdiCloseAllAttrib(Ihandle *ih, const char *value)
{
  Ihandle* client = (Ihandle*)iupAttribGetStr(ih, "MDICLIENT_HANDLE");
  (void)value;
  if (client)
    winDialogMDICloseChildren(client);
  return 0;
}

static void winDialogTrayMessage(HWND hWnd, DWORD dwMessage, HICON hIcon, PSTR pszTip)
{
  NOTIFYICONDATA tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATA));

  tnd.cbSize  = sizeof(NOTIFYICONDATA);
  tnd.hWnd    = hWnd;
  tnd.uID      = 1000;

  if (dwMessage == NIM_ADD)
  {
    tnd.uFlags = NIF_MESSAGE;
    tnd.uCallbackMessage = WM_USER+IWIN_TRAY_NOTIFICATION;
  }
  else if (dwMessage == NIM_MODIFY)
  {
    if (hIcon)  
    {
      tnd.uFlags |= NIF_ICON;
      tnd.hIcon = hIcon;
    }

    if (pszTip) 
    {
      tnd.uFlags |= NIF_TIP;
      lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
    }
  }

  Shell_NotifyIcon(dwMessage, &tnd);
}

static int winDialogCheckTray(Ihandle *ih)
{
  if (iupAttribGetStr(ih, "_IUPDLG_HASTRAY"))
    return 1;

  if (iupAttribGetInt(ih, "TRAY"))
  {
    winDialogTrayMessage(ih->handle, NIM_ADD, NULL, NULL);
    iupAttribSetStr(ih, "_IUPDLG_HASTRAY", "YES");
    return 1;
  }

  return 0;
}

static int winDialogSetTrayAttrib(Ihandle *ih, const char *value)
{
  int tray = iupStrBoolean(value);
  if (iupAttribGetStr(ih, "_IUPDLG_HASTRAY"))
  {
    if (!tray)
    {
      winDialogTrayMessage(ih->handle, NIM_DELETE, NULL, NULL);
      iupAttribSetStr(ih, "_IUPDLG_HASTRAY", NULL);
    }
  }
  else
  {
    if (tray)
    {
      winDialogTrayMessage(ih->handle, NIM_ADD, NULL, NULL);
      iupAttribSetStr(ih, "_IUPDLG_HASTRAY", "YES");
    }
  }
  return 1;
}

static int winDialogSetTrayTipAttrib(Ihandle *ih, const char *value)
{
  if (winDialogCheckTray(ih))
    winDialogTrayMessage(ih->handle, NIM_MODIFY, NULL, (PSTR)value);
  return 1;
}

static int winDialogSetTrayImageAttrib(Ihandle *ih, const char *value)
{
  if (winDialogCheckTray(ih))
  {
    HICON hIcon = (HICON)iupImageGetIcon(value);
    if (hIcon)
      winDialogTrayMessage(ih->handle, NIM_MODIFY, hIcon, NULL);
  }
  return 1;
}

static int winDialogSetBringFrontAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    SetForegroundWindow(ih->handle);
  return 0;
}

static int winDialogSetTopMostAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    SetWindowPos(ih->handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  else
    SetWindowPos(ih->handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  return 1;
}

static int winDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  if (!value)
    SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)NULL);
  else
  {
    HICON icon = iupImageGetIcon(value);
    if (icon)
      SendMessage(ih->handle, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM)icon);    
  }

  if (IsIconic(ih->handle))
    RedrawWindow(ih->handle, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW); /* redraw the icon */
  else
    RedrawWindow(ih->handle, NULL, NULL, RDW_FRAME|RDW_UPDATENOW); /* only the frame needs to be redrawn */

  return 1;
}

static int winDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGetStr(ih, "_IUPWIN_FS_STYLE"))
    {
      int width, height;
      LONG off_style, new_style;

      BOOL visible = ShowWindow (ih->handle, SW_HIDE);

      /* remove the decorations */
      off_style = WS_BORDER | WS_THICKFRAME | WS_CAPTION | 
                  WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;
      new_style = GetWindowLong(ih->handle, GWL_STYLE);
      iupAttribSetStr(ih, "_IUPWIN_FS_STYLE", (char*)new_style);
      new_style &= (~off_style);
      SetWindowLong(ih->handle, GWL_STYLE, new_style);

      /* save the previous decoration attributes */
      iupAttribStoreStr(ih, "_IUPWIN_FS_MAXBOX", iupAttribGetStr(ih, "MAXBOX"));
      iupAttribStoreStr(ih, "_IUPWIN_FS_MINBOX", iupAttribGetStr(ih, "MINBOX"));
      iupAttribStoreStr(ih, "_IUPWIN_FS_MENUBOX",iupAttribGetStr(ih, "MENUBOX"));
      iupAttribStoreStr(ih, "_IUPWIN_FS_TITLE",  IupGetAttribute(ih, "TITLE"));  /* must use IupGetAttribute to check from the native implementation */
      iupAttribStoreStr(ih, "_IUPWIN_FS_RESIZE", iupAttribGetStr(ih, "RESIZE"));
      iupAttribStoreStr(ih, "_IUPWIN_FS_BORDER", iupAttribGetStr(ih, "BORDER"));

      /* save the previous position and size */
      iupAttribStoreStr(ih, "_IUPWIN_FS_X", IupGetAttribute(ih, "X"));  /* must use IupGetAttribute to check from the native implementation */
      iupAttribStoreStr(ih, "_IUPWIN_FS_Y", IupGetAttribute(ih, "Y"));
      iupAttribStoreStr(ih, "_IUPWIN_FS_SIZE", IupGetAttribute(ih, "RASTERSIZE"));

      /* remove the decorations attributes */
      iupAttribSetStr(ih, "MAXBOX", "NO");
      iupAttribSetStr(ih, "MINBOX", "NO");
      iupAttribSetStr(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);
      iupAttribSetStr(ih, "RESIZE", "NO");
      iupAttribSetStr(ih, "BORDER", "NO");

      /* full screen size */
      iupdrvGetFullSize(&width, &height);

      SetWindowPos(ih->handle, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);

      /* layout will be updated in WM_SIZE */
      if (visible)
        ShowWindow(ih->handle, SW_SHOW);
    }
  }
  else
  {
    LONG style = (LONG)iupAttribGetStr(ih, "_IUPWIN_FS_STYLE");
    if (style)
    {
      BOOL visible = ShowWindow(ih->handle, SW_HIDE);

      /* restore the decorations attributes */
      iupAttribStoreStr(ih, "MAXBOX", iupAttribGetStr(ih, "_IUPWIN_FS_MAXBOX"));
      iupAttribStoreStr(ih, "MINBOX", iupAttribGetStr(ih, "_IUPWIN_FS_MINBOX"));
      iupAttribStoreStr(ih, "MENUBOX",iupAttribGetStr(ih, "_IUPWIN_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE",  iupAttribGetStr(ih, "_IUPWIN_FS_TITLE"));  /* TITLE is not stored in the HashTable */
      iupAttribStoreStr(ih, "RESIZE", iupAttribGetStr(ih, "_IUPWIN_FS_RESIZE"));
      iupAttribStoreStr(ih, "BORDER", iupAttribGetStr(ih, "_IUPWIN_FS_BORDER"));

      /* restore the decorations */
      SetWindowLong(ih->handle, GWL_STYLE, style);

      /* restore position and size */
      SetWindowPos(ih->handle, HWND_TOP, 
                   iupAttribGetInt(ih, "_IUPWIN_FS_X"), 
                   iupAttribGetInt(ih, "_IUPWIN_FS_Y"), 
                   IupGetInt(ih, "_IUPWIN_FS_SIZE"), 
                   IupGetInt2(ih, "_IUPWIN_FS_SIZE"), 0);

      /* layout will be updated in WM_SIZE */
      if (visible)
        ShowWindow(ih->handle, SW_SHOW);

      /* remove auxiliar attributes */
      iupAttribSetStr(ih, "_IUPWIN_FS_MAXBOX", NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_MINBOX", NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_MENUBOX",NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_TITLE",  NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_RESIZE", NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_BORDER", NULL);

      iupAttribSetStr(ih, "_IUPWIN_FS_X", NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_Y", NULL);
      iupAttribSetStr(ih, "_IUPWIN_FS_SIZE", NULL);

      iupAttribSetStr(ih, "_IUPWIN_FS_STYLE", NULL);
    }
  }
  return 1;
}


void iupdrvDialogInitClass(Iclass* ic)
{
  if (!iupwinClassExist("IupDialog"))
  {
    winDialogRegisterClass(0);
    winDialogRegisterClass(1);
    winDialogRegisterClass(2);
    winDialogRegisterClass(-1);

    WM_HELPMSG = RegisterWindowMessage(HELPMSGSTRING);
  }

  /* Driver Dependent Class functions */
  ic->Map = winDialogMapMethod;
  ic->UnMap = winDialogUnMapMethod;
  ic->LayoutUpdate = winDialogLayoutUpdateMethod;
  ic->Release = winDialogReleaseMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winDialogSetBgColorAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", iupdrvBaseGetTitleAttrib, iupdrvBaseSetTitleAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupdrvBaseGetClientSizeAttrib, iupBaseNoSetAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, winDialogSetBackgroundAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, winDialogSetIconAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, winDialogSetFullScreenAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, iupBaseNoSetAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, NULL, "1x1", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, NULL, "65535x65535", IUP_NOT_MAPPED, IUP_NO_INHERIT);

  /* IupDialog Windows Only */
  iupClassRegisterAttribute(ic, "HWND", iupBaseGetWidAttrib, NULL, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIARRANGE", NULL, winDialogSetMdiArrangeAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIACTIVATE", NULL, winDialogSetMdiActivateAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLOSEALL", NULL, winDialogSetMdiCloseAllAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIACTIVE", winDialogGetMdiActiveAttrib, iupBaseNoSetAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDINEXT", winDialogGetMdiNextAttrib, iupBaseNoSetAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERED", NULL, winDialogSetLayeredAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERALPHA", NULL, winDialogSetLayerAlphaAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, winDialogSetBringFrontAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  if (iupwinIsVista())
    iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, "NO", IUP_NOT_MAPPED, IUP_INHERIT);
  else
    iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, "YES", IUP_NOT_MAPPED, IUP_INHERIT);

  /* IupDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, winDialogSetTopMostAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, iupwinSetDragDropAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAY", NULL, winDialogSetTrayAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, winDialogSetTrayImageAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, winDialogSetTrayTipAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
}
