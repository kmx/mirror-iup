/** \file
 * \brief List Control
 *
 * See Copyright Notice in iup.h
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_list.h"

#include "iupgtk_drv.h"


static void gtkListSelectionChanged(GtkTreeSelection* selection, Ihandle* ih);
static void gtkListComboBoxChanged(GtkComboBox* widget, Ihandle* ih);


void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  (void)ih;
  *h += 3;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2*5;
  (*x) += border_size;
  (*y) += border_size;

  if (ih->data->is_dropdown)
  {
    (*x) += 5; /* extra space for the dropdown button */

    if (ih->data->has_editbox)
      (*x) += 5; /* another extra space for the dropdown button */
    else
    {
      (*y) += 4; /* extra padding space */
      (*x) += 4; /* extra padding space */
    }
  }
  else
  {
    if (ih->data->has_editbox)
      (*y) += 2*3; /* internal border between editbox and list */
  }
}

void iupdrvListConvertXYToItem(Ihandle* ih, int x, int y, int *pos)
{
  if (!ih->data->is_dropdown)
  {
    GtkTreePath* path;
    if (gtk_tree_view_get_dest_row_at_pos((GtkTreeView*)ih->handle, x, y, &path, NULL))
    {
      int* indices = gtk_tree_path_get_indices(path);
      *pos = indices[0]+1;  /* IUP starts at 1 */
      gtk_tree_path_free (path);
    }
  }
}

static GtkTreeModel* gtkListGetModel(Ihandle* ih)
{
  if (ih->data->is_dropdown)
    return gtk_combo_box_get_model((GtkComboBox*)ih->handle);
  else
    return gtk_tree_view_get_model((GtkTreeView*)ih->handle);
}

int iupdrvListGetCount(Ihandle* ih)
{
  GtkTreeModel *model = gtkListGetModel(ih);
  return gtk_tree_model_iter_n_children(model, NULL);
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  GtkTreeModel *model = gtkListGetModel(ih);
  GtkTreeIter iter;
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, iupgtkStrConvertToUTF8(value), -1);
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  GtkTreeModel *model = gtkListGetModel(ih);
  GtkTreeIter iter;
  gtk_list_store_insert(GTK_LIST_STORE(model), &iter, pos);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, iupgtkStrConvertToUTF8(value), -1);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  GtkTreeModel *model = gtkListGetModel(ih);
  GtkTreeIter iter;
  if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
  {
    if (ih->data->is_dropdown && !ih->data->has_editbox)
    {
      /* must check if removing the current item */
      int curpos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
      if (pos == curpos)
      {
        if (curpos > 0) curpos--;
        else curpos++;

        gtk_combo_box_set_active((GtkComboBox*)ih->handle, curpos);
      }
    }

    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  GtkTreeModel *model = gtkListGetModel(ih);
  gtk_list_store_clear(GTK_LIST_STORE(model));
}


/*********************************************************************************/


static int gtkListSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetStandardFontAttrib(ih, value);

  if (ih->handle)
  {
    if (ih->data->is_dropdown)
    {
      GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGetStr(ih, "_IUPGTK_RENDERER");
      g_object_set(G_OBJECT(renderer), "font-desc", (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih), NULL);
      iupgtkFontUpdateObjectPangoLayout(ih, G_OBJECT(renderer));
    }

    if (ih->data->has_editbox)
    {
      GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
      gtk_widget_modify_font((GtkWidget*)entry, (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih));
      iupgtkFontUpdatePangoLayout(ih, gtk_entry_get_layout(entry));
    }
  }
  return 1;
}

static char* gtkListGetIdValueAttrib(Ihandle* ih, const char* name_id)
{
  int pos = iupListGetPos(ih, name_id);
  if (pos != -1)
  {
    GtkTreeIter iter;
    GtkTreeModel* model = gtkListGetModel(ih);
    if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
    {
      gchar *text = NULL;
      gtk_tree_model_get(model, &iter, 0, &text, -1);
      if (text)
        return iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(text));
    }
  }
  return NULL;
}

static int gtkListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGetStr(ih, "_IUP_EXTRAPARENT");
  if (scrolled_window && !ih->data->is_dropdown)
  {
    /* ignore given value, must use only from parent for the scrollbars */
    char* parent_value = iupAttribGetStrNativeParent(ih, "BGCOLOR");
    if (!parent_value) parent_value = IupGetGlobal("DLGBGCOLOR");

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget* sb;

      if (!GTK_IS_SCROLLED_WINDOW(scrolled_window))
        scrolled_window = (GtkScrolledWindow*)iupAttribGetStr(ih, "_IUPGTK_SCROLLED_WINDOW");

      iupgtkBaseSetBgColor((GtkWidget*)scrolled_window, r, g, b);

      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtkBaseSetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtkBaseSetBgColor(sb, r, g, b);
    }
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    iupgtkBaseSetBgColor(entry, r, g, b);
  }

  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGetStr(ih, "_IUPGTK_RENDERER");
    GdkColor color = {0L,0,0,0};

    color.red = iupCOLOR8TO16(r);
    color.green = iupCOLOR8TO16(g);
    color.blue = iupCOLOR8TO16(b);

    g_object_set(G_OBJECT(renderer), "cell-background-gdk", &color, NULL);
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtkListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkBaseSetFgColor(ih->handle, r, g, b);

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    iupgtkBaseSetFgColor(entry, r, g, b);
  }

  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGetStr(ih, "_IUPGTK_RENDERER");
    GdkColor color = {0L,0,0,0};

    color.red = iupCOLOR8TO16(r);
    color.green = iupCOLOR8TO16(g);
    color.blue = iupCOLOR8TO16(b);

    g_object_set(G_OBJECT(renderer), "foreground-gdk", &color, NULL);
  }

  return 1;
}

static char* gtkListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    return iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(gtk_entry_get_text(entry)));
  }
  else 
  {
    if (ih->data->is_dropdown)
    {
      int pos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
      char* str = iupStrGetMemory(50);
      sprintf(str, "%d", pos+1);  /* IUP starts at 1 */
      return str;
    }
    else
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      if (!ih->data->is_multiple)
      {
        GtkTreeIter iter;
        GtkTreeModel* tree_model;
        if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
        {
          char* str;
          GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
          int* indices = gtk_tree_path_get_indices(path);
          str = iupStrGetMemory(50);
          sprintf(str, "%d", indices[0]+1);  /* IUP starts at 1 */
          gtk_tree_path_free (path);
          return str;
        }
      }
      else
      {
        GList *il, *list = gtk_tree_selection_get_selected_rows(selection, NULL);
        int count = iupdrvListGetCount(ih);
        char* str = iupStrGetMemory(count+1);
        memset(str, '-', count);
        str[count]=0;
        for (il=list; il; il=il->next)
        {
          GtkTreePath* path = (GtkTreePath*)il->data;
          int* indices = gtk_tree_path_get_indices(path);
          str[indices[0]] = '+';
          gtk_tree_path_free(path);
        }
        g_list_free(list);
        return str;
      }
    }
  }

  return NULL;
}

static int gtkListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    if (!value) value = "";
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");
    gtk_entry_set_text(entry, iupgtkStrConvertToUTF8(value));
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
  }
  else 
  {
    if (ih->data->is_dropdown)
    {
      int pos;
      g_signal_handlers_block_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkListComboBoxChanged), ih);
      if (iupStrToInt(value, &pos)==1)
      {
        gtk_combo_box_set_active((GtkComboBox*)ih->handle, pos-1);    /* IUP starts at 1 */
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      }
      else
      {
        gtk_combo_box_set_active((GtkComboBox*)ih->handle, -1);    /* none */
        iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", NULL);
      }
      g_signal_handlers_unblock_by_func(G_OBJECT(ih->handle), G_CALLBACK(gtkListComboBoxChanged), ih);
    }
    else
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      if (!ih->data->is_multiple)
      {
        int pos;
        g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);
        if (iupStrToInt(value, &pos)==1)
        {
          GtkTreePath* path = gtk_tree_path_new_from_indices(pos-1, -1);   /* IUP starts at 1 */
          gtk_tree_selection_select_path(selection, path);
          gtk_tree_path_free(path);
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
        }
        else
        {
          gtk_tree_selection_unselect_all(selection);
          iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);
      }
      else
      {
        /* User has changed a multiple selection on a simple list. */
	      int i, len, count;

        g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);

        /* Clear all selections */
        gtk_tree_selection_unselect_all(selection);

        if (!value)
        {
          iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", NULL);
          return 0;
        }

	      len = strlen(value);
        count = iupdrvListGetCount(ih);
        if (len < count) 
          count = len;

        /* update selection list */
        for (i = 0; i<count; i++)
        {
          if (value[i]=='+')
          {
            GtkTreePath* path = gtk_tree_path_new_from_indices(i, -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
          }
        }
        iupAttribStoreStr(ih, "_IUPLIST_OLDVALUE", value);
        g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);
      }
    }
  }

  return 0;
}

static int gtkListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    if (iupStrBoolean(value))
      gtk_combo_box_popup((GtkComboBox*)ih->handle); 
    else
      gtk_combo_box_popdown((GtkComboBox*)ih->handle); 
  }
  return 0;
}

static int gtkListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      GtkTreePath* path = gtk_tree_path_new_from_indices(pos-1, -1);   /* IUP starts at 1 */
      gtk_tree_view_scroll_to_cell((GtkTreeView*)ih->handle, path, NULL, FALSE, 0, 0);
      gtk_tree_path_free(path);
    }
  }
  return 0;
}

static int gtkListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGetStr(ih, "_IUPGTK_RENDERER");
    g_object_set(G_OBJECT(renderer), "xpad", ih->data->spacing, 
                                     "ypad", ih->data->spacing, 
                                     NULL);
  }

  return 0;
}

static int gtkListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    GtkEntry* entry;
    GtkBorder border;
    border.bottom = border.top = ih->data->vert_padding;
    border.left = border.right = ih->data->horiz_padding;
    entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    gtk_entry_set_inner_border(entry, &border);
  }
  return 0;
}

static int gtkListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<1 || end<1) 
    return 0;

  start--; /* IUP starts at 1 */
  end--;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtkListGetSelectionAttrib(Ihandle* ih)
{
  char *str;
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    start++; /* IUP starts at 1 */
    end++;
    str = iupStrGetMemory(100);
    sprintf(str, "%d:%d", (int)start, (int)end);
    return str;
  }

  return NULL;
}

static int gtkListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtkListGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;
  char *str;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    str = iupStrGetMemory(100);
    sprintf(str, "%d:%d", (int)start, (int)end);
    return str;
  }

  return NULL;
}

static int gtkListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
    gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtkStrConvertToUTF8(value), -1, &start);
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
  }

  return 0;
}

static char* gtkListGetSelectedTextAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    char* selectedtext = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);
    char* str = iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(selectedtext));
    g_free(selectedtext);
    return str;
  }

  return NULL;
}

static int gtkListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  pos--; /* IUP starts at 1 */
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtkListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    char* str = iupStrGetMemory(50);
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    pos++; /* IUP starts at 1 */
    sprintf(str, "%d", (int)pos);
    return str;
  }
  else
    return NULL;
}

static int gtkListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtkListGetCaretPosAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    char* str = iupStrGetMemory(50);
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    sprintf(str, "%d", (int)pos);
    return str;
  }
  else
    return NULL;
}

static int gtkListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  if (pos < 1) pos = 1;
  pos--;  /* return to Motif referece */

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtkListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  sscanf(value,"%i",&pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtkListSetInsertAttrib(Ihandle* ih, const char* value)
{
  gint pos;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");  /* disable callbacks */
  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  pos = gtk_editable_get_position(GTK_EDITABLE(entry));
  gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtkStrConvertToUTF8(value), -1, &pos);
  iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);

  return 0;
}

static int gtkListSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    gint pos = strlen(gtk_entry_get_text(entry))+1;
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1"); /* disable callbacks */
    gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtkStrConvertToUTF8(value), -1, &pos);
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}

static int gtkListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;

  if (ih->handle)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
    gtk_entry_set_max_length(entry, ih->data->nc);
  }

  return 0;
}

static int gtkListSetClipboardAttrib(Ihandle *ih, const char *value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  /* disable callbacks */
  iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");
  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (iupStrEqualNoCase(value, "COPY"))
    gtk_editable_copy_clipboard(GTK_EDITABLE(entry));
  else if (iupStrEqualNoCase(value, "CUT"))
    gtk_editable_cut_clipboard(GTK_EDITABLE(entry));
  else if (iupStrEqualNoCase(value, "PASTE"))
    gtk_editable_paste_clipboard(GTK_EDITABLE(entry));
  else if (iupStrEqualNoCase(value, "CLEAR"))
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
  iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
  return 0;
}

static int gtkListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  gtk_editable_set_editable(GTK_EDITABLE(entry), !iupStrBoolean(value));
  return 0;
}

static char* gtkListGetReadOnlyAttrib(Ihandle* ih)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;
  entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
  if (!gtk_editable_get_editable(GTK_EDITABLE(entry)))
    return "YES";
  else
    return "NO";
}


/*********************************************************************************/


static void gtkListEditMoveCursor(GtkWidget* entry, GtkMovementStep step, gint count, gboolean extend_selection, Ihandle* ih)
{
  int pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = gtk_editable_get_position(GTK_EDITABLE(entry));

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;

    cb(ih, 1, pos+1, pos);
  }

  (void)step;
  (void)count;
  (void)extend_selection;
}

static gboolean gtkListEditKeyPressEvent(GtkWidget* entry, GdkEventKey *evt, Ihandle *ih)
{
  if ((evt->keyval == GDK_Up || evt->keyval == GDK_KP_Up) ||
      (evt->keyval == GDK_Prior || evt->keyval == GDK_KP_Page_Up) ||
      (evt->keyval == GDK_Down || evt->keyval == GDK_KP_Down) ||
      (evt->keyval == GDK_Next || evt->keyval == GDK_KP_Page_Down))
  {
    int pos = -1;
    GtkTreeIter iter;
    GtkTreeModel* model = NULL;
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
      int* indices = gtk_tree_path_get_indices(path);
      pos = indices[0];
      gtk_tree_path_free(path);
    }

    if (pos == -1)
      pos = 0;
    else if (evt->keyval == GDK_Up || evt->keyval == GDK_KP_Up)
    {
      pos--;
      if (pos < 0) pos = 0;
    }
    else if (evt->keyval == GDK_Prior || evt->keyval == GDK_KP_Page_Up)
    {
      pos -= 5;
      if (pos < 0) pos = 0;
    }
    else if (evt->keyval == GDK_Down || evt->keyval == GDK_KP_Down)
    {
      int count = gtk_tree_model_iter_n_children(model, NULL);
      pos++;
      if (pos > count-1) pos = count-1;
    }
    else if (evt->keyval == GDK_Next || evt->keyval == GDK_KP_Page_Down)
    {
      int count = gtk_tree_model_iter_n_children(model, NULL);
      pos += 5;
      if (pos > count-1) pos = count-1;
    }

    if (pos != -1)
    {
      GtkTreePath* path = gtk_tree_path_new_from_indices(pos, -1);
      g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);
      gtk_tree_selection_select_path(selection, path);
      g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(gtkListSelectionChanged), ih);
      gtk_tree_path_free(path);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);

      if (!model) model = gtkListGetModel(ih);

      if (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos))
      {
        gchar *text = NULL;
        gtk_tree_model_get(model, &iter, 0, &text, -1);
        if (text)
          gtk_entry_set_text((GtkEntry*)entry, text);
      }

    }
  }

  return iupgtkKeyPressEvent(entry, evt, ih);
}

static gboolean gtkListEditKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  gtkListEditMoveCursor(widget, 0, 0, 0, ih);
  (void)evt;
  return FALSE;
}

static gboolean gtkListEditButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  gtkListEditMoveCursor(widget, 0, 0, 0, ih);
  (void)evt;
  return FALSE;
}

static int gtkListCallEditCb(Ihandle* ih, GtkEditable *editable, const char* insert_value, int len, int start, int end)
{
  char *new_value, *value;
  int ret = -1, key = 0;

  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (!cb && !ih->data->mask)
    return -1; /* continue */

  value = iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(gtk_entry_get_text(GTK_ENTRY(editable))));

  if (!insert_value)
  {
    new_value = iupStrDup(value);
    if (end<0) end = strlen(value)+1;
    iupStrRemove(new_value, start, end, 1);
  }
  else
  {
    if (!value)
      new_value = iupStrDup(insert_value);
    else
    {
      if (len < end-start)
      {
        new_value = iupStrDup(value);
        new_value = iupStrInsert(new_value, insert_value, start, end);
      }
      else
        new_value = iupStrInsert(value, insert_value, start, end);
    }
  }

  if (insert_value && insert_value[0]!=0 && insert_value[1]==0)
    key = insert_value[0];

  if (!new_value)
    return -1; /* continue */

  if (ih->data->nc && (int)strlen(new_value) > ih->data->nc)
  {
    if (new_value != value) free(new_value);
    return 0; /* abort */
  }

  if (ih->data->mask && iupMaskCheck(ih->data->mask, new_value)==0)
  {
    if (new_value != value) free(new_value);
    return 0; /* abort */
  }

  if (cb)
  {
    int cb_ret = cb(ih, key, (char*)new_value);
    if (cb_ret==IUP_IGNORE)
      ret = 0; /* abort */
    else if (cb_ret==IUP_CLOSE)
    {
      IupExitLoop();
      ret = 0; /* abort */
    }
    else if (cb_ret!=0 && key!=0 && 
             cb_ret != IUP_DEFAULT && cb_ret != IUP_CONTINUE)  
      ret = cb_ret; /* abort and replace */
  }

  if (new_value != value) free(new_value);
  return ret; /* continue */
}

static void gtkListEditDeleteText(GtkEditable *editable, int start, int end, Ihandle* ih)
{
  if (iupAttribGetStr(ih, "_IUPGTK_DISABLE_TEXT_CB"))
    return;

  if (gtkListCallEditCb(ih, editable, NULL, 0, start, end)==0)
    g_signal_stop_emission_by_name(editable, "delete_text");
}

static void gtkListEditInsertText(GtkEditable *editable, char *insert_value, int len, int *pos, Ihandle* ih)
{
  int ret;

  if (iupAttribGetStr(ih, "_IUPGTK_DISABLE_TEXT_CB"))
    return;

  ret = gtkListCallEditCb(ih, editable, iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(insert_value)), len, *pos, *pos);
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "insert_text");
  else if (ret != -1)
  {
    insert_value[0] = (char)ret;  /* replace key */

    /* disable callbacks */
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");
    gtk_editable_insert_text(editable, insert_value, 1, pos);
    iupAttribSetStr(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);

    g_signal_stop_emission_by_name(editable, "insert_text"); 
  }
}

static void gtkListComboBoxPopupShownChanged(GtkComboBox* widget, GParamSpec *pspec, Ihandle* ih)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
  {
    gboolean popup_shown;
    g_object_get(widget, "popup-shown", &popup_shown, NULL);
    cb(ih, popup_shown);
  }
  (void)pspec;
}

static void gtkListComboBoxChanged(GtkComboBox* widget, Ihandle* ih)
{
  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int pos = gtk_combo_box_get_active((GtkComboBox*)ih->handle);
    pos++;  /* IUP starts at 1 */
    iupListSingleCallActionCallback(ih, cb, pos);
  }
  (void)widget;
}

static gboolean gtkListSimpleKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  (void)ih;
  (void)widget;
  if (evt->keyval == GDK_Return || evt->keyval == GDK_KP_Enter)
  {
    /* used to avoid the call to DBLCLICK_CB in the default processing */
    iupgtkKeyPressEvent(widget, evt, ih);
    return TRUE;
  }
  return iupgtkKeyPressEvent(widget, evt, ih);
}

static void gtkListRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, Ihandle* ih)
{
  IFnis cb = (IFnis) IupGetCallback(ih, "DBLCLICK_CB");
  if (cb)
  {
    int* indices = gtk_tree_path_get_indices(path);
    iupListSingleCallDblClickCallback(ih, cb, indices[0]+1);  /* IUP starts at 1 */
  }
  (void)column;
  (void)tree_view;
}

static void gtkListSelectionChanged(GtkTreeSelection* selection, Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    /* must manually update its contents */
    GtkTreeIter iter;
    GtkTreeModel* tree_model;
    if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
      char* value = NULL;
      gtk_tree_model_get(tree_model, &iter, 0, &value, -1);
      if (value)
      {
        GtkEntry* entry = (GtkEntry*)iupAttribGetStr(ih, "_IUPGTK_ENTRY");
        gtk_entry_set_text(entry, value);
      }
      gtk_tree_path_free(path);
    }
  }

  if (!ih->data->is_multiple)
  {
    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      GtkTreeIter iter;
      GtkTreeModel* tree_model;
      if (gtk_tree_selection_get_selected(selection, &tree_model, &iter))
      {
        GtkTreePath *path = gtk_tree_model_get_path(tree_model, &iter);
        int* indices = gtk_tree_path_get_indices(path);
        iupListSingleCallActionCallback(ih, cb, indices[0]+1);  /* IUP starts at 1 */
        gtk_tree_path_free (path);
      }
    }
  }
  else
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
    if (multi_cb || cb)
    {
      GList *il, *list = gtk_tree_selection_get_selected_rows(selection, NULL);
      int i, sel_count = g_list_length(list);
      int* pos = malloc(sizeof(int)*sel_count);
      for (il=list, i=0; il; il=il->next, i++)
      {
        GtkTreePath* path = (GtkTreePath*)il->data;
        int* indices = gtk_tree_path_get_indices(path);
        pos[i] = indices[0];
        gtk_tree_path_free(path);
      }
      g_list_free(list);

      iupListMultipleCallActionCallback(ih, cb, multi_cb, pos, sel_count);
      free(pos);
    }
  }
}


/*********************************************************************************/


static int gtkListMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;
  GtkListStore *store;
  GtkCellRenderer *renderer;

  store = gtk_list_store_new(1, G_TYPE_STRING);

  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
      ih->handle = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(store), 0);
    else
      ih->handle = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    if (!ih->handle)
      return IUP_ERROR;

    g_object_set(G_OBJECT(ih->handle), "has-frame", TRUE, NULL);

    if (ih->data->has_editbox)
    {
      GtkWidget *entry;
      GList* list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(ih->handle));
      renderer = list->data;
      g_list_free(list);

      entry = gtk_bin_get_child(GTK_BIN(ih->handle));
      iupAttribSetStr(ih, "_IUPGTK_ENTRY", (char*)entry);

      g_signal_connect(G_OBJECT(entry), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(entry), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(entry), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(entry), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(entry), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);
      g_signal_connect(G_OBJECT(entry), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtkListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtkListEditInsertText), ih);
      g_signal_connect_after(G_OBJECT(entry), "move-cursor", G_CALLBACK(gtkListEditMoveCursor), ih);  /* only report some caret movements */
      g_signal_connect_after(G_OBJECT(entry), "key-release-event", G_CALLBACK(gtkListEditKeyReleaseEvent), ih);
      g_signal_connect(G_OBJECT(entry), "button-press-event", G_CALLBACK(gtkListEditButtonEvent), ih);  /* if connected "after" then it is ignored */
      g_signal_connect(G_OBJECT(entry), "button-release-event",G_CALLBACK(gtkListEditButtonEvent), ih);

      if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
        GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS;
    }
    else
    {
      /* had to add an event box just to get get/killfocus,enter/leave events */
      GtkWidget *box = gtk_event_box_new();
      gtk_container_add((GtkContainer*)box, ih->handle);
      iupAttribSetStr(ih, "_IUP_EXTRAPARENT", (char*)box);

      renderer = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ih->handle), renderer, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ih->handle), renderer, "text", 0, NULL);

      g_signal_connect(G_OBJECT(box), "focus-in-event",  G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(box), "focus-out-event", G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(box), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(box), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "key-press-event", G_CALLBACK(iupgtkKeyPressEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "show-help",       G_CALLBACK(iupgtkShowHelp), ih);

      if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
        GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS;
      else
        GTK_WIDGET_FLAGS(box) |= GTK_CAN_FOCUS;
    }

    g_signal_connect(ih->handle, "changed", G_CALLBACK(gtkListComboBoxChanged), ih);
    g_signal_connect(ih->handle, "notify::popup-shown", G_CALLBACK(gtkListComboBoxPopupShownChanged), ih);

    renderer->xpad = 0;
    renderer->ypad = 0;
    iupAttribSetStr(ih, "_IUPGTK_RENDERER", (char*)renderer);
  }
  else
  {
    GtkCellRenderer *renderer;
    GtkTreeSelection* selection;
    GtkTreeViewColumn *column;
    GtkPolicyType scrollbar_policy;

    ih->handle = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    if (!ih->handle)
      return IUP_ERROR;

    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL);

    if (ih->data->has_editbox)
    {
      GtkBox* vbox = (GtkBox*)gtk_vbox_new(FALSE, 0);

      GtkWidget *entry = gtk_entry_new();
      gtk_widget_show(entry);
      gtk_box_pack_start(vbox, entry, FALSE, FALSE, 0);
      iupAttribSetStr(ih, "_IUPGTK_ENTRY", (char*)entry);

      gtk_widget_show((GtkWidget*)vbox);
      gtk_box_pack_end_defaults(vbox, (GtkWidget*)scrolled_window);
      iupAttribSetStr(ih, "_IUP_EXTRAPARENT", (char*)vbox);
      iupAttribSetStr(ih, "_IUPGTK_SCROLLED_WINDOW", (char*)scrolled_window);

      GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS; /* focus goes only to the edit box */
      if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
        GTK_WIDGET_FLAGS(entry) &= ~GTK_CAN_FOCUS;

      g_signal_connect(G_OBJECT(entry), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(entry), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(entry), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(entry), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(entry), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtkListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtkListEditInsertText), ih);
      g_signal_connect_after(G_OBJECT(entry), "move-cursor", G_CALLBACK(gtkListEditMoveCursor), ih);  /* only report some caret movements */
      g_signal_connect(G_OBJECT(entry), "key-press-event",    G_CALLBACK(gtkListEditKeyPressEvent), ih);
      g_signal_connect_after(G_OBJECT(entry), "key-release-event", G_CALLBACK(gtkListEditKeyReleaseEvent), ih);
      g_signal_connect(G_OBJECT(entry), "button-press-event", G_CALLBACK(gtkListEditButtonEvent), ih);  /* if connected "after" then it is ignored */
      g_signal_connect(G_OBJECT(entry), "button-release-event",G_CALLBACK(gtkListEditButtonEvent), ih);
    }
    else
    {
      iupAttribSetStr(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

      if (!iupStrBoolean(iupAttribGetStrDefault(ih, "CANFOCUS")))
        GTK_WIDGET_FLAGS(ih->handle) &= ~GTK_CAN_FOCUS;

      g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(gtkListSimpleKeyPressEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);
    }

    column = gtk_tree_view_column_new();

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer, "text", 0, NULL);

    iupAttribSetStr(ih, "_IUPGTK_RENDERER", (char*)renderer);
    g_object_set(G_OBJECT(renderer), "xpad", 0, NULL);
    g_object_set(G_OBJECT(renderer), "ypad", 0, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(ih->handle), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ih->handle), FALSE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ih->handle), FALSE);

    gtk_container_add((GtkContainer*)scrolled_window, ih->handle);
    gtk_widget_show((GtkWidget*)scrolled_window);
    gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_IN); 

    if (ih->data->sb)
    {
      if (iupStrBoolean(iupAttribGetStrDefault(ih, "AUTOHIDE")))
        scrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        scrollbar_policy = GTK_POLICY_ALWAYS;
    }
    else
      scrollbar_policy = GTK_POLICY_NEVER;

    gtk_scrolled_window_set_policy(scrolled_window, scrollbar_policy, scrollbar_policy);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
    if (!ih->data->has_editbox && ih->data->is_multiple)
    {
      gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
      gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(ih->handle), TRUE);
    }
    else
      gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

    g_signal_connect(G_OBJECT(selection), "changed",  G_CALLBACK(gtkListSelectionChanged), ih);
    g_signal_connect(G_OBJECT(ih->handle), "row-activated", G_CALLBACK(gtkListRowActivated), ih);
    g_signal_connect(G_OBJECT(ih->handle), "motion-notify-event",G_CALLBACK(iupgtkMotionNotifyEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(iupgtkButtonEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(iupgtkButtonEvent), ih);
  }

  /* add to the parent, all GTK controls must call this. */
  iupgtkBaseAddToParent(ih);

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSetStr(ih, "DRAGDROP", "YES");

  iupListSetInitialItems(ih);

  return IUP_NOERROR;
}

void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkListMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkListSetStandardFontAttrib, IupGetGlobal("DEFAULTFONT"), IUP_NOT_MAPPED, IUP_INHERIT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkListSetBgColorAttrib, IupGetGlobal("TXTBGCOLOR"), IUP_MAPPED, IUP_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkListSetFgColorAttrib, IupGetGlobal("TXTFGCOLOR"), IUP_MAPPED, IUP_INHERIT);

  /* IupList only */
  iupClassRegisterAttribute(ic, "IDVALUE", (IattribGetFunc)gtkListGetIdValueAttrib, (IattribSetFunc)iupListSetIdValueAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", gtkListGetValueAttrib, gtkListSetValueAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, gtkListSetShowDropdownAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, gtkListSetTopItemAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, iupgtkSetDragDropAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, gtkListSetSpacingAttrib, NULL, IUP_NOT_MAPPED, IUP_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, gtkListSetPaddingAttrib, "0x0", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtkListGetSelectedTextAttrib, gtkListSetSelectedTextAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtkListGetSelectionAttrib, gtkListSetSelectionAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtkListGetSelectionPosAttrib, gtkListSetSelectionPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtkListGetCaretAttrib, gtkListSetCaretAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtkListGetCaretPosAttrib, gtkListSetCaretPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtkListSetInsertAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtkListSetAppendAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", gtkListGetReadOnlyAttrib, gtkListSetReadOnlyAttrib, NULL, IUP_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, gtkListSetNCAttrib, NULL, IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtkListSetClipboardAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtkListSetScrollToAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtkListSetScrollToPosAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
}
