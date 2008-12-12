/** \file
 * \brief Base Class
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_CLASSBASE_H 
#define __IUP_CLASSBASE_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup iclassbase Base Class
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclass
 */


/** Register all common base attributes: \n
 * WID                                   \n
 * SIZE, RASTERSIZE, POSITION            \n
 * FONT (and derived)                    \n\n
 * All controls that are positioned inside a dialog must register all common base attributes.
 * \ingroup iclassbase */
void iupBaseRegisterCommonAttrib(Iclass* ic);

/** Register all visual base attributes: \n
 * VISIBLE, ACTIVE                       \n
 * ZORDER, X, Y                          \n
 * TIP (and derived)                     \n\n
 * All controls that are positioned inside a dialog must register all visual base attributes.
 * \ingroup iclassbase */
void iupBaseRegisterVisualAttrib(Iclass* ic);

/* Register driver dependent common attributes. 
   Used only from iupBaseRegisterCommonAttrib */
void iupdrvBaseRegisterCommonAttrib(Iclass* ic);

/** Updates the expand member of the IUP object from the EXPAND attribute.
 * Should be called in the begining of the ComputeNaturalSize of each container.
 * \ingroup iclassbase */
void iupBaseContainerUpdateExpand(Ihandle* ih);

/* Updates the SIZE attribute if defined. 
   Called only from iupdrvSetStandardFontAttrib. */
void iupBaseUpdateSizeAttrib(Ihandle* ih);


/** \defgroup iclassbasemethod Base Class Methods
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 */

/** The \ref Iclass::SetCurrentSize method for controls that are not containers.
 * \ingroup iclassbasemethod */
void iupBaseSetCurrentSizeMethod(Ihandle* ih, int w, int h, int shrink);

/** Partial \ref Iclass::SetCurrentSize method for containers, 
 * it must be complemented with a loop for the children removing the element decoration or margins.
 * \ingroup iclassbasemethod */
void iupBaseContainerSetCurrentSizeMethod(Ihandle* ih, int w, int h, int shrink);

/** The \ref Iclass::SetPosition method for controls that are not containers.
 * \ingroup iclassbasemethod */
void iupBaseSetPositionMethod(Ihandle* ih, int x, int y);

/** Driver dependent \ref Iclass::LayoutUpdate method.
 * \ingroup iclassbasemethod */
void iupdrvBaseLayoutUpdateMethod(Ihandle *ih);

/** Driver dependent \ref Iclass::UnMap method.
 * \ingroup iclassbasemethod */
void iupdrvBaseUnMapMethod(Ihandle* ih);



/** \defgroup iclassbaseattribfunc Base Class Attribute Functions
 * \par
 * Used by the controls for iupClassRegisterAttribute. 
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 * @{
 */

int iupBaseNoSetAttrib(Ihandle* ih, const char* value);
char* iupBaseNoGetAttrib(Ihandle* ih);

/* common */
char* iupBaseGetWidAttrib(Ihandle* ih);
int iupBaseSetNameAttrib(Ihandle* ih, const char* value);
int iupBaseSetRasterSizeAttrib(Ihandle* ih, const char* value);
int iupBaseSetSizeAttrib(Ihandle* ih, const char* value);
char* iupBaseGetSizeAttrib(Ihandle* ih);
char* iupBaseGetRasterSizeAttrib(Ihandle* ih);

/* visual */
char* iupBaseGetVisibleAttrib(Ihandle* ih);
int iupBaseSetVisibleAttrib(Ihandle* ih, const char* value);
char* iupBaseGetActiveAttrib(Ihandle *ih);
int iupBaseSetActiveAttrib(Ihandle* ih, const char* value);
int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value);
char *iupdrvBaseGetXAttrib(Ihandle *ih);
char *iupdrvBaseGetYAttrib(Ihandle *ih);
int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value);
int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value);
int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value);
int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value);
char* iupBaseNativeParentGetBgColorAttrib(Ihandle* ih);

/* other */
char* iupBaseContainerGetExpandAttrib(Ihandle* ih);
int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value);
char* iupdrvBaseGetClientSizeAttrib(Ihandle* ih);

/* Windows Only */
char* iupdrvBaseGetTitleAttrib(Ihandle* ih);
int iupdrvBaseSetTitleAttrib(Ihandle* ih, const char* value);

/** @} */



/** \defgroup iclassbaseutil Base Class Utilities
 * \par
 * See \ref iup_classbase.h
 * \ingroup iclassbase
 * @{
 */

#define iupMAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#define iupROUND(_x) ((int)((_x)>0? (_x)+0.5: (_x)-0.5))

#define iupCOLOR8TO16(_x) ((unsigned short)(_x*257))  /* 65535/255 = 257 */
#define iupCOLOR16TO8(_x) ((unsigned char)(_x/257))

enum{IUP_ALIGN_ALEFT, IUP_ALIGN_ACENTER, IUP_ALIGN_ARIGHT};
#define IUP_ALIGN_ABOTTOM IUP_ALIGN_ARIGHT
#define IUP_ALIGN_ATOP IUP_ALIGN_ALEFT

enum{IUP_SB_NONE, IUP_SB_HORIZ, IUP_SB_VERT};
int iupBaseGetScrollbar(Ihandle* ih);

/** @} */


#ifdef __cplusplus
}
#endif

#endif
