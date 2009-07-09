/** \file
 * \brief GTK Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"


int iupgtk_utf8autoconvert = 1;

int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "LANGUAGE"))
  {
    iupStrMessageUpdateLanguage(value);
    return 1;
  }
  if (iupStrEqual(name, "CURSORPOS"))
  {
#if GTK_CHECK_VERSION(2, 8, 0)
    int x, y;
    if (iupStrToIntInt(value, &x, &y, 'x') == 2)
      gdk_display_warp_pointer(gdk_display_get_default(), gdk_screen_get_default(), x, y);
#endif
    return 0;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (!value || iupStrBoolean(value))
      iupgtk_utf8autoconvert = 1;
    else
      iupgtk_utf8autoconvert = 0;
    return 0;
  }
  return 1;
}

char *iupdrvGetGlobal(const char *name)
{
  if (iupStrEqual(name, "CURSORPOS"))
  {
    char *str = iupStrGetMemory(50);
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    sprintf(str, "%dx%d", (int)x, (int)y);
    return str;
  }
  if (iupStrEqual(name, "SHIFTKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[0] == 'S')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "CONTROLKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[1] == 'C')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "MODKEYSTATE"))
  {
    char *str = iupStrGetMemory(5);
    iupdrvGetKeyState(str);
    return str;
  }
  if (iupStrEqual(name, "SCREENSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "FULLSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetFullSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "SCREENDEPTH"))
  {
    char *str = iupStrGetMemory(50);
    int bpp = iupdrvGetScreenDepth();
    sprintf(str, "%d", bpp);
    return str;
  }
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    char *str = iupStrGetMemory(50);
    GdkScreen *screen = gdk_screen_get_default();
    GdkWindow *root = gdk_screen_get_root_window(gdk_screen_get_default());
    int x = 0;
    int y = 0;
    int w = gdk_screen_get_width(screen); 
    int h = gdk_screen_get_height(screen);
    gdk_window_get_root_origin(root, &x, &y);
    sprintf(str, "%d %d %d %d", x, y, w, h);
    return str;
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int i;
    GdkScreen *screen = gdk_screen_get_default();
    int monitors_count = gdk_screen_get_n_monitors(screen);
    char *str = iupStrGetMemory(monitors_count*50);
    char* pstr = str;
    GdkRectangle rect;

    for (i=0; i < monitors_count; i++)
    {
      gdk_screen_get_monitor_geometry(screen, i, &rect);
      pstr += sprintf(pstr, "%d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
    }

    return str;
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    if (gdk_visual_get_best_depth() > 8)
      return "YES";
    else
      return "NO";
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (iupgtk_utf8autoconvert)
      return "YES";
    else
      return "NO";
  }
  return NULL;
}
