/** \file
 * \brief iupmatrix control
 * mouse events
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to handle mouse events                                       */
/**************************************************************************/

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include <cd.h>

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_colres.h"
#include "iupmat_aux.h"
#include "iupmat_focus.h"
#include "iupmat_mouse.h"
#include "iupmat_key.h"
#include "iupmat_mark.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"


static void iMatrixMouseCallMoveCb(Ihandle* ih, int lin, int col)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "MOUSEMOVE_CB");
  if (cb)
    cb(ih, lin, col);
}

static int iMatrixMouseCallClickCb(Ihandle* ih, int press, int lin, int col, char* r)
{
  IFniis cb;

  if (press)
    cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  else
    cb = (IFniis)IupGetCallback(ih, "RELEASE_CB");

  if (cb)
    return cb(ih, lin, col, r);
                       
  return IUP_DEFAULT;
}

static void iMatrixMouseLeftPress(Ihandle* ih, int lin, int col, int shift, int ctrl, int dclick)
{
  if (dclick)
  {
    iupMatrixMarkMouseReset(ih);

    /* if a double click NOT in the current cell */
    if (lin != ih->data->lines.focus_cell || col != ih->data->columns.focus_cell)
    {
      /* leave the previous cell if the matrix previously had the focus */
      if (ih->data->has_focus && iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        return;

      if (lin>0 && col>0)
        iupMatrixFocusSet(ih, lin, col);

      iupMatrixAuxCallEnterCellCb(ih);
    }
    
    if (iupMatrixEditShow(ih))
    {
      if(ih->data->datah == ih->data->droph) 
        IupSetAttribute(ih->data->datah, "SHOWDROPDOWN", "YES");

      if (iupStrEqualNoCase(IupGetGlobal("DRIVER"), "Motif"))
        if(atoi(IupGetGlobal("MOTIFNUMBER")) < 2203) /* since OpenMotif version 2.2.3 this is not necessary */
          iupAttribSetStr(ih, "_IUPMAT_DOUBLE_CLICK", "1");
    }
  }
  else /* single click */
  {
    if (shift && ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
    {
      iupMatrixMarkMouseBlock(ih, lin, col);
    }
    else
    {
      ih->data->leftpressed = 1;

      if (lin>0 && col>0)
      {
        if (iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
          return;

        iupMatrixFocusSet(ih, lin, col);

        iupMatrixAuxCallEnterCellCb(ih);
      }

      if (ih->data->mark_mode != IMAT_MARK_NO)
        iupMatrixMarkMouseItem(ih, ctrl, lin, col);
    }
  }
}

int iupMatrixMouseButton_CB(Ihandle* ih, int b, int press, int x, int y, char* r)
{
  int lin=-1, col=-1;

  if (!iupMatrixIsValid(ih, 0))
    return IUP_DEFAULT;

  /* reset press state */
  ih->data->leftpressed = 0;

  if (press)
  {
    /* The edit Focus callback is not called when the user clicks in the parent canvas. 
       so we have to compensate that. */
    iupMatrixEditForceHidden(ih);
    ih->data->has_focus = 1;
  }

  iupMatrixAuxGetLinColFromXY(ih, x, y, &lin, &col);

  if (b == IUP_BUTTON1)
  {
    if (press)
    {
      iupMatrixKeyResetHomeEndCount(ih);

      if (iupMatrixColResStart(ih, x, y))
        return IUP_DEFAULT;  /* Resize of the width a of a column was started */

      if (lin!=-1 && col!=-1)
        iMatrixMouseLeftPress(ih, lin, col, isshift(r), iscontrol(r), isdouble(r));
    }
    else
    {
      iupMatrixMarkMouseReset(ih);

      if (iupMatrixColResIsResizing(ih))  /* If it was made a column resize, finish it */
        iupMatrixColResFinish(ih, x);
    }
  }

  if (lin!=-1 && col!=-1)
  {
    if (iMatrixMouseCallClickCb(ih, press, lin, col, r) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  iupMatrixDrawUpdate(ih);
  return IUP_DEFAULT;
}

int iupMatrixMouseMove_CB(Ihandle* ih, int x, int y)
{
  int lin, col;

  if (!iupMatrixIsValid(ih, 0))
    return IUP_DEFAULT;

  if (ih->data->leftpressed && ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
    iupMatrixMarkMouseDrag(ih, x, y);
  else if(iupMatrixColResIsResizing(ih)) /* Make a resize in a column size */
    iupMatrixColResMove(ih, x);
  else /* Change cursor when it is passed on a join involving column titles */
    iupMatrixColResCheckChangeCursor(ih, x, y);

  if (iupMatrixAuxGetLinColFromXY(ih, x, y, &lin, &col))
    iMatrixMouseCallMoveCb(ih, lin, col);

  return IUP_DEFAULT;
}
