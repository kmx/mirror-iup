/** \file
 * \brief Control Hierarchy Tree management.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_CHILDTREE_H 
#define __IUP_CHILDTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup childtree Child Tree Utilities
 * \par
 * Some native containers have an internal native child that 
 * will be the actual container of the children. This native container is 
 * returned by \ref iupClassObjectGetInnerNativeContainerHandle.
 * \par
 * Some native elements need an extra parent, the ih->handle points to the main element itself, 
 * NOT to the extra parent. This extra parent is stored as "_IUP_EXTRAPARENT".
 * \par
 * See \ref iup_childtree.h
 * \ingroup object */

/** Returns the native parent. It simply excludes containers that are from IUP_TYPEVOID classes.
 * \ingroup childtree */
Ihandle* iupChildTreeGetNativeParent(Ihandle* ih);

/** Returns the native parent handle. Uses \ref iupChildTreeGetNativeParent and \ref iupClassObjectGetInnerNativeContainerHandle.
 * \ingroup childtree */
InativeHandle* iupChildTreeGetNativeParentHandle(Ihandle* ih);


/* Other functions declared in <iup.h> and implemented here. 
IupGetDialog
IupDetach
IupAppend
IupGetChild
IupGetNextChild
IupGetBrother
IupGetParent
*/


#ifdef __cplusplus
}
#endif

#endif
