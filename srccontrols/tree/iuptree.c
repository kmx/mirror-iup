/** \file
* \brief IupTree Control Core
*
* See Copyright Notice in iup.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"
#include "iuptree.h"
#include "iupkey.h"

#include <cd.h>
#include <cdiup.h>
#include <cddbuf.h>

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_globalattrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_controls.h"
#include "iup_dialog.h"
#include "iup_cdutil.h"

#include "treedef.h"
#include "treecd.h"
#include "itgetset.h"
#include "itdraw.h"
#include "itkey.h"
#include "itmouse.h"
#include "itfind.h"
#include "itedit.h"
#include "itscroll.h"
#include "itcallback.h"
#include "itimage.h"


/* Variable that stores the control iTreeKey status */
extern int tree_ctrl;

/* Variable that stores the tree_shift iTreeKey status */
extern int tree_shift;

/***************************************************************************/

/* Verifies which scrolling bars have been defined
   sbv: =0 doesn't have vertical scrolling bar
        =1         has  vertical scrolling bar
   sbh: =0 doesn't have horizontal scrolling bar
        =1         has horizontal scrolling bar    */
static void iTreeGetSB(Ihandle* ih, int* sbh, int* sbv)
{
  char* sb = IupGetAttribute(ih, "SCROLLBAR");

  if(sb == NULL)
    sb = "YES";

  *sbh = *sbv = 0;

  if(iupStrEqualNoCase(sb, "YES") || iupStrEqualNoCase(sb, "ON"))
    *sbh = *sbv = 1;
  else if(iupStrEqualNoCase (sb, "VERTICAL"))
    *sbv = 1;
  else if(iupStrEqualNoCase (sb, "HORIZONTAL"))
    *sbh = 1;
}


/* Initializes tree data */
static void iTreeInitTreeRoot(Ihandle* ih)
{
  Node root_node;
  root_node = (Node)malloc(sizeof(struct Node_));

  ih->data->root     = root_node;
  ih->data->selected = root_node;
  ih->data->starting = root_node;
  ih->data->addexpanded = YES;
  ih->data->tree_ctrl   = NO;
  ih->data->tree_shift  = NO;

  root_node->depth = 0;
  root_node->kind  = ITREE_BRANCH;
  root_node->state = ITREE_EXPANDED;
  root_node->marked  = YES;
  root_node->visible = YES;
  root_node->name = NULL;
  root_node->next = NULL;
  root_node->userid = NULL;
  root_node->imageinuse = NO;
  root_node->expandedimageinuse = NO;
  root_node->text_color = 0x000000L;
}

static int iTreeInitTree(Ihandle* ih)
{
 int w, h;

 cdCanvasBackground(ih->data->cddbuffer, ITREE_TREE_BGCOLOR);
 cdCanvasGetSize(ih->data->cddbuffer, &w, &h, NULL, NULL);

 ih->data->XmaxC = w - 1;
 ih->data->YmaxC = h - 1;

 IupSetAttribute(ih, "BGCOLOR", ITREE_TREE_BGCOLORSTRING);
 IupSetAttribute(ih, "DX", "1.0");
 IupSetAttribute(ih, "DY", "1.0");

 return 0;
}

/**************************************************************************
***************************************************************************
*
*   Callbacks for the canvas
*
***************************************************************************
***************************************************************************/

static void iTreeCreateCanvas(Ihandle* ih)
{
  /* update canvas size */
  cdCanvasActivate(ih->data->cdcanvas);

  /* this can fail if canvas size is zero */
  ih->data->cddbuffer = cdCreateCanvas(CD_DBUFFER, ih->data->cdcanvas);

  if(!ih->data->cddbuffer)
    return;

  /* update canvas size */
  cdCanvasActivate(ih->data->cddbuffer);

  iTreeInitTree(ih);
}

/* Callback called when the tree has its size altered
   dx, dy: Canvas size, in pixels.                    */
static int iTreeResizeCB(Ihandle* ih, int dx, int dy)
{
  if(!ih->data->cddbuffer)
    iTreeCreateCanvas(ih);

  if(!ih->data->cddbuffer)
    return IUP_DEFAULT;

  cdCanvasActivate(ih->data->cddbuffer);

  ih->data->XmaxC = dx - 1;
  ih->data->YmaxC = dy - 1;

  iTreeEditCheckHidden(ih);
  
  return IUP_DEFAULT;
}

/* Callback called when the tree is scrolled.  */
static int iTreeScrollCB(Ihandle* ih)
{
  int err;

  CdActivate(ih, err);
  
  if(err == CD_OK)
  {
    iTreeEditCheckHidden(ih);

    cdCanvasNativeFont(ih->data->cddbuffer, IupGetAttribute(ih, "FONT"));
    iTreeDrawTree(ih);
    cdCanvasFlush(ih->data->cddbuffer);
  }

  if(ih && ih->data->selected && ih->data->selected->visible == NO)
    iTreeGSSetValue(ih, "PREVIOUS", 0);
  
  return IUP_DEFAULT;
}

static void iTreeSafeStrcpy(char *dest, char *orig)
{
  if(orig && dest)
    strcpy(dest, orig);
}

/* This function updates the size of the scrollbar or
   hides it if necessary based on DX                  */
static void iTreeUpdateScrollPos(Ihandle* ih)
{
  char* posx = iupStrGetMemory(10);
  char* posy = iupStrGetMemory(10);
  char*   dx = iupStrGetMemory(10);
  char*   dy = iupStrGetMemory(10);

  iTreeSafeStrcpy(dx, IupGetAttribute(ih, "DX"));
  iTreeSafeStrcpy(dy, IupGetAttribute(ih, "DY"));

  strcpy(posy, IupGetAttribute(ih, "POSY"));
  if(atoi(posy) < 0 || atoi(dy) == 1)
    IupSetAttribute(ih, "POSY", "0");
  else
    IupStoreAttribute(ih, "POSY", posy);

  strcpy(posx, IupGetAttribute(ih, "POSX"));
  if(atoi(posx) < 0 || atoi(dx) == 1)
    IupSetAttribute(ih, "POSX", "0");
  else
    IupStoreAttribute(ih, "POSX", posx);
}

static int iTreeRepaintCB(Ihandle* ih)
{
  if(!ih->data->cddbuffer)
    return IUP_DEFAULT;

  return iTreeRepaint(ih);
}

/* Callback called when the tree needs to be redrawn. */
int iTreeRepaint(Ihandle* ih)
{
  int err;

  if(!ih->data->cddbuffer)
    return IUP_DEFAULT;

  CdActivate(ih, err);
  
  if(err == CD_OK)
  {
    cdCanvasNativeFont(ih->data->cddbuffer, IupGetAttribute(ih, "FONT"));
    iTreeDrawTree(ih);  /* FIXME: split the calcsize from the redraw */
    iTreeUpdateScrollPos(ih);

    CdActivate(ih, err);
    iTreeDrawTree(ih);
    cdCanvasFlush(ih->data->cddbuffer);
  }

  if(ih && ih->data->selected && ih->data->selected->visible == NO)
    iTreeGSSetValue(ih, "PREVIOUS", 1);

  return IUP_DEFAULT;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

Ihandle* IupTree(void)
{
  return IupCreate("tree");
}

static int iTreeCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  if(ih->data) free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION",      (Icallback)iTreeRepaintCB);
  IupSetCallback(ih, "RESIZE_CB",   (Icallback)iTreeResizeCB);
  IupSetCallback(ih, "SCROLL_CB",   (Icallback)iTreeScrollCB);
  IupSetCallback(ih, "BUTTON_CB",   (Icallback)iTreeMouseButtonCB);
  IupSetCallback(ih, "MOTION_CB",   (Icallback)iTreeMouseMotionCB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iTreeCallKeyPressCB);

  /* Creates the rename node text field */
//  iTreeEditCreate(ih);

  iTreeInitTreeRoot(ih);

  iTreeGSSetImage("IMGLEAF", ih->data->image_leaf, ih->data->color_leaf, ih->data->marked_color_leaf, ITREE_TREE, NULL);
  iTreeGSSetImage("IMGCOLLAPSED", ih->data->image_collapsed, ih->data->color_collapsed, ih->data->marked_color_collapsed, ITREE_TREE, NULL);
  iTreeGSSetImage("IMGEXPANDED", ih->data->image_expanded, ih->data->color_expanded, ih->data->marked_color_expanded, ITREE_TREE, NULL);

  /* Initially marked element is root */
  iTreeGSSetValue(ih, "0", 0);

  /* change the IupCanvas default values */
  /* The tree has scrollbar by default */
  iupAttribSetStr(ih, "SCROLLBAR", "YES");
  iupAttribSetStr(ih, "BORDER", "NO");

  return IUP_NOERROR;
}

static void iTreeDestroyMethod(Ihandle* ih)
{
  Node temp, next;

  temp = ih->data->root;
  while(temp)
  {
    next = temp->next;

    if(temp->name)
      free(temp->name);

    free(temp);
    temp = next;
  }

  if(ih->data->cddbuffer)
    cdKillCanvas(ih->data->cddbuffer);

  if(ih->data->cdcanvas)
    cdKillCanvas(ih->data->cdcanvas);
}

static int iTreeMapMethod(Ihandle* ih)
{
  ih->data->cdcanvas = cdCreateCanvas(CD_IUP, ih);
  if(!ih->data->cdcanvas)
    return IUP_ERROR;

  /* this can fail if canvas size is zero */
  ih->data->cddbuffer = cdCreateCanvas(CD_DBUFFER, ih->data->cdcanvas);

  iTreeInitTree(ih);

  return IUP_NOERROR;
}

/* Calculates the natural width of the tree */
static int iTreeGetNaturalWidth(Ihandle* ih)
{
  (void)ih;
  return ITREE_TREE_WIDTH;
}

/* Calculates the natural height of the tree */
static int iTreeGetNaturalHeight(Ihandle* ih)
{
  (void)ih;
  return ITREE_TREE_HEIGHT;
}

static void iTreeComputeNaturalSizeMethod(Ihandle* ih)
{
  int width, height, sizex, sizey;
  int sbh, sbv, sbw = 0;

  /* Get scrollbar parameters */
  iTreeGetSB(ih, &sbh, &sbv);
  if(sbv || sbh)
    sbw = iupdrvGetScrollbarSize();

  /* Gets values associated with SIZE attribute */
  sizex = ih->userwidth;
  sizey = ih->userheight;

  /* Get the natural width */
  if(sizex)
    width = sizex;
  else
    width = iTreeGetNaturalWidth(ih) + sbw * sbv;

  /* Get the natural height */
  if(sizey)
    height = sizey;
  else
    height = iTreeGetNaturalHeight(ih) + sbw * sbh;

  ih->naturalwidth  = width;
  ih->naturalheight = height;
}


static void iTreeDestroyImage(char* name)
{
  Ihandle *image = IupGetHandle(name);
  if (image) IupDestroy(image);
}

static void iTreeReleaseMethod(Iclass* ic)
{
  (void)ic;
  iTreeDestroyImage("IupTreeDragCursor");

  iTreeDestroyImage("IMGLEAF");
  iTreeDestroyImage("IMGCOLLAPSED");
  iTreeDestroyImage("IMGEXPANDED");
  iTreeDestroyImage("IMGBLANK");
  iTreeDestroyImage("IMGPAPER");
}

/*****************************************************************************/
/***** SET AND GET ATTRIBUTES ************************************************/
/*****************************************************************************/
/* Scrollbar, Font, ShowDragDrop, ShowRename... will not be defined...       */

static int iTreeSetAddExpandedAttrib(Ihandle* ih, const char* value)
{
  iTreeGSAddExpanded(ih, value);
  return 1;
}

static int iTreeSetRenameCaretAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "CARET", value);
  iupAttribStoreStr(ih, "CARET", value);
  return 1;
}

static char* iTreeGetRenameCaretAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->data->texth, "CARET");
}

static int iTreeSetRenameSelectionAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "SELECTION", value);
  iupAttribStoreStr(ih, "SELECTION", value);
  return 1;
}

static char* iTreeGetRenameSelectionAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->data->texth, "SELECTION");
}

static int iTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  /* Unmarks all nodes if control is not pressed and value is not ITREE_INVERT,
  * because ITREE_INVERT does not depend on the control iTreeKey status. */
  if(tree_ctrl == NO && value && !iupStrEqualNoCase(value, "ITREE_INVERT"))
    iTreeGSSetMarked(ih, NO, 0);

  iTreeGSSetValue(ih, value, 0);

  /* Marks block if tree_shift is pressed and value is not CLEARALL, to guarantee
  * CLEARALL will work even if there is a block marked */
  if(tree_shift == YES && value && !iupStrEqualNoCase(value, "CLEARALL"))
    iTreeGSSetValue(ih, "BLOCK", 0);

  return 1;
}

static char* iTreeGetValueAttrib(Ihandle* ih)
{
  return iTreeGSGetValue(ih);
}

static int iTreeSetMarkedAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSMark(ih, &name_id[strlen("MARKED")], value);
  iupAttribSetStr(ih, name_id, NULL);  /* Do not store id dependent attributes in the environment */
  return 1;
}

static char* iTreeGetMarkedAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetMarked(ih, &name_id[strlen("MARKED")]);
}

static int iTreeSetCtrlAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetCtrl(ih, value);
  return 1;
}

static char* iTreeGetCtrlAttrib(Ihandle* ih)
{
  return iTreeGSGetCtrl(ih);
}

static int iTreeSetShiftAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetShift(ih, value);
  return 1;
}

static char* iTreeGetShiftAttrib(Ihandle* ih)
{
  return iTreeGSGetShift(ih);
}

static int iTreeSetStartingAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetStarting(ih, value);
  return 1;
}

static char* iTreeGetStartingAttrib(Ihandle* ih)
{
  return iTreeGSGetStarting(ih);
}

static int iTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetImage(value, ih->data->image_leaf, ih->data->color_leaf, ih->data->marked_color_leaf, ITREE_TREE, NULL);
  return 1;
}

static int iTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetImage(value, ih->data->image_collapsed, ih->data->color_collapsed, ih->data->marked_color_collapsed, ITREE_TREE, NULL);
  return 1;
}

static int iTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  iTreeGSSetImage(value, ih->data->image_expanded, ih->data->color_expanded, ih->data->marked_color_expanded, ITREE_TREE, NULL);
  return 1;
}

/* IUP_IMAGEEXPANDED *MUST* appear before IUP_IMAGE, or else 
   IUP_IMAGEEXPANDED will never be called...*/
static int iTreeSetImageExpandedAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  Node node = iTreeFindNodeFromString(ih, &name_id[strlen("IMAGEEXPANDED")]);
  iTreeGSSetImage(value, node->expandedimage, node->expandedcolor, node->expandedmarked_color, ITREE_NODEEXPANDED, node);
  iupAttribSetStr(ih, name_id, NULL);  /* Do not store id dependent attributes in the environment */
  return 1;
}

static int iTreeSetImageAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  Node node = iTreeFindNodeFromString(ih, &name_id[strlen("IMAGE")]);
  iTreeGSSetImage(value, node->image, node->color, node->marked_color, ITREE_NODE, node);
  iupAttribSetStr(ih, name_id, NULL);  /* Do not store id dependent attributes in the environment */
  return 1;
}

static int iTreeSetNameAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSSetName(ih, &name_id[strlen("NAME")], value);
  return 1;
}

static char* iTreeGetNameAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetName(ih, &name_id[strlen("NAME")]);
}

static int iTreeSetStateAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSSetState(ih, &name_id[strlen("STATE")], value);
  return 1;
}

static char* iTreeGetStateAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetState(ih, &name_id[strlen("STATE")]);
}

static int iTreeSetDepthAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSSetDepth(ih, &name_id[strlen("DEPTH")], value);
  return 1;
}

static char* iTreeGetDepthAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetDepth(ih, &name_id[strlen("DEPTH")]);
}

static char* iTreeGetKindAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetKind(ih, &name_id[strlen("KIND")]);
}

static char* iTreeGetParentAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetParent(ih, &name_id[strlen("PARENT")]);
}

static int iTreeSetColorAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSSetColor(ih, &name_id[strlen("COLOR")], value);
  return 1;
}

static char* iTreeGetColorAttrib(Ihandle* ih, const char* name_id)
{
  return iTreeGSGetColor(ih, &name_id[strlen("COLOR")]);
}

static int iTreeSetAddLeafAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  char* next = iupStrGetMemory(10);

  if(!iupStrEqualNoCase(name_id, "ADDLEAF"))
    sprintf(next, "%d", atoi(&name_id[strlen("ADDLEAF")]) + 1);
  else
    sprintf(next, "%d", atoi(IupGetAttribute(ih, "VALUE")) + 1);

  if(iTreeGSAddNode(ih, &name_id[strlen("ADDLEAF")], ITREE_LEAF))
    iTreeGSSetName(ih, next, value);

  return 1;
}

static int iTreeSetAddBranchAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  char* next = iupStrGetMemory(10);

  if(!iupStrEqualNoCase(name_id, "ADDBRANCH"))
    sprintf(next, "%d", atoi(&name_id[strlen("ADDBRANCH")]) + 1);
  else
    sprintf(next, "%d", atoi(IupGetAttribute(ih, "VALUE")) + 1);

  if(iTreeGSAddNode(ih, &name_id[strlen("ADDBRANCH")], ITREE_BRANCH))
    iTreeGSSetName(ih, next, value);

  return 1;
}

static int iTreeSetDelNodeAttrib(Ihandle* ih, const char* name_id, const char* value)
{
  iTreeGSDelNode(ih, &name_id[strlen("DELNODE")], value);
  return 1;
}

static int iTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  int x, y, text_x;
  (void)value;

  if(iTreeKeyNodeCalcPos(ih, &x, &y, &text_x))
  {
    if(IupGetInt(ih, "SHOWRENAME"))
      iTreeEditShow(ih, text_x, x, y);
    else
      iTreeCallRenameNodeCB(ih);
  }
  return 0;
}

static int iTreeSetRedrawAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iTreeRepaint(ih);
  return 0;
}

static int iTreeSetRepaintAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iTreeRepaint(ih);
  return 0;
}

static int iTreeSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);
  iTreeRepaint(ih);
  return 1;
}

static void iTreeCreateCursor(void)
{
  Ihandle *imgcursor;
  unsigned char tree_img_drag_cur[16*24] = 
  {
   1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  ,1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0
  ,1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0
  ,1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0
  ,1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0
  ,1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0
  ,1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0
  ,1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0
  ,1,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0
  ,1,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0
  ,1,2,2,2,2,2,2,1,1,1,1,1,0,0,0,0
  ,1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0
  ,1,2,2,1,1,2,2,1,0,0,0,0,0,0,0,0
  ,1,2,1,0,0,1,2,2,1,0,0,0,0,0,0,0
  ,1,1,0,0,0,1,2,2,1,0,0,0,0,0,0,0
  ,1,0,0,1,2,0,1,2,2,1,2,1,2,1,2,1
  ,0,0,0,2,1,2,1,2,2,1,0,2,1,2,1,2
  ,0,0,0,1,2,0,0,1,2,2,1,0,0,0,2,1
  ,0,0,0,2,1,0,0,1,2,1,1,0,0,0,1,2
  ,0,0,0,1,2,0,0,0,1,0,0,0,0,0,2,1
  ,0,0,0,2,1,0,0,0,0,0,0,0,0,0,1,2
  ,0,0,0,1,2,0,0,0,0,0,0,0,0,0,2,1
  ,0,0,0,2,1,2,1,2,1,2,1,2,1,2,1,2
  ,0,0,0,1,2,1,2,1,2,1,2,1,2,1,2,1
  };

  imgcursor = IupImage(16, 24, tree_img_drag_cur);
  IupSetAttribute(imgcursor, "0", "BGCOLOR"); 
  IupSetAttribute(imgcursor, "1", "0 0 0"); 
  IupSetAttribute(imgcursor, "2", "255 255 255"); 
  IupSetAttribute(imgcursor, "HOTSPOT", "0:0");
  IupSetHandle("IupTreeDragCursor", imgcursor); 
}

Iclass* iupTreeGetClass(void)
{
  Iclass* ic = iupClassNew(iupCanvasGetClass());

  ic->name = "tree";
  ic->format = "S"; /* one optional string */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->Create  = iTreeCreateMethod;
  ic->Destroy = iTreeDestroyMethod;
  ic->Map     = iTreeMapMethod;
  ic->ComputeNaturalSize = iTreeComputeNaturalSizeMethod;
  ic->Release = iTreeReleaseMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* IupTree Callbacks */
  iupClassRegisterCallback(ic, "SELECTION_CB",      "ii");
  iupClassRegisterCallback(ic, "MULTISELECTION_CB", "ii");
  iupClassRegisterCallback(ic, "BRANCHOPEN_CB",     "i");
  iupClassRegisterCallback(ic, "BRANCHCLOSE_CB",    "i");
  iupClassRegisterCallback(ic, "EXECUTELEAF_CB",    "i");
  iupClassRegisterCallback(ic, "RENAMENODE_CB",     "is");
  iupClassRegisterCallback(ic, "SHOWRENAME_CB",     "i");
  iupClassRegisterCallback(ic, "RENAME_CB",         "is");
  iupClassRegisterCallback(ic, "DRAGDROP_CB",       "iiii");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB",     "i");

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "ADDEXPANDED", NULL, iTreeSetAddExpandedAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAMECARET", iTreeGetRenameCaretAttrib, iTreeSetRenameCaretAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAMESELECTION", iTreeGetRenameSelectionAttrib, iTreeSetRenameSelectionAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  /* Falta SCROLLBAR (YES), FONT (NULL), SHOWDRAGDROP (NO), SHOWRENAME (NO) ??? */

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttribute(ic, "VALUE",  iTreeGetValueAttrib, iTreeSetValueAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKED", (IattribGetFunc)iTreeGetMarkedAttrib, (IattribSetFunc)iTreeSetMarkedAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CTRL",   iTreeGetCtrlAttrib, iTreeSetCtrlAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHIFT",  iTreeGetShiftAttrib, iTreeSetShiftAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", iTreeGetStartingAttrib, iTreeSetStartingAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT); /* root node */

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttribute(ic, "IMAGE", NULL,  (IattribSetFunc)iTreeSetImageAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEEXPANDED", NULL, (IattribSetFunc)iTreeSetImageExpandedAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGELEAF",     NULL, iTreeSetImageLeafAttrib, "IMGLEAF", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, iTreeSetImageBranchCollapsedAttrib, "IMGCOLLAPSED", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED",  NULL, iTreeSetImageBranchExpandedAttrib,  "IMGEXPANDED",  IUP_NOT_MAPPED, IUP_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttribute(ic, "NAME",   (IattribGetFunc)iTreeGetNameAttrib,   (IattribSetFunc)iTreeSetNameAttrib,  NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATE",  (IattribGetFunc)iTreeGetStateAttrib,  (IattribSetFunc)iTreeSetStateAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DEPTH",  (IattribGetFunc)iTreeGetDepthAttrib,  (IattribSetFunc)iTreeSetDepthAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COLOR",  (IattribGetFunc)iTreeGetColorAttrib,  (IattribSetFunc)iTreeSetColorAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "KIND",   (IattribGetFunc)iTreeGetKindAttrib,   NULL, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PARENT", (IattribGetFunc)iTreeGetParentAttrib, NULL, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttribute(ic, "ADDLEAF",   NULL, (IattribSetFunc)iTreeSetAddLeafAttrib,   NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDBRANCH", NULL, (IattribSetFunc)iTreeSetAddBranchAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELNODE",   NULL, (IattribSetFunc)iTreeSetDelNodeAttrib,   NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDRAW",    NULL, (IattribSetFunc)iTreeSetRedrawAttrib,    NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME",    NULL, (IattribSetFunc)iTreeSetRenameAttrib,    NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "REPAINT", NULL, iTreeSetRepaintAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iTreeSetActiveAttrib, "YES", IUP_MAPPED, IUP_INHERIT);

  if (!IupGetHandle("IupTreeDragCursor"))
  {
    iTreeCreateCursor();
    iTreeImageInitializeImages();
  }

  return ic;
}


/*****************************************************************************************************/


void IupTreeSetAttribute(Ihandle* ih, const char* a, int id, char* v)
{
  char* attr = iupStrGetMemory(50);
  sprintf(attr, "%s%d", a, id);
  IupSetAttribute(ih, attr, v);
}

void IupTreeStoreAttribute(Ihandle* ih, const char* a, int id, char* v)
{
  char* attr = iupStrGetMemory(50);
  sprintf(attr, "%s%d", a, id);
  IupStoreAttribute(ih, attr, v);
}

char* IupTreeGetAttribute(Ihandle* ih, const char* a, int id)
{
  char* attr = iupStrGetMemory(50);
  sprintf(attr, "%s%d", a, id);
  return IupGetAttribute(ih, attr);
}

int IupTreeGetInt(Ihandle* ih, const char* a, int id)
{
  char* attr = iupStrGetMemory(50);
  sprintf(attr, "%s%d", a, id);
  return IupGetInt(ih, attr);
}

float IupTreeGetFloat(Ihandle* ih, const char* a, int id)
{
  char* attr = iupStrGetMemory(50);
  sprintf(attr, "%s%d", a, id);
  return IupGetFloat(ih, attr);
}

void IupTreeSetfAttribute(Ihandle* ih, const char* a, int id, char* f, ...)
{
  static char v[SHRT_MAX];
  char* attr = iupStrGetMemory(50);
  va_list arglist;
  sprintf(attr, "%s%d", a, id);
  va_start(arglist, f);
  vsprintf(v, f, arglist);
  va_end(arglist);
  IupStoreAttribute(ih, attr, v);
}
