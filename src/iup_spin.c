/** \file
 * \brief Spin control
 *
  * See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_childtree.h"


static Ihandle* spin_timer = NULL;


static int iSpinCallCB(Ihandle* ih, int dub, int ten, int sign)
{
  IFni cb;

  /* get the callback on the spin or on the spinbox */
  Ihandle* spinbox = (Ihandle*)iupAttribGet(ih->parent, "_IUPSPIN_BOX");
  if (spinbox) 
    ih = spinbox;
  else
    ih = ih->parent;

  cb = (IFni) IupGetCallback(ih, "SPIN_CB");
  if (cb) 
  {
    return cb(ih, sign*(dub && ten ? 100 :
                               ten ?  10 : 
                               dub ?   2 : 1));
  }

  return IUP_DEFAULT;
}

static int iSpinTimerCB(Ihandle* ih)
{
  Ihandle* spin_button = (Ihandle*)iupAttribGet(ih, "_IUPSPIN_BUTTON");
  char* status   = iupAttribGet(ih, "_IUPSPIN_STATUS");
  int spin_dir   = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  int count      = iupAttribGetInt(ih, "_IUPSPIN_COUNT");
  char* reconfig = NULL;

  if(count == 0) /* first time */
    reconfig = "50";
  else if(count == 14) /* 300 + 14*50 = 1000 (1 second) */
    reconfig = "25";
  else if(count == 34) /* 300 + 14*50 + 20*50 = 2000 (2 seconds) */
    reconfig = "10";

  if (reconfig)
  {
    IupSetAttribute(ih, "RUN", "NO");
    IupSetAttribute(ih, "TIME", reconfig);
    IupSetAttribute(ih, "RUN", "YES");
  }

  iupAttribSetInt(ih, "_IUPSPIN_COUNT", count + 1);

  return iSpinCallCB(spin_button, iup_isshift(status), iup_iscontrol(status), spin_dir);
}

static void iSpinPrepareTimer(Ihandle* ih, char* status, char* dir)
{
  (void)ih;

  iupAttribSetStr(spin_timer, "_IUPSPIN_BUTTON", (char*)ih);

  iupAttribStoreStr(spin_timer, "_IUPSPIN_STATUS", status);

  iupAttribSetStr(spin_timer, "_IUPSPIN_DIR",   dir);
  iupAttribSetStr(spin_timer, "_IUPSPIN_COUNT", "0");

  IupSetAttribute(spin_timer, "TIME",           "400");
  IupSetAttribute(spin_timer, "RUN",            "YES");
}

static int iSpinK_SP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 0, 0, dir);
}

static int iSpinK_sSP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 1, 0, dir);
}

static int iSpinK_cSP(Ihandle* ih)
{
  int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
  
  return iSpinCallCB(ih, 0, 1, dir);
}

static int iSpinButtonCB(Ihandle* ih, int but, int pressed, int x, int y, char* status)
{
  (void)x;
  (void)y;
 
  if (pressed && but == IUP_BUTTON1)
  {
    int dir = iupAttribGetInt(ih, "_IUPSPIN_DIR");
    
    iSpinPrepareTimer(ih, status, iupAttribGet(ih, "_IUPSPIN_DIR"));
    
    return iSpinCallCB(ih, iup_isshift(status), iup_iscontrol(status), dir);
  }
  else if (!pressed && but == IUP_BUTTON1)
  {
    IupSetAttribute(spin_timer, "RUN", "NO");
  }
  
  return IUP_DEFAULT;
}

static int iSpinCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bt_up;
  Ihandle* bt_down;
  (void)params;

  /* Button UP */
  bt_up = IupButton(NULL, NULL);
  
  IupSetAttribute(bt_up, "EXPAND", "NO");
  IupSetAttribute(bt_up, "IMAGE",  "IupSpinUpImage");
  IupSetAttribute(bt_up, "_IUPSPIN_DIR", "1");
  IupSetAttribute(bt_up, "CANFOCUS", "NO");

  IupSetCallback(bt_up, "BUTTON_CB", (Icallback) iSpinButtonCB);
  IupSetCallback(bt_up, "K_SP",      (Icallback) iSpinK_SP);
  IupSetCallback(bt_up, "K_sSP",     (Icallback) iSpinK_sSP);
  IupSetCallback(bt_up, "K_cSP",     (Icallback) iSpinK_cSP);

  /* Button DOWN */
  bt_down = IupButton(NULL, NULL);
  
  IupSetAttribute(bt_down, "EXPAND", "NO");
  IupSetAttribute(bt_down, "IMAGE",  "IupSpinDownImage");
  IupSetAttribute(bt_down, "_IUPSPIN_DIR", "-1");
  IupSetAttribute(bt_down, "CANFOCUS", "NO");

  IupSetCallback(bt_down, "BUTTON_CB", (Icallback) iSpinButtonCB);
  IupSetCallback(bt_down, "K_SP",      (Icallback) iSpinK_SP);
  IupSetCallback(bt_down, "K_sSP",     (Icallback) iSpinK_sSP);
  IupSetCallback(bt_down, "K_cSP",     (Icallback) iSpinK_cSP);

  /* manually add the buttons as a children */
  ih->firstchild = bt_up;
  bt_up->parent = ih;
  bt_up->brother = bt_down;
  bt_down->parent = ih;
  
  IupSetAttribute(ih, "GAP",    "0");
  IupSetAttribute(ih, "MARGIN", "0x0");

  return IUP_NOERROR;
}

static int iSpinboxCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *spin;

  if (!params || !(params[0]))
    return IUP_ERROR;

  IupSetAttribute(ih, "GAP",       "0");
  IupSetAttribute(ih, "MARGIN",    "0x0");
  IupSetAttribute(ih, "ALIGNMENT", "ACENTER");

  /* IupText is already a child of Spinbox because of the IupHbox Create method */

  spin = IupSpin();
  iupChildTreeAppend(ih, spin);

  iupAttribSetStr(spin, "_IUPSPIN_BOX", (char*)ih);

  return IUP_NOERROR;
}

static void iSpinLoadImages(void)
{
  Ihandle* img;

  /* Spin UP image */
  unsigned char iupspin_up_img[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  /* Spin DOWN image */
  unsigned char iupspin_down_img[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1
  };

  img = IupImage(9, 6, iupspin_up_img);
  IupSetAttribute(img, "0", "0 0 0"); 
  IupSetAttribute(img, "1", "BGCOLOR"); 
  IupSetHandle("IupSpinUpImage", img); 

  img = IupImage(9, 6, iupspin_down_img);
  IupSetAttribute(img, "0", "0 0 0"); 
  IupSetAttribute(img, "1", "BGCOLOR"); 
  IupSetHandle("IupSpinDownImage", img); 
}

static void iSpinReleaseMethod(Iclass* ic)
{
  (void)ic;

  if (spin_timer)
  {
    /* no need to destroy the images, they will be destroyed by IupClose automatically */
    IupDestroy(spin_timer);
    spin_timer = NULL;
  }
}

Iclass* iupSpinboxGetClass(void)
{
  Iclass* ic = iupClassNew(iupHboxGetClass());

  ic->name = "spinbox";
  ic->format = "h"; /* one Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILD_ONE; /* fake value to define it as a container */
  ic->is_interactive = 0;

  iupClassRegisterCallback(ic, "SPIN_CB", "i");

  /* Class functions */
  ic->Create = iSpinboxCreateMethod;

  return ic;
}

Iclass* iupSpinGetClass(void)
{
  Iclass* ic = iupClassNew(iupVboxGetClass());

  ic->name = "spin";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILD_ONE; /* fake value to define it as a container */
  ic->is_interactive = 0;

  /* Class functions */
  ic->Create = iSpinCreateMethod;
  ic->Release = iSpinReleaseMethod;

  iupClassRegisterCallback(ic, "SPIN_CB", "i");

  if (!spin_timer)
  {
    iSpinLoadImages();

    spin_timer = IupTimer();
    IupSetCallback(spin_timer, "ACTION_CB", (Icallback) iSpinTimerCB);
  }

  return ic;
}

Ihandle* IupSpin(void)
{
  return IupCreate("spin");
}

Ihandle* IupSpinbox(Ihandle* ctrl)
{
  void *params[2];
  params[0] = (void*)ctrl;
  params[1] = NULL; /* must add NULL because spinbox inherit from Hbox */
  return IupCreatev("spinbox", params);
}
