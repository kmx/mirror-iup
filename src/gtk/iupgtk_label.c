/** \file
 * \brief Label Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_focus.h"

#include "iupgtk_drv.h"


static int gtkLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupgtkSetMnemonicTitle(ih, label, value))
    {
      Ihandle* next = iupFocusNextInteractive(ih);
      if (next)
      {
        if (next->handle)
          gtk_label_set_mnemonic_widget(label, next->handle);
        else
          iupAttribSetStr(next, "_IUPGTK_LABELMNEMONIC", (char*)label);  /* used by iupgtkUpdateMnemonic */
      }
    }
    return 1;
  }

  return 0;
}

static int gtkLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupStrBoolean(value))
      gtk_label_set_line_wrap(label, TRUE);
    else
      gtk_label_set_line_wrap(label, FALSE);
    return 1;
  }
  return 0;
}

static int gtkLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupStrBoolean(value))
      gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
    else
      gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_NONE);
    return 1;
  }
  return 0;
}

static int gtkLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    GtkMisc* misc = (GtkMisc*)ih->handle;
    PangoAlignment alignment;
    float xalign, yalign;
    char value1[30]="", value2[30]="";

    iupStrToStrStr(value, value1, value2, ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
    {
      xalign = 1.0f;
      alignment = PANGO_ALIGN_RIGHT;
    }
    else if (iupStrEqualNoCase(value1, "ACENTER"))
    {
      xalign = 0.5f;
      alignment = PANGO_ALIGN_CENTER;
    }
    else /* "ALEFT" */
    {
      xalign = 0;
      alignment = PANGO_ALIGN_LEFT;
    }

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      yalign = 1.0f;
    else if (iupStrEqualNoCase(value2, "ATOP"))
      yalign = 0;
    else  /* ACENTER (default) */
      yalign = 0.5f;

    gtk_misc_set_alignment(misc, xalign, yalign);

    if (ih->data->type == IUP_LABEL_TEXT)
      pango_layout_set_alignment(gtk_label_get_layout((GtkLabel*)ih->handle), alignment);

    return 1;
  }
  else
    return 0;
}

static int gtkLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    GtkMisc* misc = (GtkMisc*)ih->handle;
    gtk_misc_set_padding(misc, ih->data->horiz_padding, ih->data->vert_padding);
  }
  return 0;
}

static char* gtkLabelGetPangoLayoutAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
    return (char*)gtk_label_get_layout((GtkLabel*)ih->handle);
  else
    return NULL;
}

static void gtkLabelSetPixbuf(Ihandle* ih, const char* name, int make_inactive, const char* attrib_name)
{
  GtkImage* image_label = (GtkImage*)ih->handle;

  if (name)
  {
    GdkPixbuf* pixbuf = iupImageGetImage(name, ih, make_inactive, attrib_name);
    GdkPixbuf* old_pixbuf = gtk_image_get_pixbuf(image_label);
    if (pixbuf != old_pixbuf)
      gtk_image_set_from_pixbuf(image_label, pixbuf);
    return;
  }

  /* if not defined */
  gtk_image_clear(image_label);
}

static int gtkLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupdrvIsActive(ih))
      gtkLabelSetPixbuf(ih, value, 0, "IMAGE");
    else
    {
      if (!iupAttribGetStr(ih, "IMINACTIVE"))
      {
        /* if not active and IMINACTIVE is not defined 
           then automaticaly create one based on IMAGE */
        gtkLabelSetPixbuf(ih, value, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtkLabelSetPixbuf(ih, value, 0, "IMINACTIVE");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        char* name = iupAttribGetStr(ih, "IMAGE");
        gtkLabelSetPixbuf(ih, name, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGetStr(ih, "IMINACTIVE");
      if (name)
        gtkLabelSetPixbuf(ih, name, 0, "IMINACTIVE");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        name = iupAttribGetStr(ih, "IMAGE");
        gtkLabelSetPixbuf(ih, name, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    else
    {
      /* must restore the normal image */
      char* name = iupAttribGetStr(ih, "IMAGE");
      gtkLabelSetPixbuf(ih, name, 0, "IMAGE");
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int gtkLabelMapMethod(Ihandle* ih)
{
  char* value;
  GtkWidget *label;

  value = iupAttribGetStr(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      label = gtk_hseparator_new();
    }
    else /* "VERTICAL" */
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      label = gtk_vseparator_new();
    }
  }
  else
  {
    value = iupAttribGetStr(ih, "IMAGE");
    if (value)
    {
      ih->data->type = IUP_LABEL_IMAGE;
      label = gtk_image_new();
    }
    else
    {
      ih->data->type = IUP_LABEL_TEXT;
      label = gtk_label_new(NULL);
    }
  }

  if (!label)
    return IUP_ERROR;

  ih->handle = label;

  /* add to the parent, all GTK controls must call this. */
  iupgtkBaseAddToParent(ih);

  gtk_widget_realize(label);

  /* ensure the default values, that are different from the native ones */
  gtkLabelSetAlignmentAttrib(ih, iupAttribGetStrDefault(ih, "ALIGNMENT"));

  return IUP_NOERROR;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkLabelMapMethod;

  /* Driver Dependent Attribute functions */

  /* Common GTK only (when text is in a secondary element) */
  iupClassRegisterAttribute(ic, "PANGOLAYOUT", gtkLabelGetPangoLayoutAttrib, NULL, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkLabelSetActiveAttrib, "YES", IUP_MAPPED, IUP_INHERIT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, "DLGFGCOLOR", IUP_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkLabelSetTitleAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* IupLabel only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkLabelSetAlignmentAttrib, "ALEFT:ACENTER", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkLabelSetImageAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkLabelSetImInactiveAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, gtkLabelSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, gtkLabelSetWordWrapAttrib, NULL, IUP_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, gtkLabelSetEllipsisAttrib, NULL, IUP_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, IUP_MAPPED, IUP_INHERIT);
}
