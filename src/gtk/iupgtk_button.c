/** \file
 * \brief Button Control
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
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupgtk_drv.h"


#if !GTK_CHECK_VERSION(2, 6, 0)
static void gtk_button_set_image(GtkButton *button, GtkWidget *image)
{
}
static GtkWidget* gtk_button_get_image(GtkButton *button)
{
  return NULL;
}
#endif

void iupdrvButtonAddBorders(int *x, int *y)
{
#ifdef WIN32
  int border_size = 2*5;
#else
  int border_size = 2*5+1; /* borders are not symetric */
#endif
  (*x) += border_size;
  (*y) += border_size;
}

static void gtk_button_children_callback(GtkWidget *widget, gpointer client_data)
{
  if (GTK_IS_LABEL(widget))
  {
    GtkLabel **label = (GtkLabel**) client_data;
    *label = (GtkLabel*)widget;
  }
}

static GtkLabel* gtkButtonGetLabel(Ihandle* ih)
{
  if (ih->data->type == IUP_BUTTON_TEXT)  /* text only */
    return (GtkLabel*)gtk_bin_get_child((GtkBin*)ih->handle);
  else if (ih->data->type == IUP_BUTTON_BOTH) /* both */
  {
    /* when both is set, button contains an GtkAlignment, 
       that contains a GtkBox, that contains a label and an image */
    GtkContainer *container = (GtkContainer*)gtk_bin_get_child((GtkBin*)gtk_bin_get_child((GtkBin*)ih->handle));
    GtkLabel* label = NULL;
    gtk_container_foreach(container, gtk_button_children_callback, &label);
    return label;
  }
  return NULL;
}

static int gtkButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_BUTTON_IMAGE)  /* text or both */
  {
    GtkLabel* label = gtkButtonGetLabel(ih);
    iupgtkSetMnemonicTitle(ih, label, value);
    return 1;
  }

  return 0;
}

static int gtkButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  GtkButton* button = (GtkButton*)ih->handle;
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

  gtk_button_set_alignment(button, xalign, yalign);

  if (ih->data->type == IUP_BUTTON_TEXT)   /* text only */
  {
    PangoLayout* layout = gtk_label_get_layout(gtkButtonGetLabel(ih));
    if (layout) pango_layout_set_alignment(layout, alignment);
  }

  return 1;
}

static int gtkButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    if (ih->data->type == IUP_BUTTON_TEXT)   /* text only */
    {
      GtkMisc* misc = (GtkMisc*)gtk_bin_get_child((GtkBin*)ih->handle);
      gtk_misc_set_padding(misc, ih->data->horiz_padding, ih->data->vert_padding);
    }
    else
    {
      GtkAlignment* alignment = (GtkAlignment*)gtk_bin_get_child((GtkBin*)ih->handle);
      gtk_alignment_set_padding(alignment, ih->data->vert_padding, ih->data->vert_padding, 
                                           ih->data->horiz_padding, ih->data->horiz_padding);
    }
  }
  return 0;
}

static int gtkButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkLabel* label = gtkButtonGetLabel(ih);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkBaseSetFgColor((GtkWidget*)label, r, g, b);

  return 1;
}

static int gtkButtonSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetStandardFontAttrib(ih, value);

  if (ih->handle)
  {
    GtkLabel* label = gtkButtonGetLabel(ih);
    if (!label) return 1;

    gtk_widget_modify_font((GtkWidget*)label, (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih));
    iupgtkFontUpdatePangoLayout(ih, gtk_label_get_layout(label));
  }
  return 1;
}

static void gtkButtonSetPixbuf(Ihandle* ih, const char* name, int make_inactive, const char* attrib_name)
{
  GtkButton* button = (GtkButton*)ih->handle;
  GtkImage* image = (GtkImage*)gtk_button_get_image(button);

  if (name && image)
  {
    GdkPixbuf* pixbuf = iupImageGetImage(name, ih, make_inactive, attrib_name);
    GdkPixbuf* old_pixbuf = gtk_image_get_pixbuf(image);
    if (pixbuf != old_pixbuf)
      gtk_image_set_from_pixbuf(image, pixbuf);
    return;
  }

  /* if not defined */
#if GTK_CHECK_VERSION(2, 8, 0)
  gtk_image_clear(image);
#endif
}

static int gtkButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_BUTTON_TEXT)   /* image or both */
  {
    if (iupdrvIsActive(ih))
      gtkButtonSetPixbuf(ih, value, 0, "IMAGE");
    else
    {
      if (!iupAttribGetStr(ih, "IMINACTIVE"))
      {
        /* if not active and IMINACTIVE is not defined 
           then automaticaly create one based on IMAGE */
        gtkButtonSetPixbuf(ih, value, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_BUTTON_TEXT)   /* image or both */
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtkButtonSetPixbuf(ih, value, 0, "IMINACTIVE");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        char* name = iupAttribGetStr(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type != IUP_BUTTON_TEXT)     /* image or both */
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGetStr(ih, "IMINACTIVE");
      if (name)
        gtkButtonSetPixbuf(ih, name, 0, "IMINACTIVE");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        name = iupAttribGetStr(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 1, "IMINACTIVE"); /* make_inactive */
      }
    }
    else
    {
      /* must restore the normal image */
      char* name = iupAttribGetStr(ih, "IMAGE");
      gtkButtonSetPixbuf(ih, name, 0, "IMAGE");
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int gtkButtonSetFocusOnClickAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    gtk_button_set_focus_on_click((GtkButton*)ih->handle, TRUE);
  else
    gtk_button_set_focus_on_click((GtkButton*)ih->handle, FALSE);
  return 1;
}

static gboolean gtkButtonEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle *ih)
{
  iupgtkEnterLeaveEvent(widget, evt, ih);
  (void)widget;

  if (evt->type == GDK_ENTER_NOTIFY)
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);
  else  if (evt->type == GDK_LEAVE_NOTIFY)               
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);

  return FALSE;
}

static gboolean gtkButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  if (ih->data->type != IUP_BUTTON_TEXT)   /* image or both */
  {
    char* name = iupAttribGetStr(ih, "IMPRESS");
    if (name)
    {
      if (evt->type == GDK_BUTTON_PRESS)
        gtkButtonSetPixbuf(ih, name, 0, "IMPRESS");
      else
      {
        name = iupAttribGetStr(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 0, "IMAGE");
      }
    }
  }
    
  return iupgtkButtonEvent(widget, evt, ih);
}

static void gtkButtonClicked(GtkButton *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE) 
      IupExitLoop();
  }
  (void)widget;
}

static int gtkButtonMapMethod(Ihandle* ih)
{
  int impress;
  char* value;

  ih->handle = gtk_button_new();
  if (!ih->handle)
    return IUP_ERROR;

  value = iupAttribGetStr(ih, "IMAGE");
  if (value)
  {
    gtk_button_set_image((GtkButton*)ih->handle, gtk_image_new());
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGetStr(ih, "TITLE");
    if (value)
    {
      GtkSettings* settings = gtk_widget_get_settings(ih->handle);
      g_object_set(settings, "gtk-button-images", (int)TRUE, NULL);

      gtk_button_set_label((GtkButton*)ih->handle, value);
      ih->data->type |= IUP_BUTTON_TEXT;
      
#if GTK_CHECK_VERSION(2, 10, 0)
      gtk_button_set_image_position((GtkButton*)ih->handle, ih->data->img_position);  /* IUP and GTK have the same Ids */
#endif
    }
  }
  else
  {
    char* title = iupAttribGetStr(ih, "TITLE");
    if (!title) title="";
    gtk_button_set_label((GtkButton*)ih->handle, title);
    ih->data->type = IUP_BUTTON_TEXT;
  }

  /* add to the parent, all GTK controls must call this. */
  iupgtkBaseAddToParent(ih);

  if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
    GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS;

  value = iupAttribGetStr(ih, "IMPRESS");
  impress = (ih->data->type & IUP_BUTTON_IMAGE && value)? 1: 0;
  if (!impress && iupAttribGetInt(ih, "FLAT"))
  {
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);

    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(gtkButtonEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(gtkButtonEnterLeaveEvent), ih);
  }
  else
  {
    if (impress && !iupAttribGetStr(ih, "IMPRESSBORDER"))
      gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);
    else
      gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);

    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  g_signal_connect(G_OBJECT(ih->handle), "clicked", G_CALLBACK(gtkButtonClicked), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkButtonEvent), ih);

  gtk_widget_realize(ih->handle);

  /* ensure the default values, that are different from the native ones */
  gtkButtonSetAlignmentAttrib(ih, iupAttribGetStrDefault(ih, "ALIGNMENT"));

  return IUP_NOERROR;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkButtonMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkButtonSetStandardFontAttrib, "DEFAULTFONT", IUP_NOT_MAPPED, IUP_INHERIT);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkButtonSetActiveAttrib, "YES", IUP_MAPPED, IUP_INHERIT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkButtonSetFgColorAttrib, "DLGFGCOLOR", IUP_MAPPED, IUP_INHERIT);  /* black */
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkButtonSetTitleAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkButtonSetAlignmentAttrib, "ACENTER:ACENTER", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkButtonSetImageAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkButtonSetImInactiveAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSONCLICK", NULL, gtkButtonSetFocusOnClickAttrib, "YES", IUP_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, gtkButtonSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, IUP_MAPPED, IUP_INHERIT);
}
