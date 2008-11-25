/** \file
 * \brief Motif Open/Close
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>    
#include <stdio.h>    
#include <locale.h>
#include <string.h>    

#include <Xm/Xm.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


/* global variables */
Widget   iupmot_appshell = 0;
Display* iupmot_display = 0;
int      iupmot_screen = 0;
Visual*  iupmot_visual = 0;
XtAppContext iupmot_appcontext = 0;

void* iupdrvGetDisplay(void)
{
  return iupmot_display;
}

void iupmotSetGlobalColorAttrib(Widget w, const char* xmname, const char* name)
{
  unsigned char r, g, b; 
  Pixel color;

  XtVaGetValues(w, xmname, &color, NULL);
  iupmotColorGetRGB(color, &r, &g, &b);

  IupSetfAttribute(NULL, name, "%3d %3d %3d", (int)r, (int)g, (int)b);
}

int iupdrvOpen(int *argc, char ***argv)
{
  IupSetGlobal("DRIVER", "Motif");

  /* XtSetLanguageProc(NULL, NULL, NULL); 
     Removed to avoid invalid locale in modern Linux that set LANG=en_US.UTF-8 */

  iupmot_appshell = XtVaOpenApplication(&iupmot_appcontext, "Iup", NULL, 0, argc, *argv, NULL,
                                        sessionShellWidgetClass, NULL);
  if (!iupmot_appshell)
  {
    fprintf(stderr, "IUP error: cannot open X-Windows.\n");
    return IUP_ERROR;
  }
  IupSetGlobal("APPSHELL", (char*)iupmot_appshell);

  IupStoreGlobal("SYSTEMLANGUAGE", setlocale(LC_ALL, NULL));

  iupmot_display = XtDisplay(iupmot_appshell);
  iupmot_screen  = XDefaultScreen(iupmot_display);

  IupSetGlobal("XDISPLAY", (char*)iupmot_display);
  IupSetGlobal("XSCREEN", (char*)iupmot_screen);

  /* screen depth can be 8bpp, but canvas can be 24bpp */
  {
    XVisualInfo vinfo;
    if (XMatchVisualInfo(iupmot_display, iupmot_screen, 24, TrueColor, &vinfo) ||
        XMatchVisualInfo(iupmot_display, iupmot_screen, 16, TrueColor, &vinfo))
    {
      iupmot_visual = vinfo.visual;
      IupSetGlobal("TRUECOLORCANVAS", "YES");
    }
    else
    {
      iupmot_visual = DefaultVisual(iupmot_display, iupmot_screen);
      IupSetGlobal("TRUECOLORCANVAS", "NO");
    }
  }

  /* driver system version */
  {
    int major = xmUseVersion/1000;
    int minor = xmUseVersion - major * 1000;
    IupSetfAttribute(NULL, "MOTIFVERSION", "%d.%d", major, minor);
    IupSetfAttribute(NULL, "MOTIFNUMBER", "%d", (XmVERSION * 1000 + XmREVISION * 100 + XmUPDATE_LEVEL));

    IupSetGlobal("XSERVERVENDOR", ServerVendor(iupmot_display));
    IupSetfAttribute(NULL, "XVENDORRELEASE", "%d", VendorRelease(iupmot_display));
  }

  iupmotColorInit();

  /* dialog background color */
  {
    iupmotSetGlobalColorAttrib(iupmot_appshell, XmNbackground, "DLGBGCOLOR");
    IupSetfAttribute(NULL, "DLGFGCOLOR", "%3d %3d %3d", 0, 0, 0);
    IupSetfAttribute(NULL, "TXTBGCOLOR", "%3d %3d %3d", 255, 255, 255);
    IupSetfAttribute(NULL, "TXTFGCOLOR", "%3d %3d %3d", 0, 0, 0);

    IupSetGlobal("_IUP_SET_DLGFGCOLOR", "YES");
    IupSetGlobal("_IUP_SET_TXTCOLORS", "YES");
  }

  if (getenv("IUP_DEBUG"))
    XSynchronize(iupmot_display, 1);

  return IUP_NOERROR;
}

void iupdrvClose(void)
{ 
  iupmotColorFinish();
  iupmotTipsFinish();

  if (iupmot_appshell)
    XtDestroyWidget(iupmot_appshell);

  if (iupmot_appcontext)
    XtDestroyApplicationContext(iupmot_appcontext);
}
