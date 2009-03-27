/** \file
 * \brief iupmatrix control
 * keyboard control
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to control keys in the matrix and in the text edition        */
/**************************************************************************/

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include <cd.h>

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_scroll.h"
#include "iupmat_focus.h"
#include "iupmat_aux.h"
#include "iupmat_getset.h"
#include "iupmat_key.h"
#include "iupmat_mark.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"


int iupMatrixProcessKeyPress(Ihandle* ih, int c)
{
  /* This function is also called from inside the keyboard callbacks 
     of the Text and Dropdown list when in edition mode */

  int ret = IUP_IGNORE; /* default for processed keys */

  /* Hide (off) the marked cells if the key is not tab/del */
  if (c != K_TAB && c != K_sTAB && c != K_DEL && c != K_sDEL)
    iupMatrixMarkClearAll(ih, 1);

  /* If the focus is not visible, a scroll is done for that the focus to be visible */
  if (!iupMatrixAuxIsCellFullVisible(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell))
    iupMatrixScrollToVisible(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);

  switch (c)
  {
    case K_CR+2000:   /* used by the iMatrixEditTextKeyAny_CB and iMatrixEditDropDownKeyAny_CB */
      if (iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyCr(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_cHOME:
    case K_sHOME:
    case K_HOME:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyHome(ih);
      ih->data->homekeycount++;
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_cEND:
    case K_sEND:
    case K_END:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyEnd(ih);
      ih->data->endkeycount++;
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_sTAB:
    case K_TAB:
      return IUP_CONTINUE;  /* do not redraw */

    case K_cLEFT:
    case K_sLEFT:
    case K_LEFT:
      if (iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyLeft(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_cRIGHT:
    case K_sRIGHT:
    case K_RIGHT:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyRight(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_cUP:
    case K_sUP:
    case K_UP:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyUp(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break ;

    case K_cDOWN:
    case K_sDOWN:
    case K_DOWN:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyDown(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_sPGUP:
    case K_PGUP:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyPgUp(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_sPGDN:
    case K_PGDN:
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMatrixScrollKeyPgDown(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      break;

    case K_SP:
    case K_CR:
    case K_sCR:
      if (iupMatrixEditShow(ih))
        return IUP_IGNORE; /* do not redraw */
      break;

    case K_sDEL:
    case K_DEL:
      {
        int lin, col;
        char str[100];
        IFnii mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");

        iupMatrixStoreGlobalAttrib(ih);

        for(lin = 1; lin < ih->data->lines.num; lin++)
        {
          for(col = 1; col < ih->data->columns.num; col++)
          {
            if (iupMatrixMarkCellGet(ih, lin, col, mark_cb, str))
            {
              if (iupMatrixAuxCallEditionCbLinCol(ih, lin, col, 1) != IUP_IGNORE)
              {
                IFniis value_edit_cb;

                iupMatrixCellSetValue(ih, lin, col, NULL);

                value_edit_cb = (IFniis)IupGetCallback(ih, "VALUE_EDIT_CB");
                if (value_edit_cb)
                  value_edit_cb(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell, NULL);

                iupMatrixDrawCell(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
              }
            }
          }
        }
        break;
      }
    default:
      /* if a valid character is pressed enter edition mode */
      if (iup_isprint(c))
      {
        if (iupMatrixEditShow(ih))
        {
          if (ih->data->datah == ih->data->texth)
          {
            char value[2] = {0,0};
            value[0] = (char)c;
            IupStoreAttribute(ih->data->datah, "VALUE", value);
            IupSetAttribute(ih->data->datah, "CARET", "2");
          }
          return IUP_IGNORE; /* do not redraw */
        }
      }
      ret = IUP_DEFAULT; /* unprocessed keys */
      break;
  }

  iupMatrixDrawUpdate(ih);  
  return ret;
}

void iupMatrixKeyResetHomeEndCount(Ihandle* ih)
{
  ih->data->homekeycount = 0;
  ih->data->endkeycount = 0;
}

int iupMatrixKeyPress_CB(Ihandle* ih, int c, int press)
{
  int oldc = c;
  IFniiiis cb;

  if (!iupMatrixIsValid(ih, 1))
    return IUP_DEFAULT;

  if (!press)
    return IUP_DEFAULT;

  cb = (IFniiiis)IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    if (iup_isprint(c))
    {
      char value[2]={0,0};
      value[0] = (char)c;
      c = cb(ih, c, ih->data->lines.focus_cell, ih->data->columns.focus_cell, 0, value);
    }
    else
    {
      c = cb(ih, c, ih->data->lines.focus_cell, ih->data->columns.focus_cell, 0,
             iupMatrixCellGetValue(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell));
    }

    if (c == IUP_IGNORE || c == IUP_CLOSE || c == IUP_CONTINUE)
      return c;
    else if (c == IUP_DEFAULT)
      c = oldc;
  }

  if (c != K_HOME && c != K_sHOME)
    ih->data->homekeycount = 0;
  if (c != K_END && c != K_sEND)
    ih->data->endkeycount = 0;

  return iupMatrixProcessKeyPress(ih, c);
}
