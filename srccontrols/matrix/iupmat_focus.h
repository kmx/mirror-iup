/** \file
 * \brief iupmatrix focus control.
 *
 * See Copyright Notice in iup.h
 * $Id$
 */
 
#ifndef __IUPMAT_FOCUS_H 
#define __IUPMAT_FOCUS_H

#ifdef __cplusplus
extern "C" {
#endif

int  iMatrixFocusCB(Ihandle* ih, int focus);

void iMatrixShowFocus  (Ihandle* ih);
void iMatrixHideFocus  (Ihandle* ih);
void iMatrixSetFocusPos(Ihandle* ih, int lin, int col);

void iMatrixHideSetShowFocus(Ihandle* ih, int lin, int col);
void iMatrixSetShowFocus    (Ihandle* ih, int lin, int col);

int  iMatrixIsFocusVisible(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
