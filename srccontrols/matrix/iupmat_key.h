/** \file
 * \brief iupmatrix. keyboard control.
 *
 * See Copyright Notice in iup.h
 * $Id$
 */
 
#ifndef __IUPMAT_KEY_H 
#define __IUPMAT_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

int  iMatrixKeyPressCB(Ihandle* ih, int c, int press);
int  iMatrixKey(Ihandle* ih, int c);

void iMatrixResetKeyCount  (void);
int  iMatrixGetHomeKeyCount(void);
int  iMatrixGetEndKeyCount (void);

#ifdef __cplusplus
}
#endif

#endif
