/** \file
 * \brief IupMessageDlg class
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_stdcontrols.h"


Ihandle* IupMessageDlg(void)
{
  return IupCreate("messagedlg");
}

Iclass* iupMessageDlgGetClass(void)
{
  Iclass* ic = iupClassNew(iupDialogGetClass());

  ic->name = "messagedlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  /* reset not used native dialog methods */
  ic->parent->LayoutUpdate = NULL;
  ic->parent->SetPosition = NULL;
  ic->parent->Map = NULL;
  ic->parent->UnMap = NULL;

  iupdrvMessageDlgInitClass(ic);

  /* only the default values */
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, "MESSAGE", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONS", NULL, NULL, "OK", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONDEFAULT", NULL, NULL, "1", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONRESPONSE", NULL, NULL, "1", IUP_MAPPED, IUP_NO_INHERIT);

  return ic;
}

void IupMessage(const char* title, const char* message)
{
  Ihandle* dlg = IupCreate("messagedlg");

  IupSetAttribute(dlg, "TITLE", (char*)title);
  IupSetAttribute(dlg, "VALUE", (char*)message);
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));

  IupPopup(dlg, IUP_CENTER, IUP_CENTER);
  IupDestroy(dlg);
}

void IupMessagef(const char *title, const char *format, ...)
{
  static char message[SHRT_MAX];
  va_list arglist;
  va_start(arglist, format);
  vsprintf(message, format, arglist);
  va_end (arglist);
  IupMessage(title, message);
}
