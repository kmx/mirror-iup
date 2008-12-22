/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/SpinB.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


#ifndef XmIsSpinBox
#define XmIsSpinBox(w)	XtIsSubclass(w, xmSpinBoxWidgetClass)
#endif

#ifndef XmNwrap
#define XmNwrap "Nwrap"
#endif

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag)
{
  (void)ih;
  (void)formattag;
}

void iupdrvTextAddSpin(int *w, int h)
{
  *w += h/2;
}

void iupdrvTextAddBorders(int *w, int *h)
{
  int border_size = 2*5;
  (*w) += border_size;
  (*h) += border_size;
}

static void motTextGetLinColFromPosition(const char *str, int pos, int *lin, int *col )
{
  int i;

  *lin = 0;
  *col = 0;

  for (i=0; i<pos; i++)
  {
    if (*str == '\n')
    {
      (*lin)++;
      *col = 0;
    }
    else
      (*col)++;

    str++;
  }

  (*lin)++; /* IUP starts at 1 */
  (*col)++;
}

static int motTextSetLinColToPosition(const char *str, int lin, int col)
{
  int pos=0, cur_lin=0, cur_col=0;

  lin--; /* IUP starts at 1 */
  col--;

  while (*str)
  {
    if (lin<=cur_lin && col<=cur_col)
      break;

    if (*str == '\n')
    {
      cur_lin++;
      cur_col = 0;
    }
    else
      cur_col++;

    str++;
    pos++;
  }

  return pos;
}

void iupdrvTextConvertXYToChar(Ihandle* ih, int x, int y, int *lin, int *col, int *pos)
{
  *pos = XmTextXYToPos(ih->handle, x, y);
  if (ih->data->is_multiline)
  {
    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, *pos, lin, col);
    XtFree(value);
  }
  else
  {
    *col = (*pos) + 1; /* IUP starts at 1 */
    *lin = 1;
  }
}


/*******************************************************************************************/


static int motTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
  }
  return 0;
}

static int motTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  XtVaSetValues(ih->handle, XmNeditable, iupStrBoolean(value)? False: True, NULL);
  return 0;
}

static char* motTextGetReadOnlyAttrib(Ihandle* ih)
{
  Boolean editable;
  XtVaGetValues(ih->handle, XmNeditable, &editable, NULL);
  if (editable)
    return "YES";
  else
    return "NO";
}

static int motTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  /* disable callbacks */
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
  XmTextRemove(ih->handle);
  XmTextInsert(ih->handle, XmTextGetInsertionPosition(ih->handle), (char*)value);
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);

  return 0;
}

static int motTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  XmTextPosition start, end;

  if (!value)
    return 0;

  if (XmTextGetSelectionPosition(ih->handle, &start, &end) && start!=end)
  {
    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
    XmTextReplace(ih->handle, start, end, (char*)value);
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }

  return 0;
}

static char* motTextGetSelectedTextAttrib(Ihandle* ih)
{
  char* selectedtext = XmTextGetSelection(ih->handle);
  char* str = iupStrGetMemoryCopy(selectedtext);
  XtFree(selectedtext);
  return str;
}

static int motTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  XmTextPosition pos = XmTextGetLastPosition(ih->handle);
  /* disable callbacks */
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
  if (ih->data->is_multiline && ih->data->append_newline)
    XmTextInsert(ih->handle, pos, "\n");
	if (value)
    XmTextInsert(ih->handle, pos+1, (char*)value);
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  return 0;
}

static int motTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XmTextClearSelection(ih->handle, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XmTextSetSelection(ih->handle, (XmTextPosition)0, (XmTextPosition)XmTextGetLastPosition(ih->handle), CurrentTime);
    return 0;
  }

  if (ih->data->is_multiline)
  {
    int lin_start=1, col_start=1, lin_end=1, col_end=1;
    char *str;

    if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end)!=4) return 0;
    if (lin_start<1 || col_start<1 || lin_end<1 || col_end<1) return 0;

    str = XmTextGetString(ih->handle);
    start = motTextSetLinColToPosition(str, lin_start, col_start);
    end = motTextSetLinColToPosition(str, lin_end, col_end);
    XtFree(str);
  }
  else
  {
    if (iupStrToIntInt(value, &start, &end, ':')!=2) 
      return 0;

    if(start<1 || end<1) 
      return 0;

    start--; /* IUP starts at 1 */
    end--;
  }

  /* end is inside the selection, in IUP is outside */
  end--;

  XmTextSetSelection(ih->handle, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motTextGetSelectionAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;
  char* str;

  if (!XmTextGetSelectionPosition(ih->handle, &start, &end) || start==end)
    return NULL;

  str = iupStrGetMemory(100);

  /* end is inside the selection, in IUP is outside */
  end++;

  if (ih->data->is_multiline)
  {
    int start_col, start_lin, end_col, end_lin;

    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, start, &start_lin, &start_col);
    motTextGetLinColFromPosition(value, end,   &end_lin,   &end_col);
    XtFree(value);

    sprintf(str,"%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
  }
  else
  {
    start++; /* IUP starts at 1 */
    end++;
    sprintf(str, "%d:%d", (int)start, (int)end);
  }

  return str;
}

static int motTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    XmTextClearSelection(ih->handle, CurrentTime);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    XmTextSetSelection(ih->handle, (XmTextPosition)0, (XmTextPosition)XmTextGetLastPosition(ih->handle), CurrentTime);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  /* end is inside the selection, in IUP is outside */
  end--;

  XmTextSetSelection(ih->handle, (XmTextPosition)start, (XmTextPosition)end, CurrentTime);

  return 0;
}

static char* motTextGetSelectionPosAttrib(Ihandle* ih)
{
  XmTextPosition start = 0, end = 0;
  char* str;

  if (!XmTextGetSelectionPosition(ih->handle, &start, &end) || start==end)
    return NULL;

  str = iupStrGetMemory(100);

  /* end is inside the selection, in IUP is outside */
  end++;

  sprintf(str, "%d:%d", (int)start, (int)end);

  return str;
}

static int motTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    char *str;

    iupStrToIntInt(value, &lin, &col, ',');

    str = XmTextGetString(ih->handle);
    pos = motTextSetLinColToPosition(str, lin, col);
    XtFree(str);
  }
  else
  {
    sscanf(value,"%i",&pos);
    pos--; /* IUP starts at 1 */
  }

  XmTextSetInsertionPosition(ih->handle, (XmTextPosition)pos);
  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static char* motTextGetCaretAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(50);

  XmTextPosition pos = XmTextGetInsertionPosition(ih->handle);

  if (ih->data->is_multiline)
  {
    int col, lin;

    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, pos, &lin, &col);
    XtFree(value);

    sprintf(str, "%d,%d", lin, col);
  }
  else
  {
    pos++; /* IUP starts at 1 */
    sprintf(str, "%d", (int)pos);
  }

  return str;
}

static int motTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  if (pos < 0) pos = 0;

  XmTextSetInsertionPosition(ih->handle, (XmTextPosition)pos);
  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static char* motTextGetCaretPosAttrib(Ihandle* ih)
{
  XmTextPosition pos = XmTextGetInsertionPosition(ih->handle);
  char* str = iupStrGetMemory(50);
  sprintf(str, "%d", (int)pos);
  return str;
}

static int motTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1, pos;
    char* str;

    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    str = XmTextGetString(ih->handle);
    pos = motTextSetLinColToPosition(str, lin, col);
    XtFree(str);
  }
  else
  {
    sscanf(value,"%i",&pos);
    if (pos < 1) pos = 1;
    pos--;  /* return to Motif referece */
  }

  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static int motTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  if (pos < 0) pos = 0;

  XmTextShowPosition(ih->handle, (XmTextPosition)pos);

  return 0;
}

static int motTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;
  if (ih->handle)
    XtVaSetValues(ih->handle, XmNmaxLength, ih->data->nc, NULL);
  return 0;
}

static int motTextSetClipboardAttrib(Ihandle *ih, const char *value)
{
  if (iupStrEqualNoCase(value, "COPY"))
  {
    char *str = XmTextGetSelection(ih->handle);
    if (!str) return 0;

    XmTextCopy(ih->handle, CurrentTime);

    /* do it also for the X clipboard */
    XStoreBytes(iupmot_display, str, strlen(str)+1);
    XtFree(str);
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    char *str = XmTextGetSelection(ih->handle);
    if (!str) return 0;

    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");

    XmTextCut(ih->handle, CurrentTime);

    /* do it also for the X clipboard */
    XStoreBytes(iupmot_display, str, strlen(str)+1);
    XtFree(str);
    XmTextRemove(ih->handle);

    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    int size;
    char* str = XFetchBytes(iupmot_display, &size);
    if (!str) return 0;

    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");

    XmTextPaste(ih->handle); /* TODO: this could force 2 pastes, check in CDE */

    /* do it also for the X clipboard */
    XmTextRemove(ih->handle);
    XmTextInsert(ih->handle, XmTextGetInsertionPosition(ih->handle), str);
    XFree(str);

    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
    XmTextRemove(ih->handle);
    iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}

static int motTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Widget extraparent = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (extraparent)
  {
    Pixel color;

    /* ignore given value for the scrollbars, must use only from parent */
    char* parent_value = iupAttribGetStrNativeParent(ih, "BGCOLOR");
    if (!parent_value) parent_value = IupGetGlobal("DLGBGCOLOR");

    color = iupmotColorGetPixelStr(parent_value);
    if (color != (Pixel)-1)
    {
      Widget sb = NULL;

      iupmotSetBgColor(extraparent, color);

      XtVaGetValues(extraparent, XmNverticalScrollBar, &sb, NULL);
      if (sb) iupmotSetBgColor(sb, color);

      XtVaGetValues(extraparent, XmNhorizontalScrollBar, &sb, NULL);
      if (sb) iupmotSetBgColor(sb, color);
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);   /* use given value for contents */
}

static int motTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int min;
    if (iupStrToInt(value, &min))
    {
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
      XtVaSetValues(ih->handle, XmNminimumValue, min, NULL);
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
    }
  }
  return 1;
}

static int motTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
      XtVaSetValues(ih->handle, XmNmaximumValue, max, NULL);
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
    }
  }
  return 1;
}

static int motTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int inc;
    if (iupStrToInt(value, &inc))
    {
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
      XtVaSetValues(ih->handle, XmNincrementValue, inc, NULL);
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
    }
  }
  return 1;
}

static int motTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int pos;
    if (iupStrToInt(value, &pos))
    {
      char* value = NULL;
      int min, max;
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
      XtVaGetValues(ih->handle, XmNminimumValue, &min, 
                                XmNmaximumValue, &max, NULL);
      if (pos < min) pos = min;
      if (pos > max) pos = max;
      if (iupAttribGetStr(ih, "_IUPMOT_SPIN_NOAUTO"))
        value = XmTextGetString(ih->handle);

      XtVaSetValues(ih->handle, XmNposition, pos, NULL);

      if (value)
      {
        XmTextSetString(ih->handle, (char*)value);
        XtFree(value);
      }
      iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
      return 1;
    }
  }
  return 0;
}

static char* motTextGetSpinValueAttrib(Ihandle* ih)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    int pos;
    char *str = iupStrGetMemory(50);
    XtVaGetValues(ih->handle, XmNposition, &pos, NULL);
    sprintf(str, "%d", pos);
    return str;
  }
  return NULL;
}

static int motTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  /* disable callbacks */
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
  motTextSetSpinValueAttrib(ih, value);
  XmTextSetString(ih->handle, (char*)value);
  iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
  return 0;
}

static char* motTextGetValueAttrib(Ihandle* ih)
{
  char* value = XmTextGetString(ih->handle);
  char* str = iupStrGetMemoryCopy(value);
  XtFree(value);
  return str;
}
                       

/******************************************************************************/


static void motTextSpinModifyVerifyCallback(Widget w, Ihandle* ih, XmSpinBoxCallbackStruct *cbs)
{
  IFni cb = (IFni) IupGetCallback(ih, "SPIN_CB");
  if (cb) cb(ih, cbs->position);
  (void)w;

  iupAttribSetStr(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB", "1");
}

static void motTextModifyVerifyCallback(Widget w, Ihandle *ih, XmTextVerifyPtr text)
{
  int start, end, key = 0;
  char *value, *new_value, *insert_value;
  KeySym motcode = 0;
  IFnis cb;

  if (iupAttribGetStr(ih, "_IUPMOT_DISABLE_TEXT_CB"))
    return;

  if (iupAttribGetStr(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB"))
  {
    if (iupAttribGetStr(ih, "_IUPMOT_SPIN_NOAUTO"))
      text->doit = False;

    iupAttribSetStr(ih, "_IUPMOT_SPIN_DISABLE_TEXT_CB", NULL);
    return;
  }

  cb = (IFnis)IupGetCallback(ih, "ACTION");
  if (!cb && !ih->data->mask)
    return;

  if (text->event && text->event->type == KeyPress)
  {
    unsigned int state = ((XKeyEvent*)text->event)->state;
    if (state & ControlMask ||  /* Ctrl */
        state & Mod1Mask || 
        state & Mod5Mask ||  /* Alt */
        state & Mod4Mask) /* Apple/Win */
      return;

    motcode = XKeycodeToKeysym(iupmot_display, ((XKeyEvent*)text->event)->keycode, 0);
  }

  value = XmTextGetString(ih->handle);
  start = text->startPos;
  end = text->endPos;
  insert_value = text->text->ptr;

  if (motcode == XK_Delete)
  {
    new_value = value;
    iupStrRemove(value, start, end, 1);
  }
  else if (motcode == XK_BackSpace)
  {
    new_value = value;
    iupStrRemove(value, start, end, -1);
  }
  else
  {
    if (!value)
      new_value = iupStrDup(insert_value);
    else if (insert_value)
      new_value = iupStrInsert(value, insert_value, start, end);
    else
      new_value = value;
  }

  if (insert_value && insert_value[0]!=0 && insert_value[1]==0)
    key = insert_value[0];

  if (ih->data->mask && iupMaskCheck(ih->data->mask, new_value)==0)
  {
    if (new_value != value) free(new_value);
    XtFree(value);
    text->doit = False;     /* abort processing */
    return;
  }

  if (cb)
  {
    int cb_ret = cb(ih, key, (char*)new_value);
    if (cb_ret==IUP_IGNORE)
      text->doit = False;     /* abort processing */
    else if (cb_ret==IUP_CLOSE)
    {
      IupExitLoop();
      text->doit = False;     /* abort processing */
    }
    else if (cb_ret!=0 && key!=0 && 
             cb_ret != IUP_DEFAULT && cb_ret != IUP_CONTINUE)  
    {
      insert_value[0] = (char)cb_ret;  /* replace key */
    }
  }

  if (text->doit)
  {
    /* Spin is not automatically updated when you directly edit the text */
    Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
    if (spinbox && XmIsSpinBox(spinbox) && !iupAttribGetStr(ih, "_IUPMOT_SPIN_NOAUTO"))
    {
      int pos;
      if (iupStrToInt(new_value, &pos))
      {
        XmTextPosition caret_pos = text->currInsert;
        iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", "1");
        XtVaSetValues(ih->handle, XmNposition, pos, NULL);
        iupAttribSetStr(ih, "_IUPMOT_DISABLE_TEXT_CB", NULL);
        /* do not handle all situations, but handle the basic ones */
        if (text->startPos == text->endPos) /* insert */
          caret_pos++;
        else if (text->startPos < text->endPos && text->startPos < text->currInsert)  /* backspace */
          caret_pos--;
        XmTextSetInsertionPosition(ih->handle, caret_pos);
        text->doit = False;
      }
    }
  }

  if (new_value != value) free(new_value);
  XtFree(value);
  (void)w;
}

static void motTextMotionVerifyCallback(Widget w, Ihandle* ih, XmTextVerifyCallbackStruct* textverify)
{
  int col, lin, pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = textverify->newInsert;

  if (ih->data->is_multiline)
  {
    char *value = XmTextGetString(ih->handle);
    motTextGetLinColFromPosition(value, pos, &lin, &col);
    XtFree(value);
  }
  else
  {
    col = pos;
    col++; /* IUP starts at 1 */
    lin = 1;
  }

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, lin, col, pos);
  }

  (void)w;
}

static void motTextKeyPressEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  Widget spinbox;

  if (evt->state & ControlMask)   /* Ctrl */
  {
    KeySym motcode = XKeycodeToKeysym(iupmot_display, evt->keycode, 0);
    if (motcode == XK_c)
      motTextSetClipboardAttrib(ih, "COPY");
    else if (motcode == XK_x)
      motTextSetClipboardAttrib(ih, "CUT");
    else if (motcode == XK_v)
      motTextSetClipboardAttrib(ih, "PASTE");
    else if (motcode == XK_a)
      XmTextSetSelection(ih->handle, 0, XmTextGetLastPosition(ih->handle), CurrentTime);
  }

  spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    KeySym motcode = XKeycodeToKeysym(iupmot_display, evt->keycode, 0);
    if (motcode == XK_Left || motcode == XK_Right)
    {
      /* avoid spin increment using Left/Right arrows, 
         but must manually handle the new cursor position */
      XmTextPosition caret_pos = XmTextGetInsertionPosition(ih->handle);
      XmTextSetInsertionPosition(ih->handle, (motcode == XK_Left)? caret_pos-1: caret_pos+1);
      *cont = False; 
    }
  }

  iupmotKeyPressEvent(w, ih, (XEvent*)evt, cont);
}


/******************************************************************************/


static void motTextLayoutUpdateMethod(Ihandle* ih)
{
  Widget spinbox = (Widget)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (spinbox && XmIsSpinBox(spinbox))
  {
    XtVaSetValues(ih->handle,
      XmNwidth, (XtArgVal)ih->currentwidth-ih->currentheight/2,
      XmNheight, (XtArgVal)ih->currentheight,
      NULL);

    XtVaSetValues(spinbox,
      XmNx, (XtArgVal)ih->x,
      XmNy, (XtArgVal)ih->y,
      XmNwidth, (XtArgVal)ih->currentwidth,
      XmNheight, (XtArgVal)ih->currentheight,
      XmNarrowSize, ih->currentheight/2,
      NULL);
  }
  else
    iupdrvBaseLayoutUpdateMethod(ih);
}

static int motTextMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];
  Widget parent = iupChildTreeGetNativeParentHandle(ih);
  char* child_id = iupDialogGetChildIdStr(ih);
  int spin = 0;
  WidgetClass widget_class = xmTextWidgetClass;

  if (ih->data->is_multiline)
  {
    Widget sb_win;
    int wordwrap = 0;

    if (iupStrBoolean(iupAttribGetStr(ih, "WORDWRAP")))
    {
      wordwrap = 1;
      ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scroolbar */
    }

    /******************************/
    /* Create the scrolled window */
    /******************************/

    iupmotSetArg(args[num_args++], XmNmappedWhenManaged, False);  /* not visible when managed */
    iupmotSetArg(args[num_args++], XmNscrollingPolicy, XmAPPLICATION_DEFINED);
    iupmotSetArg(args[num_args++], XmNvisualPolicy, XmVARIABLE);
    iupmotSetArg(args[num_args++], XmNscrollBarDisplayPolicy, XmSTATIC);   /* can NOT be XmAS_NEEDED because XmAPPLICATION_DEFINED */
    iupmotSetArg(args[num_args++], XmNspacing, 0); /* no space between scrollbars and text */
    iupmotSetArg(args[num_args++], XmNborderWidth, 0);
    iupmotSetArg(args[num_args++], XmNshadowThickness, 0);
    
    sb_win = XtCreateManagedWidget(
      child_id,  /* child identifier */
      xmScrolledWindowWidgetClass, /* widget class */
      parent,                      /* widget parent */
      args, num_args);

    if (!sb_win)
      return IUP_ERROR;

    parent = sb_win;
    child_id = "text";

    num_args = 0;
    iupmotSetArg(args[num_args++], XmNeditMode, XmMULTI_LINE_EDIT);
    if (wordwrap)
      iupmotSetArg(args[num_args++], XmNwordWrap, True)
  }
  else
  {
    widget_class = xmTextFieldWidgetClass;

    if (iupAttribGetInt(ih, "SPIN"))
    {
      Widget spinbox;

      num_args = 0;
      iupmotSetArg(args[num_args++], XmNmappedWhenManaged, False);  /* not visible when managed */
      iupmotSetArg(args[num_args++], XmNspacing, 0); /* no space between spin and text */
      iupmotSetArg(args[num_args++], XmNborderWidth, 0);
      iupmotSetArg(args[num_args++], XmNshadowThickness, 0);
      iupmotSetArg(args[num_args++], XmNmarginHeight, 0);
      iupmotSetArg(args[num_args++], XmNmarginWidth, 0);
      iupmotSetArg(args[num_args++], XmNarrowSize, 8);

      if (iupStrEqualNoCase(iupAttribGetStr(ih, "SPINALIGN"), "LEFT"))
        iupmotSetArg(args[num_args++], XmNarrowLayout, XmARROWS_BEGINNING)
      else
        iupmotSetArg(args[num_args++], XmNarrowLayout, XmARROWS_END)

      spinbox = XtCreateManagedWidget(
        child_id,  /* child identifier */
        xmSpinBoxWidgetClass, /* widget class */
        parent,                      /* widget parent */
        args, num_args);

      if (!spinbox)
        return IUP_ERROR;

      XtAddCallback(spinbox, XmNmodifyVerifyCallback, (XtCallbackProc)motTextSpinModifyVerifyCallback, (XtPointer)ih);

      parent = spinbox;
      child_id = "text";
      spin = 1;

      if (!iupStrBoolean(iupAttribGetStrDefault(ih, "SPINAUTO")))
        iupAttribSetStr(ih, "_IUPMOT_SPIN_NOAUTO", "1");
    }

    num_args = 0;
    iupmotSetArg(args[num_args++], XmNeditMode, XmSINGLE_LINE_EDIT);

    if (spin)
    {
      /* Spin Constraints */
      iupmotSetArg(args[num_args++], XmNspinBoxChildType, XmNUMERIC);
      iupmotSetArg(args[num_args++], XmNminimumValue, 0);
      iupmotSetArg(args[num_args++], XmNmaximumValue, 100);
      iupmotSetArg(args[num_args++], XmNposition, 0);

      if (iupStrBoolean(iupAttribGetStr(ih, "SPINWRAP")))
        iupmotSetArg(args[num_args++], XmNwrap, TRUE)
      else
        iupmotSetArg(args[num_args++], XmNwrap, FALSE)
    }
    else
    {
      iupmotSetArg(args[num_args++], XmNmappedWhenManaged, False);  /* not visible when managed */
    }
  }

  iupmotSetArg(args[num_args++], XmNx, 0);  /* x-position */
  iupmotSetArg(args[num_args++], XmNy, 0);  /* y-position */
  iupmotSetArg(args[num_args++], XmNwidth, 10);  /* default width to avoid 0 */
  iupmotSetArg(args[num_args++], XmNheight, 10); /* default height to avoid 0 */

  iupmotSetArg(args[num_args++], XmNmarginHeight, 0);  /* default padding */
  iupmotSetArg(args[num_args++], XmNmarginWidth, 0);

  if (iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
    iupmotSetArg(args[num_args++], XmNtraversalOn, True)
  else
    iupmotSetArg(args[num_args++], XmNtraversalOn, False)

  iupmotSetArg(args[num_args++], XmNnavigationType, XmTAB_GROUP);
  iupmotSetArg(args[num_args++], XmNhighlightThickness, 2);
  iupmotSetArg(args[num_args++], XmNverifyBell, False);
  iupmotSetArg(args[num_args++], XmNspacing, 0);

  if (IupGetInt(ih, "BORDER"))              /* Use IupGetInt for inheritance */
    iupmotSetArg(args[num_args++], XmNshadowThickness, 2)
  else
    iupmotSetArg(args[num_args++], XmNshadowThickness, 0)

  if (ih->data->is_multiline)
  {
    if (ih->data->sb & IUP_SB_HORIZ)
      iupmotSetArg(args[num_args++], XmNscrollHorizontal, True)
    else
      iupmotSetArg(args[num_args++], XmNscrollHorizontal, False)

    if (ih->data->sb & IUP_SB_VERT)
      iupmotSetArg(args[num_args++], XmNscrollVertical, True)
    else
      iupmotSetArg(args[num_args++], XmNscrollVertical, False)
  }

  ih->handle = XtCreateManagedWidget(
    child_id,       /* child identifier */
    widget_class,   /* widget class */
    parent,         /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  if (ih->data->is_multiline)
  {
    iupAttribSetStr(ih, "_IUP_EXTRAPARENT", (char*)parent);
    XtVaSetValues(parent, XmNworkWindow, ih->handle, NULL);
  } 
  else if (spin)
    iupAttribSetStr(ih, "_IUP_EXTRAPARENT", (char*)parent);

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)motTextKeyPressEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);

  XtAddCallback(ih->handle, XmNmodifyVerifyCallback, (XtCallbackProc)motTextModifyVerifyCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNmotionVerifyCallback, (XtCallbackProc)motTextMotionVerifyCallback, (XtPointer)ih);

  /* Disable Drag Source */
  iupmotDisableDragSource(ih->handle);

  /* initialize the widget */
  if (ih->data->is_multiline || spin)
    XtRealizeWidget(parent);
  else
    XtRealizeWidget(ih->handle);

  if (IupGetGlobal("_IUP_SET_TXTCOLORS"))
  {
    iupmotSetGlobalColorAttrib(ih->handle, XmNbackground, "TXTBGCOLOR");
    iupmotSetGlobalColorAttrib(ih->handle, XmNforeground, "TXTFGCOLOR");
    IupSetGlobal("_IUP_SET_TXTCOLORS", NULL);
  }

  return IUP_NOERROR;
}

void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motTextMapMethod;
  ic->LayoutUpdate = motTextLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motTextSetBgColorAttrib, "TXTBGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, "TXTFGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, motTextSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", motTextGetValueAttrib, motTextSetValueAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", motTextGetSelectedTextAttrib, motTextSetSelectedTextAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", motTextGetSelectionAttrib, motTextSetSelectionAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", motTextGetSelectionPosAttrib, motTextSetSelectionPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", motTextGetCaretAttrib, motTextSetCaretAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", motTextGetCaretPosAttrib, motTextSetCaretPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, motTextSetInsertAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, motTextSetAppendAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", motTextGetReadOnlyAttrib, motTextSetReadOnlyAttrib, NULL, IUP_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, motTextSetNCAttrib, NULL, IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, motTextSetClipboardAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", NULL, iupBaseNoSetAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);  /* formatting not supported in Motif */
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, motTextSetScrollToAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, motTextSetScrollToPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, motTextSetSpinMinAttrib, "0", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, motTextSetSpinMaxAttrib, "100", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, motTextSetSpinIncAttrib, "1", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", motTextGetSpinValueAttrib, motTextSetSpinValueAttrib, "0", IUP_MAPPED, IUP_NO_INHERIT);
}
