/** \file
 * \brief Control tree hierarchy manager.  
 * implements also IupDestroy
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h> 

#include "iup.h"

#include "iup_object.h"
#include "iup_dlglist.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_attrib.h" 
#include "iup_assert.h" 
#include "iup_str.h" 
#include "iup_drv.h" 


Ihandle* IupGetDialog(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  for (ih = ih; ih->parent; ih = ih->parent)
    ; /* empty*/

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    return ih;
  else if (ih->iclass->nativetype == IUP_TYPEMENU)
  {
    Ihandle *dlg;
    /* if ih is a menu then */
    /* searches all the dialogs that may have been associated with the menu. */
    for (dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
    {
      if (IupGetAttributeHandle(dlg, "MENU") == ih)
        return dlg;
    }
  }

  return NULL;
}

static void iChildDetach(Ihandle* parent, Ihandle* child)
{
  Ihandle *c, 
          *c_prev = NULL;

  /* Cleans the child entry inside the parent's child list */
  for (c = parent->firstchild; c; c = c->brother)
  {
    if (c == child) /* Found the right child */
    {
      if (c_prev == NULL)
        parent->firstchild = child->brother;
      else
        c_prev->brother = child->brother;
        
      child->brother = NULL;
      child->parent = NULL;
      return;
    }

    c_prev = c;
  }
}

void IupDetach(Ihandle *child)
{
  Ihandle *parent, *top_parent;

  iupASSERT(iupObjectCheck(child));
  if (!iupObjectCheck(child))
    return;

  IupUnmap(child);

  /* Not valid if does NOT has a parent */
  if (!child->parent)
    return;

  parent = child->parent;
  top_parent = iupChildTreeGetNativeParent(child);

  iChildDetach(parent, child);
  iupClassObjectChildRemoved(parent, child);

  while (parent && parent != top_parent)
  {
    parent = parent->parent;
    if (parent)
      iupClassObjectChildRemoved(parent, child);
  }
}

static int iChildFind(Ihandle* parent, Ihandle* child)
{
  Ihandle *c;

  /* Finds the reference child entry inside the parent's child list */
  for (c = parent->firstchild; c; c = c->brother)
  {
    if (c == child) /* Found the right child */
      return 1;
  }

  return 0;
}

static void iChildInsert(Ihandle* parent, Ihandle* ref_child, Ihandle* child)
{
  Ihandle *c, 
          *c_prev = NULL;

  /* Finds the reference child entry inside the parent's child list */
  for (c = parent->firstchild; c; c = c->brother)
  {
    if (c == ref_child) /* Found the right child */
    {
      child->parent = parent;
      child->brother = ref_child;

      if (c_prev == NULL)
        parent->firstchild = child;
      else
        c_prev->brother = child;
      return;
    }

    c_prev = c;
  }
}

Ihandle* IupInsert(Ihandle* parent, Ihandle* ref_child, Ihandle* child)
{
  Ihandle* top_parent = parent;

  iupASSERT(iupObjectCheck(parent));
  if (!iupObjectCheck(parent))
    return NULL;

  iupASSERT(iupObjectCheck(ref_child));
  if (!iupObjectCheck(ref_child))
    return NULL;

  iupASSERT(iupObjectCheck(child));
  if (!iupObjectCheck(child))
    return NULL;


  /* this will return the actual parent */
  parent = iupClassObjectGetInnerContainer(top_parent);
  if (!parent)
    return NULL;

  if (parent->iclass->childtype == IUP_CHILDNONE)
    return NULL;
  if (parent->iclass->childtype == IUP_CHILD_ONE && parent->firstchild)
    return NULL;


  /* if already at the parent box, allow to move even if mapped */
  if (parent->iclass->nativetype == IUP_TYPEVOID &&
      iChildFind(parent, child))
  {
    iChildDetach(parent, child);
    iChildInsert(parent, ref_child, child);
  }
  else
  {
    /* Not valid if it is mapped */
    if (child->handle)
      return NULL;

    iChildInsert(parent, ref_child, child);
    iupClassObjectChildAdded(parent, child);
    if (top_parent != parent)
      iupClassObjectChildAdded(top_parent, child);
  }

  return parent;
}

static void iChildAppend(Ihandle* parent, Ihandle* child)
{
  child->parent = parent;

  if (parent->firstchild == NULL)
    parent->firstchild = child;
  else
  {
    Ihandle* c = parent->firstchild;
    while (c->brother)
      c = c->brother;
    c->brother = child;
  }
}

Ihandle* IupAppend(Ihandle* parent, Ihandle* child)
{
  Ihandle* top_parent = parent;

  iupASSERT(iupObjectCheck(parent));
  if (!iupObjectCheck(parent))
    return NULL;

  iupASSERT(iupObjectCheck(child));
  if (!iupObjectCheck(child))
    return NULL;


  /* this will return the actual parent */
  parent = iupClassObjectGetInnerContainer(top_parent);
  if (!parent)
    return NULL;

  if (parent->iclass->childtype == IUP_CHILDNONE)
    return NULL;
  if (parent->iclass->childtype == IUP_CHILD_ONE && parent->firstchild)
    return NULL;


  /* if already at the parent box, allow to move even if mapped */
  if (parent->iclass->nativetype == IUP_TYPEVOID &&
      iChildFind(parent, child))
  {
    iChildDetach(parent, child);
    iChildAppend(parent, child);
  }
  else
  {
    /* Not valid if it is mapped */
    if (child->handle)
      return NULL;

    iChildAppend(parent, child);
    iupClassObjectChildAdded(parent, child);
    if (top_parent != parent)
      iupClassObjectChildAdded(top_parent, child);
  }

  return parent;
}

static void iChildReparent(Ihandle* child, Ihandle* new_parent)
{
  Ihandle *c;

  /* Forward the reparent to all native children */

  for (c = child->firstchild; c; c = c->brother)
  {
    if (c->iclass->nativetype != IUP_TYPEVOID)
      iupdrvReparent(c);
    else
      iChildReparent(c, new_parent);
  }
}

int IupReparent(Ihandle* child, Ihandle* parent)
{
  Ihandle* top_parent = parent;
  Ihandle* old_parent;

  iupASSERT(iupObjectCheck(parent));
  if (!iupObjectCheck(parent))
    return IUP_ERROR;

  iupASSERT(iupObjectCheck(child));
  if (!iupObjectCheck(child))
    return IUP_ERROR;


  /* this will return the actual parent */
  parent = iupClassObjectGetInnerContainer(top_parent);
  if (!parent)
    return IUP_ERROR;

  if (parent->iclass->childtype == IUP_CHILDNONE)
    return IUP_ERROR;
  if (parent->iclass->childtype == IUP_CHILD_ONE && parent->firstchild)
    return IUP_ERROR;


  /* both must be already mapped or both unmapped */
  if ((!parent->handle &&  child->handle) ||
      ( parent->handle && !child->handle))
    return IUP_ERROR;


  /* detach from old parent */
  old_parent = child->parent;
  iChildDetach(old_parent, child);
  iupClassObjectChildRemoved(old_parent, child);

 
  /* attach to new parent */
  iChildAppend(parent, child);
  iupClassObjectChildAdded(parent, child);
  if (top_parent != parent)
    iupClassObjectChildAdded(top_parent, child);


  /* no need to remap, just notify the native system */
  if (child->handle && parent->handle)
  {
    if (child->iclass->nativetype != IUP_TYPEVOID)
      iupdrvReparent(child);
    else
      iChildReparent(child, parent);
  }

  return IUP_NOERROR;
}

Ihandle* IupGetChild(Ihandle* ih, int pos)
{
  int p;
  Ihandle* child;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  for (p = 0, child = ih->firstchild; child; child = child->brother, p++)
  {
    if (p == pos)
      return child;
  }

  return NULL;
}

int IupGetChildPos(Ihandle* ih, Ihandle* child)
{
  int pos;
  Ihandle* c;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  for (pos = 0, c = ih->firstchild; c; c = c->brother, pos++)
  {
    if (c == child)
      return pos;
  }
  return -1;
}

int IupGetChildCount(Ihandle* ih)
{
  int count = 0;
  Ihandle* child;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  for (child = ih->firstchild; child; child = child->brother)
    count++;

  return count;
}

Ihandle* IupGetNextChild(Ihandle* ih, Ihandle* child)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  if (!child)
    return ih->firstchild;
  else
    return child->brother;
}

Ihandle* IupGetBrother(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  return ih->brother;
}

Ihandle* IupGetParent(Ihandle *ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;
    
  return ih->parent;
}

Ihandle* iupChildTreeGetNativeParent(Ihandle* ih)
{
  Ihandle* parent = ih->parent;
  while (parent && parent->iclass->nativetype == IUP_TYPEVOID)
    parent = parent->parent;
  return parent;
}

InativeHandle* iupChildTreeGetNativeParentHandle(Ihandle* ih)
{
  Ihandle* native_parent = iupChildTreeGetNativeParent(ih);
  return (InativeHandle*)iupClassObjectGetInnerNativeContainerHandle(native_parent, ih);
}
