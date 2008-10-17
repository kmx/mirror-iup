/*IupTabs Example in C 
Creates a IupTabs control. */

#include <stdio.h>

#include "iup.h"
#include "iupcontrols.h"

int main(int argc, char **argv)
{
  Ihandle *dlg, *vbox1, *vbox2, *tabs1, *tabs2, *box;

  IupOpen(&argc, &argv);
  IupControlsOpen();                    /* Initializes the controls library */

  vbox1 = IupVbox(IupLabel("Inside Tab A"), IupButton("Button A", ""), NULL);
  vbox2 = IupVbox(IupLabel("Inside Tab B"), IupButton("Button B", ""), NULL);

  IupSetAttribute(vbox1, ICTL_TABTITLE, "Tab A");
  IupSetAttribute(vbox2, ICTL_TABTITLE, "Tab B");

  tabs1 = IupTabs(vbox1, vbox2, NULL);

  vbox1 = IupVbox(IupLabel("Inside Tab C"), IupButton("Button C", ""), NULL);
  vbox2 = IupVbox(IupLabel("Inside Tab D"), IupButton("Button D", ""), NULL);

  IupSetAttribute(vbox1, ICTL_TABTITLE, "Tab C");
  IupSetAttribute(vbox2, ICTL_TABTITLE, "Tab D");

  tabs2 = IupTabs(vbox1, vbox2, NULL);
  IupSetAttribute(tabs2, ICTL_TABTYPE, ICTL_LEFT);

  box = IupHbox(tabs1, tabs2, NULL);
  IupSetAttribute(box, "MARGIN", "10x10");
  IupSetAttribute(box, "GAP", "10");

  dlg = IupDialog(box);
  IupSetAttribute(dlg, "TITLE", "IupTabs");
  IupSetAttribute(dlg, "SIZE", "200x80");

  IupShowXY (dlg, IUP_CENTER, IUP_CENTER);
  IupMainLoop () ;
  IupDestroy(dlg);
  IupControlsClose();
  IupClose () ;

  return 0 ;
}