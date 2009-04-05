/** \file
 * \brief Windows Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include <windows.h>
#include <commctrl.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_draw.h"


HINSTANCE iupwin_hinstance = 0;    
int       iupwin_comctl32ver6 = 0;

void* iupdrvGetDisplay(void)
{
  return NULL;
}

void iupwinShowLastError(void)
{
  int error = GetLastError();
  if (error)
  {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, "GetLastError:", MB_OK|MB_ICONERROR);
    LocalFree(lpMsgBuf);
  }
}

int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc; /* unused in the Windows driver */
  (void)argv;

  if (iupwinGetSystemMajorVersion() < 5) /* older than Windows 2000 */
    return IUP_ERROR;

  IupSetGlobal("DRIVER",  "Win32");

  {
    /* TODO: Check this code */
    HWND win = GetConsoleWindow();
    if (win)
      iupwin_hinstance = (HINSTANCE)GetWindowLongPtr(win, GWLP_HINSTANCE);
    else
      iupwin_hinstance = GetModuleHandle(NULL);
    IupSetGlobal("HINSTANCE", (char*)iupwin_hinstance);
  }

  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  {
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;  /* trackbar, tooltips, updown, tab, progress */
    InitCommonControlsEx(&InitCtrls);  
  }

  iupwin_comctl32ver6 = (iupwinGetComCtl32Version() >= 0x060000)? 1: 0;
  if (iupwin_comctl32ver6 && !iupwinIsAppThemed())  /* When the user seleted the Windows Classic theme */
    iupwin_comctl32ver6 = 0;

  IupSetGlobal("SYSTEMLANGUAGE", iupwinGetSystemLanguage());

  /* dialog background color */
  {
    COLORREF color;

    color = GetSysColor(COLOR_BTNFACE);
    IupSetfAttribute(NULL, "DLGBGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                        (int)GetGValue(color), 
                                                        (int)GetBValue(color));
    color = GetSysColor(COLOR_BTNTEXT);
    IupSetfAttribute(NULL, "DLGFGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                        (int)GetGValue(color), 
                                                        (int)GetBValue(color));
    color = GetSysColor(COLOR_WINDOW);
    IupSetfAttribute(NULL, "TXTBGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                        (int)GetGValue(color), 
                                                        (int)GetBValue(color));
    color = GetSysColor(COLOR_WINDOWTEXT);
    IupSetfAttribute(NULL, "TXTFGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                        (int)GetGValue(color), 
                                                        (int)GetBValue(color));
    color = GetSysColor(COLOR_MENU);
    IupSetfAttribute(NULL, "MENUBGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                         (int)GetGValue(color), 
                                                         (int)GetBValue(color));
    color = GetSysColor(COLOR_MENUTEXT);
    IupSetfAttribute(NULL, "MENUFGCOLOR", "%3d %3d %3d", (int)GetRValue(color), 
                                                         (int)GetGValue(color), 
                                                         (int)GetBValue(color));
  }

  iupwinHandleInit();
  iupwinBrushInit();
  iupwinDrawInit();

#ifdef __WATCOMC__ 
  {
    /* this is used to force Watcom to link the winmain.c module. */
    void iupwinMainDummy(void);
    iupwinMainDummy();
  }
#endif

  return IUP_NOERROR;
}

void iupdrvClose(void)
{
  iupwinHandleFinish();
  iupwinBrushFinish();

  CoUninitialize();
}
