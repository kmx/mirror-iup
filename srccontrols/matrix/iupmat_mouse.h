/** \file
 * \brief iMatrixrix control
 * mouse events.
 *
 * See Copyright Notice in iup.h
 * $Id$
 */
 
#ifndef __IUPMAT_MOUSE_H 
#define __IUPMAT_MOUSE_H

#ifdef __cplusplus
extern "C" {
#endif

int iMatrixMouseButtonCB (Ihandle* ih, int b, int press, int x, int y, char* r);
int iMatrixMouseMoveCB   (Ihandle* ih, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
