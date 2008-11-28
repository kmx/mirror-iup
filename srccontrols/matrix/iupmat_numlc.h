/** \file
 * \brief iupmatrix. change number of collumns or lines.
 *
 * See Copyright Notice in iup.h
 * $Id$
 */
 
#ifndef __IUPMAT_NUMLC_H 
#define __IUPMAT_NUMLC_H

#ifdef __cplusplus
extern "C" {
#endif

char* iMatrixNlcGetNumLin(Ihandle* ih);
char* iMatrixNlcGetNumCol(Ihandle* ih);

void  iMatrixNlcAddLin(Ihandle* ih, const char* v);
void  iMatrixNlcDelLin(Ihandle* ih, const char* v);
void  iMatrixNlcAddCol(Ihandle* ih, const char* v);
void  iMatrixNlcDelCol(Ihandle* ih, const char* v);

void  iMatrixNlcNumLin(Ihandle* ih, const char* v);
void  iMatrixNlcNumCol(Ihandle* ih, const char* v);


#ifdef __cplusplus
}
#endif

#endif
