/** \file
 * \brief IupColorDlg pre-defined dialog
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
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"


Ihandle* IupColorDlg(void)
{
  return IupCreate("colordlg");
}

Iclass* iupColorDlgGetClass(void)
{
  Iclass* ic = iupClassNew(iupDialogGetClass());

  ic->name = "colordlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  /* reset not used native dialog methods */
  ic->parent->LayoutUpdate = NULL;
  ic->parent->SetPosition = NULL;
  ic->parent->Map = NULL;
  ic->parent->UnMap = NULL;

  iupdrvColorDlgInitClass(ic);

  return ic;
}
