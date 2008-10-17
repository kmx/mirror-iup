/*IupFrame Example in C 
Draws a frame around a button. Note that IUP_FGCOLOR is added to the frame but it is inherited by the button. */

/* IupFrame example */
#include <stdio.h>
#include "iup.h"

int main(int argc, char **argv)
{
  /* IUP identifiers */
  Ihandle *frame ;
  Ihandle *dialog ;

  /* Initializes IUP */  
  IupOpen(&argc, &argv);

  /* Creates frame with a label */
  frame = IupFrame
          (
            IupHbox
            (
              IupFill(),
              IupLabel ("IupFrame Attributes:\nFGCOLOR = \"255 0 0\"\nSIZE = \"EIGHTHxEIGHTH\"\nTITLE = \"This is the frame\"\nMARGIN = \"10x10\""),
              IupFill(),
              NULL
            )
          ) ;

  /* Sets frame's attributes */
  IupSetAttributes(frame, "FGCOLOR=\"255 0 0\", SIZE=EIGHTHxEIGHTH, TITLE=\"This is the frame\", MARGIN=10x10");

  /* Creates dialog  */
  dialog = IupDialog(frame);

  /* Sets dialog's title */
  IupSetAttribute(dialog, IUP_TITLE, "IupFrame");

  IupShow( dialog ) ;  /* Shows dialog in the center of the screen */
  IupMainLoop() ;      /* Initializes IUP main loop */
  IupDestroy(dialog);
  IupClose() ;         /* Finishes IUP */

  return 0 ;
}