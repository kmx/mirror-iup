/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_toggle.h"

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

void iupdrvToggleAddCheckBox(int *x, int *y)
{
  (*x) += 16+4;
  if ((*y) < 16) (*y) = 16; /* minimum height */
  (*y) += 4;
}

static int gtkToggleGetCheck(Ihandle* ih)
{
  if (gtk_toggle_button_get_inconsistent((GtkToggleButton*)ih->handle))
    return -1;
  if (gtk_toggle_button_get_active((GtkToggleButton*)ih->handle))
    return 1;
  else
    return 0;
}

static void gtkToggleSetPixbuf(Ihandle* ih, const char* name, int make_inactive, const char* attrib_name)
{
  GtkButton* button = (GtkButton*)ih->handle;
  GtkImage* image = (GtkImage*)gtk_button_get_image(button);

  if (name)
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

static void gtkToggleUpdateImage(Ihandle* ih, int active, int check)
{
  char* name;

  if (!active)
  {
    name = iupAttribGetStr(ih, "IMINACTIVE");
    if (name)
      gtkToggleSetPixbuf(ih, name, 0, "IMINACTIVE");
    else
    {
      /* if not defined then automaticaly create one based on IMAGE */
      name = iupAttribGetStr(ih, "IMAGE");
      gtkToggleSetPixbuf(ih, name, 1, "IMINACTIVE"); /* make_inactive */
    }
  }
  else
  {
    /* must restore the normal image */
    if (check)
    {
      name = iupAttribGetStr(ih, "IMPRESS");
      if (name)
        gtkToggleSetPixbuf(ih, name, 0, "IMPRESS");
      else
      {
        /* if not defined then automaticaly create one based on IMAGE */
        name = iupAttribGetStr(ih, "IMAGE");
        gtkToggleSetPixbuf(ih, name, 0, "IMPRESS");
      }
    }
    else
    {
      name = iupAttribGetStr(ih, "IMAGE");
      if (name)
        gtkToggleSetPixbuf(ih, name, 0, "IMAGE");
    }
  }
}


/*************************************************************************/


static int gtkToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value,"NOTDEF"))
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, TRUE);
  else 
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);

    /* This action causes the toggled signal to be emitted. */
    iupAttribSetStr(ih, "_IUPGTK_IGNORE_TOGGLE", "1");

    if (iupStrBoolean(value))
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, TRUE);
    else
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, FALSE);

    iupAttribSetStr(ih, "_IUPGTK_IGNORE_TOGGLE", NULL);
  }

  return 0;
}

static char* gtkToggleGetValueAttrib(Ihandle* ih)
{
  int check = gtkToggleGetCheck(ih);
  if (check == -1)
    return "NOTDEF";
  else if (check == 1)
    return "ON";
  else
    return "OFF";
}

static int gtkToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    GtkButton* button = (GtkButton*)ih->handle;
    GtkLabel* label = (GtkLabel*)gtk_button_get_image(button);
    iupgtkSetMnemonicTitle(ih, label, value);
    return 1;
  }

  return 0;
}

static int gtkToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  GtkButton* button = (GtkButton*)ih->handle;
  float xalign, yalign;
  char value1[30]="", value2[30]="";

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    xalign = 1.0f;
  else if (iupStrEqualNoCase(value1, "ACENTER"))
    xalign = 0.5f;
  else /* "ALEFT" */
    xalign = 0;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    yalign = 1.0f;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    yalign = 0;
  else  /* ACENTER (default) */
    yalign = 0.5f;

  gtk_button_set_alignment(button, xalign, yalign);

  return 1;
}

static int gtkToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    GtkButton* button = (GtkButton*)ih->handle;
    GtkMisc* misc = (GtkMisc*)gtk_button_get_image(button);
    gtk_misc_set_padding(misc, ih->data->horiz_padding, ih->data->vert_padding);
  }
  return 0;
}

static int gtkToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* label = (GtkWidget*)gtk_button_get_image((GtkButton*)ih->handle);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkBaseSetFgColor(label, r, g, b);

  return 1;
}

static int gtkToggleSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetStandardFontAttrib(ih, value);

  if (ih->handle)
  {
    GtkWidget* label = gtk_button_get_image((GtkButton*)ih->handle);
    if (!label) return 1;

    gtk_widget_modify_font(label, (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih));

    if (ih->data->type == IUP_TOGGLE_TEXT)
      iupgtkFontUpdatePangoLayout(ih, gtk_label_get_layout((GtkLabel*)label));
  }
  return 1;
}

static int gtkToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMAGE"))
      iupAttribSetStr(ih, "IMAGE", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMINACTIVE"))
      iupAttribSetStr(ih, "IMINACTIVE", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGetStr(ih, "IMPRESS"))
      iupAttribSetStr(ih, "IMPRESS", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    gtkToggleUpdateImage(ih, iupStrBoolean(value), gtkToggleGetCheck(ih));

  return iupBaseSetActiveAttrib(ih, value);
}

/****************************************************************************************************/

static void gtkToggleToggled(GtkToggleButton *widget, Ihandle* ih)
{
  IFni cb;
  int check;

  if (iupAttribGetStr(ih, "_IUPGTK_IGNORE_TOGGLE"))
    return;

  check = gtkToggleGetCheck(ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

  (void)widget;
}

static int gtkToggleUpdate3StateCheck(Ihandle *ih, int keyb)
{
  int check = gtkToggleGetCheck(ih);
  if (check == 1)  /* GOTO check == -1 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, TRUE);
    gtkToggleToggled((GtkToggleButton*)ih->handle, ih);
    return TRUE; /* ignore message to avoid change toggle state */
  }
  else if (check == -1)  /* GOTO check == 0 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);
    if (keyb)
    {
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, FALSE);
      return TRUE; /* ignore message to avoid change toggle state */
    }
  }
  else  /* (check == 0)  GOTO check == 1 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);
    if (keyb)
    {
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, TRUE);
      return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  return FALSE;
}

static gboolean gtkToggleButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  if (iupAttribGetStr(ih, "_IUPGTK_IGNORE_TOGGLE"))
    return FALSE;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    char* name = iupAttribGetStr(ih, "IMPRESS");
    if (name)
    {
      if (evt->type == GDK_BUTTON_PRESS)
        gtkToggleUpdateImage(ih, iupdrvIsActive(ih), 1);
      else
        gtkToggleUpdateImage(ih, iupdrvIsActive(ih), 0);
    }
  }
  else
  {
    if (evt->type == GDK_BUTTON_RELEASE)
    {
      if (gtkToggleUpdate3StateCheck(ih, 0))
        return TRUE; /* ignore message to avoid change toggle state */
    }
  }
    
  (void)widget;
  return FALSE;
}

static gboolean gtkToggleKeyEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (evt->type == GDK_KEY_PRESS)
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
      return TRUE; /* ignore message to avoid change toggle state */
  }
  else
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
    {
      if (gtkToggleUpdate3StateCheck(ih, 1))
        return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  (void)widget;
  return FALSE;
}

static int gtkToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char *value;
  int is3state = 0;

  if (!ih->parent)
    return IUP_ERROR;

  if (radio)
    ih->data->radio = 1;

  value = iupAttribGetStr(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  if (radio)
  {
    GtkRadioButton* last_tg = (GtkRadioButton*)iupAttribGetStr(radio, "_IUPGTK_LASTRADIOBUTTON");
    if (last_tg)
      ih->handle = gtk_radio_button_new_from_widget(last_tg);
    else
      ih->handle = gtk_radio_button_new(NULL);
    iupAttribSetStr(radio, "_IUPGTK_LASTRADIOBUTTON", (char*)ih->handle);
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT)
    {
      ih->handle = gtk_check_button_new();

      if (iupAttribGetIntDefault(ih, "3STATE"))
        is3state = 1;
    }
    else
      ih->handle = gtk_toggle_button_new();
  }

  if (!ih->handle)
    return IUP_ERROR;

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    gtk_button_set_image((GtkButton*)ih->handle, gtk_label_new(NULL));
    gtk_toggle_button_set_mode((GtkToggleButton*)ih->handle, TRUE);
  }
  else
  {
    gtk_button_set_image((GtkButton*)ih->handle, gtk_image_new());
    gtk_toggle_button_set_mode((GtkToggleButton*)ih->handle, FALSE);
  }

  /* add to the parent, all GTK controls must call this. */
  iupgtkBaseAddToParent(ih);

  if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
    GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS;

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  g_signal_connect(G_OBJECT(ih->handle), "toggled",            G_CALLBACK(gtkToggleToggled), ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE || is3state)
  {
    g_signal_connect(G_OBJECT(ih->handle), "button-press-event",  G_CALLBACK(gtkToggleButtonEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkToggleButtonEvent), ih);
  }

  if (is3state)
  {
    g_signal_connect(G_OBJECT(ih->handle), "key-press-event",  G_CALLBACK(gtkToggleKeyEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "key-release-event",  G_CALLBACK(gtkToggleKeyEvent), ih);
  }

  gtk_widget_realize(ih->handle);

  /* ensure the default values, that are different from the native ones */
  gtk_button_set_alignment((GtkButton*)ih->handle, 0.5, 0.5);

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkToggleMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkToggleSetStandardFontAttrib, "DEFAULTFONT", IUP_NOT_MAPPED, IUP_INHERIT);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkToggleSetActiveAttrib, "YES", IUP_MAPPED, IUP_INHERIT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkToggleSetFgColorAttrib, "DLGFGCOLOR", IUP_MAPPED, IUP_INHERIT);  /* black */
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkToggleSetTitleAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkToggleSetAlignmentAttrib, "ACENTER:ACENTER", IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkToggleSetImageAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkToggleSetImInactiveAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, gtkToggleSetImPressAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", gtkToggleGetValueAttrib, gtkToggleSetValueAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, gtkToggleSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, IUP_MAPPED, IUP_INHERIT);
}
