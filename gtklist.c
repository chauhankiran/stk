/* GTK - The General Toolkit (written for the GIMP)
 * Copyright (C) 1995 Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtklist.h"
#include "gtkmain.h"
#include "gtkmisc.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define LIST_TIMER_LENGTH  40


typedef struct _GtkList      GtkList;
typedef struct _GtkListItem  GtkListItem;
typedef struct _GtkListBox   GtkListBox;

struct _GtkList
{
  GtkContainer container;

  GList *children;
  GdkWindow *view_window;

  gint list_width;
  gint list_height;

  GtkSelectionMode mode;
  GtkDataList selection;

  guint32 timer;
  guint16 select_start_pos;
  guint16 select_end_pos;
  unsigned int scroll_direction : 1;
  unsigned int have_grab : 1;

  GtkDataAdjustment *hadjustment;
  GtkDataAdjustment *vadjustment;
  GtkObserver adjustment_observer;
};

struct _GtkListItem
{
  GtkContainer container;

  GtkWidget *child;

  GtkDataInt state;
  GtkObserver state_observer;
};

static void   gtk_list_destroy        (GtkWidget       *widget);
static void   gtk_list_map            (GtkWidget       *widget);
static void   gtk_list_unmap          (GtkWidget       *widget);
static void   gtk_list_realize        (GtkWidget       *widget);
static void   gtk_list_draw           (GtkWidget       *widget,
				       GdkRectangle    *area,
				       gint             is_expose);
static gint   gtk_list_event          (GtkWidget       *widget,
				       GdkEvent        *event);
static void   gtk_list_size_request   (GtkWidget       *widget,
				       GtkRequisition  *requisition);
static void   gtk_list_size_allocate  (GtkWidget       *widget,
				       GtkAllocation   *allocation);
static gint   gtk_list_is_child       (GtkWidget       *widget,
				       GtkWidget       *child);
static gint   gtk_list_locate         (GtkWidget       *widget,
				       GtkWidget      **child,
				       gint             x,
				       gint             y);
static void   gtk_list_add            (GtkContainer    *container,
				       GtkWidget       *widget);
static void   gtk_list_remove         (GtkContainer    *container,
				       GtkWidget       *widget);
static void   gtk_list_foreach        (GtkContainer    *container,
				       GtkCallback      callback,
				       gpointer         callback_data);
static void   gtk_list_select_child   (GtkList         *list,
				       GtkListItem     *list_item,
				       gint             toggle);
static void   gtk_list_select_update  (GtkList         *list,
				       gint             new_end_pos);
static void   gtk_list_unselect_child (GtkList         *list,
				       GtkListItem     *list_item);
static void   gtk_list_unselect_all   (GtkList         *list);

static void   gtk_list_add_timer      (GtkList         *list);
static void   gtk_list_remove_timer   (GtkList         *list);
static gint   gtk_list_timer          (gpointer         data);

static gint   gtk_list_adjustment_update     (GtkObserver *observer,
					      GtkData     *data);
static void   gtk_list_adjustment_disconnect (GtkObserver *observer,
					      GtkData     *data);


static void   gtk_list_item_destroy             (GtkWidget       *widget);
static void   gtk_list_item_map                 (GtkWidget       *widget);
static void   gtk_list_item_unmap               (GtkWidget       *widget);
static void   gtk_list_item_realize             (GtkWidget       *widget);
static void   gtk_list_item_draw                (GtkWidget       *widget,
						 GdkRectangle    *area,
						 gint             is_expose);
static gint   gtk_list_item_event               (GtkWidget       *widget,
						 GdkEvent        *event);
static void   gtk_list_item_size_request        (GtkWidget       *widget,
						 GtkRequisition  *requisition);
static void   gtk_list_item_size_allocate       (GtkWidget       *widget,
						 GtkAllocation   *allocation);
static gint   gtk_list_item_is_child            (GtkWidget       *widget,
						 GtkWidget       *child);
static gint   gtk_list_item_locate              (GtkWidget       *widget,
						 GtkWidget      **child,
						 gint             x,
						 gint             y);
static void   gtk_list_item_activate            (GtkWidget       *widget);
static void   gtk_list_item_set_state           (GtkWidget       *widget,
						 GtkStateType     state);
static gint   gtk_list_item_install_accelerator (GtkWidget       *widget,
						 gchar            accelerator_key,
						 guint8           accelerator_mods);
static void   gtk_list_item_add                 (GtkContainer    *container,
						 GtkWidget       *widget);
static void   gtk_list_item_remove              (GtkContainer    *container,
						 GtkWidget       *widget);
static void   gtk_list_item_foreach             (GtkContainer    *container,
						 GtkCallback      callback,
						 gpointer         callback_data);

static gint   gtk_list_item_state_update     (GtkObserver *observer,
					      GtkData     *data);
static void   gtk_list_item_state_disconnect (GtkObserver *observer,
					      GtkData     *data);


static guint16 list_type = 0;
static guint16 list_item_type = 0;

static GtkWidgetFunctions list_widget_functions =
{
  gtk_list_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_list_map,
  gtk_list_unmap,
  gtk_list_realize,
  gtk_list_draw,
  gtk_widget_default_draw_focus,
  gtk_list_event,
  gtk_list_size_request,
  gtk_list_size_allocate,
  gtk_list_is_child,
  gtk_list_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions list_item_widget_functions =
{
  gtk_list_item_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_list_item_map,
  gtk_list_item_unmap,
  gtk_list_item_realize,
  gtk_list_item_draw,
  gtk_widget_default_draw_focus,
  gtk_list_item_event,
  gtk_list_item_size_request,
  gtk_list_item_size_allocate,
  gtk_list_item_is_child,
  gtk_list_item_locate,
  gtk_list_item_activate,
  gtk_list_item_set_state,
  gtk_list_item_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions list_container_functions =
{
  gtk_list_add,
  gtk_list_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_list_foreach,
};

static GtkContainerFunctions list_item_container_functions =
{
  gtk_list_item_add,
  gtk_list_item_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_list_item_foreach,
};


GtkWidget*
gtk_list_new (GtkDataAdjustment *hadjustment,
	      GtkDataAdjustment *vadjustment)
{
  GtkList *list;

  g_function_enter ("gtk_list_new");

  list = g_new (GtkList, 1);

  list->container.widget.type = gtk_get_list_type ();
  list->container.widget.function_table = &list_widget_functions;
  list->container.function_table = &list_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) list);
  gtk_container_set_defaults ((GtkWidget*) list);

  list->children = NULL;
  list->view_window = NULL;
  list->list_width = 1;
  list->list_height = 1;
  list->mode = GTK_SELECTION_SINGLE;

  gtk_data_init ((GtkData*) &list->selection);
  list->selection.list = NULL;
  list->timer = 0;
  list->scroll_direction = 0;
  list->have_grab = FALSE;

  if (hadjustment)
    list->hadjustment = hadjustment;
  else
    list->hadjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  if (vadjustment)
    list->hadjustment = vadjustment;
  else
    list->vadjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  list->adjustment_observer.update = gtk_list_adjustment_update;
  list->adjustment_observer.disconnect = gtk_list_adjustment_disconnect;
  list->adjustment_observer.user_data = list;

  gtk_data_attach ((GtkData*) list->hadjustment, &list->adjustment_observer);
  gtk_data_attach ((GtkData*) list->vadjustment, &list->adjustment_observer);

  g_function_leave ("gtk_list_new");
  return ((GtkWidget*) list);
}

GtkWidget*
gtk_list_item_new ()
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_new");

  list_item = g_new (GtkListItem, 1);

  list_item->container.widget.type = gtk_get_list_item_type ();
  list_item->container.widget.function_table = &list_item_widget_functions;
  list_item->container.function_table = &list_item_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) list_item);
  gtk_container_set_defaults ((GtkWidget*) list_item);

  list_item->child = NULL;

  gtk_data_init ((GtkData*) &list_item->state);
  list_item->state.value = GTK_STATE_NORMAL;
  list_item->state.data.type = gtk_data_int_type ();
  list_item->state.data.observers = NULL;

  list_item->state_observer.update = gtk_list_item_state_update;
  list_item->state_observer.disconnect = gtk_list_item_state_disconnect;
  list_item->state_observer.user_data = list_item;

  gtk_data_attach ((GtkData*) &list_item->state, &list_item->state_observer);

  g_function_leave ("gtk_list_item_new");
  return ((GtkWidget*) list_item);
}

GtkWidget*
gtk_list_item_new_with_label (gchar *label)
{
  GtkWidget *list_item;
  GtkWidget *label_widget;

  g_function_enter ("gtk_list_item_new_with_label");

  g_assert (label != NULL);

  list_item = gtk_list_item_new ();
  label_widget = gtk_label_new (label);
  gtk_container_add (list_item, label_widget);

  gtk_label_set_alignment (label_widget, 0.0, 0.5);
  gtk_widget_show (label_widget);

  g_function_leave ("gtk_list_item_new_with_label");
  return list_item;
}

void
gtk_list_insert_items (GtkWidget *list,
		       GList     *items,
		       gint       position)
{
  GtkList *rlist;
  GtkWidget *widget;
  GList *temp_list;
  GList *last;
  gint nchildren;

  g_function_enter ("gtk_list_insert_items");

  g_assert (list != NULL);
  g_assert (items != NULL);

  rlist = (GtkList*) list;
  nchildren = g_list_length (rlist->children);

  temp_list = items;
  while (temp_list)
    {
      widget = temp_list->data;
      temp_list = temp_list->next;

      widget->parent = (GtkContainer*) list;

      if (GTK_WIDGET_VISIBLE (widget->parent))
	{
	  if (GTK_WIDGET_REALIZED (widget->parent) &&
	      !GTK_WIDGET_REALIZED (widget))
	    gtk_widget_realize (widget);

	  if (GTK_WIDGET_MAPPED (widget->parent) &&
	      !GTK_WIDGET_MAPPED (widget))
	    gtk_widget_map (widget);
	}
    }

  if ((position < 0) || (position > nchildren))
    position = nchildren;

  if (position == nchildren)
    {
      if (rlist->children)
	{
	  temp_list = g_list_last (rlist->children);
	  temp_list->next = items;
	  items->prev = temp_list;
	}
      else
	{
	  rlist->children = items;
	}
    }
  else
    {
      temp_list = g_list_nth (rlist->children, position);
      last = g_list_last (items);

      if (temp_list->prev)
	temp_list->prev->next = items;
      last->next = temp_list;
      items->prev = temp_list->prev;
      temp_list->prev = last;

      if (temp_list == rlist->children)
	rlist->children = items;
    }

  widget = rlist->children->data;

  if ((rlist->mode == GTK_SELECTION_BROWSE) && !rlist->selection.list)
    gtk_list_select_child (rlist, (GtkListItem*) widget, FALSE);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (rlist))
    gtk_container_need_resize ((GtkContainer*) rlist, widget);

  g_function_leave ("gtk_list_insert_items");
}

void
gtk_list_append_items (GtkWidget *list,
		       GList     *items)
{
  g_function_enter ("gtk_list_append_items");

  g_assert (list != NULL);
  g_assert (items != NULL);

  gtk_list_insert_items (list, items, -1);

  g_function_leave ("gtk_list_append_items");
}

void
gtk_list_prepend_items (GtkWidget *list,
			GList     *items)
{
  g_function_enter ("gtk_list_prepend_items");

  g_assert (list != NULL);
  g_assert (items != NULL);

  gtk_list_insert_items (list, items, 0);

  g_function_leave ("gtk_list_prepend_items");
}

void
gtk_list_remove_items (GtkWidget *list,
		       GList     *items)
{
  GtkList *rlist;
  GtkWidget *widget;
  GList *temp_list;
  GList *temp_list2;

  g_function_enter ("gtk_list_remove_items");

  g_assert (list != NULL);
  g_assert (items != NULL);

  /*  traverse through the list of items, searching for
   *  each in the list's children list, and removing any located instances
   */
  rlist = (GtkList*) list;
  temp_list = items;
  while (temp_list)
    {
      widget = temp_list->data;
      temp_list = temp_list->next;

      /*  remove link from the list of children  */
      temp_list2 = rlist->children;
      while (temp_list2)
	{
	  if (widget == (GtkWidget*) temp_list2->data)
	    {
	      rlist->children = g_list_remove_link (rlist->children, temp_list2);
	      break;
	    }
	  temp_list2 = temp_list2->next;
	}

      /*  remove link from the list of selected items  */
      temp_list2 = rlist->selection.list;
      while (temp_list2)
	{
	  if (widget == (GtkWidget*) temp_list2->data)
	    {
	      rlist->selection.list = g_list_remove_link (rlist->selection.list, temp_list2);
	      break;
	    }
	  temp_list2 = temp_list2->next;
	}

      if (GTK_WIDGET_MAPPED (widget))
	gtk_widget_unmap (widget);
      widget->parent = NULL;
    }

  if (rlist->children)
    {
      widget = rlist->children->data;

      if ((rlist->mode == GTK_SELECTION_BROWSE) && !rlist->selection.list)
	gtk_list_select_child (rlist, (GtkListItem*) widget, FALSE);
    }
  else
    {
      widget = NULL;
    }

  if (widget && GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (rlist))
    gtk_container_need_resize ((GtkContainer*) rlist, widget);

  g_function_leave ("gtk_list_remove_items");
}

void
gtk_list_clear_items (GtkWidget *list,
		      gint       start,
		      gint       end)
{
  GtkList *rlist;
  GtkWidget *widget;
  GList *temp_list;
  GList *temp_list2;
  GList *start_list;
  GList *end_list;
  gint nchildren;

  g_function_enter ("gtk_list_clear_items");

  g_assert (list != NULL);
  g_assert (start >= 0);

  rlist = (GtkList*) list;
  nchildren = g_list_length (rlist->children);

  if (nchildren > 0)
    {
      if ((end < 0) || (end > nchildren))
	end = nchildren;

      g_assert (start < end);

      start_list = g_list_nth (rlist->children, start);
      end_list = g_list_nth (rlist->children, end);

      if (start_list->prev)
	start_list->prev->next = end_list;
      if (end_list && end_list->prev)
	end_list->prev->next = NULL;
      if (end_list)
	end_list->prev = start_list->prev;
      if (start_list == rlist->children)
	rlist->children = end_list;

      temp_list = start_list;
      while (temp_list)
	{
	  widget = temp_list->data;
	  temp_list = temp_list->next;

	  temp_list2 = rlist->selection.list;
	  while (temp_list2)
	    {
	      if (widget == (GtkWidget*) temp_list2->data)
		{
		  rlist->selection.list = g_list_remove_link (rlist->selection.list, temp_list2);
		  break;
		}
	      temp_list2 = temp_list2->next;
	    }

	  gtk_widget_destroy (widget);
	}

      g_list_free (start_list);

      if (rlist->children)
	{
	  widget = rlist->children->data;

	  if ((rlist->mode == GTK_SELECTION_BROWSE) && !rlist->selection.list)
	    gtk_list_select_child (rlist, (GtkListItem*) widget, FALSE);
	}
      else
	{
	  widget = NULL;
	}

      if (widget && GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (rlist))
	gtk_container_need_resize ((GtkContainer*) rlist, widget);
    }

  g_function_leave ("gtk_list_clear_items");
}

void
gtk_list_set_selection_mode (GtkWidget        *list,
			     GtkSelectionMode  mode)
{
  GtkList *rlist;

  g_function_enter ("gtk_list_set_selection_mode");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  rlist->mode = mode;

  g_function_leave ("gtk_list_set_selection_mode");
}

void
gtk_list_get_list_size (GtkWidget *list,
			gint    *width,
			gint    *height)
{
  GtkList *rlist;

  g_function_enter ("gtk_list_get_list_size");

  g_assert (list != NULL);
  g_assert (width != NULL);
  g_assert (height != NULL);
  rlist = (GtkList*) list;

  *width = rlist->list_width;
  *height = rlist->list_height;

  g_function_leave ("gtk_list_get_list_size");
}

GtkData*
gtk_list_get_hadjustment (GtkWidget *list)
{
  GtkList *rlist;
  GtkData *data;

  g_function_enter ("gtk_list_get_hadjustment");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  data = (GtkData*) rlist->hadjustment;

  g_function_leave ("gtk_list_get_hadjustment");
  return data;
}

GtkData*
gtk_list_get_vadjustment (GtkWidget *list)
{
  GtkList *rlist;
  GtkData *data;

  g_function_enter ("gtk_list_get_vadjustment");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  data = (GtkData*) rlist->vadjustment;

  g_function_leave ("gtk_list_get_vadjustment");
  return data;
}

GtkData*
gtk_list_item_get_state (GtkWidget *item)
{
  GtkListItem *ritem;
  GtkData *data;

  g_function_enter ("gtk_list_item_get_state");

  g_assert (item != NULL);

  ritem = (GtkListItem*) item;
  data = (GtkData*) &ritem->state;

  g_function_leave ("gtk_list_item_get_state");
  return data;
}

void
gtk_list_select_item (GtkWidget *list,
		      gint     item)
{
  GtkList *rlist;
  GtkListItem *list_item;
  GList *temp_list;

  g_function_enter ("gtk_list_select_item");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  temp_list = g_list_nth (rlist->children, item);
  g_assert (temp_list != NULL);

  list_item = temp_list->data;
  g_assert (list_item != NULL);

  gtk_list_select_child (rlist, list_item, FALSE);

  g_function_leave ("gtk_list_select_item");
}

void
gtk_list_unselect_item (GtkWidget *list,
			gint     item)
{
  GtkList *rlist;
  GtkListItem *list_item;
  GList *temp_list;

  g_function_enter ("gtk_list_unselect_item");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  temp_list = g_list_nth (rlist->children, item);
  g_assert (temp_list != NULL);

  list_item = temp_list->data;
  g_assert (list_item != NULL);

  gtk_list_unselect_child (rlist, list_item);

  g_function_leave ("gtk_list_unselect_item");
}

gint*
gtk_list_get_selected (GtkWidget *list,
		       gint      *nitems)
{
  GtkList *rlist;
  GtkListItem *list_item;
  GList *temp_list;
  gint *selected_items;
  gint n_selected_items;
  gint index, lindex;

  g_function_enter ("gtk_list_get_selected");

  g_assert (list != NULL);
  rlist = (GtkList*) list;

  temp_list = rlist->children;
  n_selected_items = 0;

  while (temp_list)
    {
      list_item = temp_list->data;
      temp_list = temp_list->next;

      if (list_item->state.value == GTK_STATE_SELECTED)
	n_selected_items += 1;
    }

  if (n_selected_items > 0)
    {
      selected_items = g_new (gint, n_selected_items);

      index = 0;
      lindex = 0;
      temp_list = rlist->children;

      while (temp_list)
	{
	  list_item = temp_list->data;
	  temp_list = temp_list->next;

	  if (list_item->state.value == GTK_STATE_SELECTED)
	    selected_items[index++] = lindex;

	  lindex += 1;
	}
    }
  else
    {
      selected_items = NULL;
    }

  g_function_leave ("gtk_list_get_selected");
  return selected_items;
}


guint16
gtk_get_list_type ()
{
  g_function_enter ("gtk_get_list_type");

  if (!list_type)
    gtk_widget_unique_type (&list_type);

  g_function_leave ("gtk_get_list_type");
  return list_type;
}

guint16
gtk_get_list_item_type ()
{
  g_function_enter ("gtk_get_list_item_type");

  if (!list_item_type)
    gtk_widget_unique_type (&list_item_type);

  g_function_leave ("gtk_get_list_item_type");
  return list_item_type;
}


static void
gtk_list_destroy (GtkWidget *widget)
{
  GtkList *list;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_list_destroy");

  list = (GtkList*) widget;
  g_assert (list != NULL);

  children = list->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (!gtk_widget_destroy (child))
	child->parent = NULL;
    }

  g_list_free (list->children);
  gtk_data_detach ((GtkData*) list->hadjustment, &list->adjustment_observer);
  gtk_data_detach ((GtkData*) list->vadjustment, &list->adjustment_observer);
  gtk_data_destroy ((GtkData*) list->hadjustment);
  gtk_data_destroy ((GtkData*) list->vadjustment);

  if (list->view_window)
    gdk_window_destroy (list->view_window);
  if (list->container.widget.window)
    gdk_window_destroy (list->container.widget.window);
  g_free (list);

  g_function_leave ("gtk_list_destroy");
}

static void
gtk_list_map (GtkWidget *widget)
{
  GtkList *list;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_list_map");

  list = (GtkList*) widget;
  g_assert (list != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);
  gdk_window_show (list->view_window);

  children = list->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child) && !GTK_WIDGET_MAPPED (child))
	gtk_widget_map (child);
    }

  g_function_leave ("gtk_list_map");
}

static void
gtk_list_unmap (GtkWidget *widget)
{
  GtkList *list;

  g_function_enter ("gtk_list_unmap");

  list = (GtkList*) widget;
  g_assert (list != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);
  gdk_window_hide (list->view_window);

  g_function_leave ("gtk_list_unmap");
}

static void
gtk_list_realize (GtkWidget *widget)
{
  GtkList *list;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_list_realize");

  list = (GtkList*) widget;
  g_assert (list != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  /* Create the view window.
   */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  list->view_window = gdk_window_new (widget->parent->widget.window,
				      &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (list->view_window, list);

  list->container.widget.style = gtk_style_attach (list->container.widget.style,
						   list->view_window);
  gdk_window_set_background (list->view_window,
			     &list->container.widget.style->background[GTK_STATE_NORMAL]);

  /* Create the list window.
   */
  attributes.x = -list->hadjustment->value;
  attributes.y = -list->vadjustment->value;
  attributes.width = MAX (list->list_width, widget->allocation.width);
  attributes.height = list->list_height;
  list->container.widget.window = gdk_window_new (list->view_window,
						  &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (list->container.widget.window, list);

  gdk_window_set_background (list->container.widget.window,
			     &list->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_list_realize");
}

static void
gtk_list_draw (GtkWidget    *widget,
	       GdkRectangle *area,
	       gint        is_expose)
{
  GtkList *list;
  GtkWidget *child;
  GList *children;
  GdkRectangle child_area;

  g_function_enter ("gtk_list_draw");

  list = (GtkList*) widget;
  g_assert (list != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      children = list->children;

      if (is_expose)
	{
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (GTK_WIDGET_NO_WINDOW (child))
		if (gtk_widget_intersect (child, area, &child_area))
		  gtk_widget_draw (child, &child_area, is_expose);
	    }
	}
      else
	{
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (gtk_widget_intersect (child, area, &child_area))
		gtk_widget_draw (child, &child_area, is_expose);
	    }
	}
    }

  g_function_leave ("gtk_list_draw");
}

static gint
gtk_list_event (GtkWidget *widget,
		GdkEvent  *event)
{
  GtkList *list;
  GtkListItem *list_item;
  GtkWidget *child;
  gfloat new_value;
  GdkModifierType mods;
  gint x, y;

  g_function_enter ("gtk_list_event");

  list = (GtkList*) widget;
  g_assert (list != NULL);
  g_assert (event != NULL);

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_MOTION_NOTIFY:
      gdk_window_get_pointer (widget->window, &x, &y, &mods);
      if (mods & GDK_BUTTON1_MASK)
	{
	  gtk_list_select_update (list, y);

	  gdk_window_get_pointer (list->view_window, &x, &y, NULL);

	  if (y < 0)
	    {
	      new_value = list->vadjustment->value - list->vadjustment->step_increment;
	      if (new_value < list->vadjustment->lower)
		new_value = list->vadjustment->lower;

	      if (new_value != list->vadjustment->value)
		{
		  list->scroll_direction = 0;
		  gtk_list_add_timer (list);

		  list->vadjustment->value = new_value;
		  gtk_data_notify ((GtkData*) list->vadjustment);
		}
	    }
	  else if (y > list->view_window->height)
	    {
	      new_value = list->vadjustment->value + list->vadjustment->step_increment;
	      if (new_value > (list->vadjustment->upper - list->vadjustment->page_size))
		new_value = list->vadjustment->upper - list->vadjustment->page_size;

	      if (new_value != list->vadjustment->value)
		{
		  list->scroll_direction = 1;
		  gtk_list_add_timer (list);

		  list->vadjustment->value = new_value;
		  gtk_data_notify ((GtkData*) list->vadjustment);
		}
	    }
	}
      break;

    case GDK_BUTTON_PRESS:
      child = gtk_get_event_widget (event);
      if (gtk_widget_is_child (widget, child))
	{
	  while (!gtk_widget_is_immediate_child (widget, child))
	    child = (GtkWidget*) child->parent;

	  list_item = (GtkListItem*) child;
	  gtk_list_select_child (list, list_item, event->button.state & GDK_CONTROL_MASK);
	}

      if (list->mode == GTK_SELECTION_EXTENDED)
	{
	  list->have_grab = (gdk_pointer_grab (list->view_window, TRUE,
					       GDK_BUTTON_RELEASE_MASK |
					       GDK_POINTER_MOTION_MASK |
					       GDK_POINTER_MOTION_HINT_MASK,
					       NULL, NULL, event->button.time) == 0);
	  if (list->have_grab)
	    {
	      gtk_grab_add (widget);
	      gdk_window_get_pointer (widget->window, &x, &y, NULL);
	      list->select_start_pos = y;
	      list->select_end_pos = y;
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (list->have_grab)
	{
	  list->have_grab = FALSE;
	  gdk_pointer_ungrab (event->button.time);
	  gtk_grab_remove (widget);
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_list_event");
  return FALSE;
}

static void
gtk_list_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkList *list;
  GtkWidget *child;
  GList *children;
  gint nchildren;

  g_function_enter ("gtk_list_size_request");

  list = (GtkList*) widget;
  g_assert (list != NULL);
  g_assert (requisition != NULL);

  list->list_width = 0;
  list->list_height = 0;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      nchildren = 0;
      children = list->children;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (GTK_WIDGET_VISIBLE (child))
	    {
	      child->requisition.width = 0;
	      child->requisition.height = 0;

	      gtk_widget_size_request (child, &child->requisition);

	      list->list_width = MAX (list->list_width, child->requisition.width);
	      list->list_height += child->requisition.height;
	    }
	}

      list->list_width += list->container.border_width * 2;
      list->list_height += list->container.border_width * 2;

      list->list_width = MAX (list->list_width, 1);
      list->list_height = MAX (list->list_height, 1);
    }

  requisition->width = 1;
  requisition->height = 1;

  g_function_leave ("gtk_list_size_request");
}

static void
gtk_list_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkList *list;
  GtkWidget *child;
  GtkAllocation child_allocation;
  GList *children;
  gfloat h_max_val;
  gfloat v_max_val;
  gfloat h_page_size;
  gfloat v_page_size;

  g_function_enter ("gtk_list_size_allocate");

  list = (GtkList*) widget;
  g_assert (list != NULL);
  g_assert (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (list->view_window,
		       allocation->x,
		       allocation->y);
      gdk_window_set_size (list->view_window,
			   allocation->width,
			   allocation->height);
      gdk_window_set_size (widget->window,
			   MAX (list->list_width, allocation->width),
			   list->list_height);
    }

  if (list->children)
    {
      children = list->children;

      child_allocation.x = list->container.border_width;
      child_allocation.y = list->container.border_width;
      child_allocation.width = widget->window->width - list->container.border_width * 2;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (GTK_WIDGET_VISIBLE (child))
	    {
	      child_allocation.height = child->requisition.height;

	      gtk_widget_size_allocate (child, &child_allocation);

	      child_allocation.y += child_allocation.height;
	    }
	}
    }

  if ((list->list_width != 0) && (list->list_height != 0))
    {
      h_page_size = MIN (list->container.widget.allocation.width, list->list_width);
      v_page_size = MIN (list->container.widget.allocation.height, list->list_height);

      h_max_val = list->list_width;
      v_max_val = list->list_height;

      list->hadjustment->lower = 0;
      list->hadjustment->upper = h_max_val;
      list->hadjustment->step_increment = 5;
      list->hadjustment->page_increment = h_page_size / 2;
      list->hadjustment->page_size = MIN (h_page_size, h_max_val);

      list->vadjustment->lower = 0;
      list->vadjustment->upper = v_max_val;
      list->vadjustment->step_increment = 5;
      list->vadjustment->page_increment = v_page_size / 2;
      list->vadjustment->page_size = MIN (v_page_size, v_max_val);
    }
  else
    {
      list->hadjustment->lower = 0;
      list->hadjustment->upper = 0;
      list->hadjustment->step_increment = 0;
      list->hadjustment->page_increment = 0;
      list->hadjustment->page_size = 0;

      list->vadjustment->lower = 0;
      list->vadjustment->upper = 0;
      list->vadjustment->step_increment = 0;
      list->vadjustment->page_increment = 0;
      list->vadjustment->page_size = 0;
    }

  gtk_data_notify ((GtkData*) list->hadjustment);
  gtk_data_notify ((GtkData*) list->vadjustment);

  g_function_leave ("gtk_list_size_allocate");
}

static gint
gtk_list_is_child (GtkWidget *widget,
		   GtkWidget *child)
{
  GtkList *list;
  GtkWidget *child_widget;
  GList *children;
  gint return_val;

  g_function_enter ("gtk_list_is_child");

  list = (GtkList*) widget;
  g_assert (list != NULL);
  g_assert (child != NULL);

  return_val = FALSE;

  children = list->children;
  while (children)
    {
      child_widget = children->data;
      children = children->next;

      if (child_widget == child)
	{
	  return_val = TRUE;
	  break;
	}
    }

  if (!return_val)
    {
      children = list->children;
      while (children)
	{
	  child_widget = children->data;
	  children = children->next;

	  if (gtk_widget_is_child (child_widget, child))
	    {
	      return_val = TRUE;
	      break;
	    }
	}
    }

  g_function_leave ("gtk_list_is_child");
  return return_val;
}

static gint
gtk_list_locate (GtkWidget  *widget,
		 GtkWidget **child,
		 gint      x,
		 gint      y)
{
  GtkList *list;
  GtkWidget *child_widget;
  GList *children;
  gint return_val;
  gint child_x;
  gint child_y;

  g_function_enter ("gtk_list_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  list = (GtkList*) widget;
  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < MAX (list->list_width, widget->allocation.width)) &&
      (y < list->list_height))
    {
      return_val = TRUE;

      children = list->children;

      while (children)
	{
	  child_widget = children->data;
	  children = children->next;

	  child_x = x - child_widget->allocation.x;
	  child_y = y - child_widget->allocation.y;

	  if (gtk_widget_locate (child_widget, child, child_x, child_y))
	    break;
	}

      if (!(*child))
	*child = widget;
    }

  g_function_leave ("gtk_list_locate");
  return return_val;
}

static void
gtk_list_add (GtkContainer *container,
	      GtkWidget    *widget)
{
  GtkList *list;

  g_function_enter ("gtk_list_add");

  list = (GtkList*) container;
  g_assert (list != NULL);
  g_assert (widget != NULL);

  list->children = g_list_append (list->children, widget);

  if ((list->mode == GTK_SELECTION_BROWSE) && !list->selection.list)
    gtk_list_select_child (list, (GtkListItem*) widget, FALSE);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_list_add");
}

static void
gtk_list_remove (GtkContainer *container,
		 GtkWidget    *widget)
{
  GtkList *list;

  g_function_enter ("gtk_list_remove");

  list = (GtkList*) container;
  g_assert (list != NULL);
  g_assert (widget != NULL);

  list->children = g_list_remove (list->children, widget);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_list_remove");
}

static void
gtk_list_foreach (GtkContainer *container,
		  GtkCallback   callback,
		  gpointer      callback_data)
{
  GtkList *list;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_list_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  list = (GtkList*) container;
  children = list->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data, NULL);
    }

  g_function_leave ("gtk_list_foreach");
}

static void
gtk_list_select_child (GtkList     *list,
		       GtkListItem *list_item,
		       gint         toggle)
{
  GList *selection;
  GList *temp_list;
  GtkListItem *temp_item;

  g_function_enter ("gtk_list_select_child");

  g_assert (list != NULL);
  g_assert (list_item != NULL);

  switch (list->mode)
    {
    case GTK_SELECTION_SINGLE:
      selection = list->selection.list;

      while (selection)
	{
	  if (list_item != (GtkListItem*) selection->data)
	    {
	      temp_item = selection->data;
	      temp_item->state.value = GTK_STATE_NORMAL;
	      gtk_data_notify ((GtkData*) &temp_item->state);

	      temp_list = selection;
	      selection = selection->next;

	      if (temp_list->next)
		temp_list->next->prev = temp_list->prev;
	      if (temp_list->prev)
		temp_list->prev->next = temp_list->next;

	      if (temp_list == list->selection.list)
		list->selection.list = selection;

	      temp_list->next = NULL;
	      temp_list->prev = NULL;
	      g_list_free (temp_list);
	    }
	  else
	    selection = selection->next;
	}

      if (list_item->state.value == GTK_STATE_NORMAL)
	{
	  list->selection.list = g_list_prepend (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_SELECTED;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      else if (list_item->state.value == GTK_STATE_SELECTED)
	{
	  list->selection.list = g_list_remove (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      break;

    case GTK_SELECTION_BROWSE:
      selection = list->selection.list;

      while (selection)
	{
	  if (list_item != (GtkListItem*) selection->data)
	    {
	      temp_item = selection->data;
	      temp_item->state.value = GTK_STATE_NORMAL;
	      gtk_data_notify ((GtkData*) &temp_item->state);

	      temp_list = selection;
	      selection = selection->next;

	      if (temp_list->next)
		temp_list->next->prev = temp_list->prev;
	      if (temp_list->prev)
		temp_list->prev->next = temp_list->next;

	      if (temp_list == list->selection.list)
		list->selection.list = selection;

	      temp_list->next = NULL;
	      temp_list->prev = NULL;
	      g_list_free (temp_list);
	    }
	  else
	    selection = selection->next;
	}

      if (list_item->state.value == GTK_STATE_NORMAL)
	{
	  list->selection.list = g_list_prepend (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_SELECTED;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      break;

    case GTK_SELECTION_MULTIPLE:
      if (list_item->state.value == GTK_STATE_NORMAL)
	{
	  list->selection.list = g_list_prepend (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_SELECTED;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      else if (list_item->state.value == GTK_STATE_SELECTED)
	{
	  list->selection.list = g_list_remove (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      break;

    case GTK_SELECTION_EXTENDED:
      selection = list->selection.list;

      if (!toggle)
	while (selection)
	  {
	    if (list_item != (GtkListItem*) selection->data)
	      {
		temp_item = selection->data;
		temp_item->state.value = GTK_STATE_NORMAL;
		gtk_data_notify ((GtkData*) &temp_item->state);

		temp_list = selection;
		selection = selection->next;

		if (temp_list->next)
		  temp_list->next->prev = temp_list->prev;
		if (temp_list->prev)
		  temp_list->prev->next = temp_list->next;

		if (temp_list == list->selection.list)
		  list->selection.list = selection;

		temp_list->next = NULL;
		temp_list->prev = NULL;
		g_list_free (temp_list);
	      }
	    else
	      selection = selection->next;
	  }

      if (toggle && (list_item->state.value == GTK_STATE_SELECTED))
	{
	  list->selection.list = g_list_remove (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      else if (list_item->state.value == GTK_STATE_NORMAL)
	{
	  list->selection.list = g_list_prepend (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_SELECTED;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      break;
    }

  g_function_leave ("gtk_list_select_child");
}

static void
gtk_list_select_update (GtkList *list,
			gint     new_end_pos)
{
  g_function_enter ("gtk_list_select_update");

  g_assert (list != NULL);

  if (new_end_pos < 0)
    new_end_pos = 0;
  else if (new_end_pos > list->list_height)
    new_end_pos = list->list_height;

  list->select_end_pos = new_end_pos;

  g_function_leave ("gtk_list_select_update");
}

static void
gtk_list_unselect_child (GtkList     *list,
			 GtkListItem *list_item)
{
  GList *selection;
  GList *temp_list;
  GtkListItem *temp_item;

  g_function_enter ("gtk_list_unselect_child");

  g_assert (list != NULL);
  g_assert (list_item != NULL);

  switch (list->mode)
    {
    case GTK_SELECTION_SINGLE:
    case GTK_SELECTION_MULTIPLE:
      if (list_item->state.value == GTK_STATE_SELECTED)
	{
	  list->selection.list = g_list_remove (list->selection.list, list_item);

	  list_item->state.value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) &list_item->state);

	  gtk_data_notify ((GtkData*) &list->selection);
	}
      break;

    case GTK_SELECTION_BROWSE:
      selection = list->selection.list;

      while (selection)
	{
	  if (list_item != (GtkListItem*) selection->data)
	    {
	      temp_item = selection->data;
	      temp_item->state.value = GTK_STATE_NORMAL;
	      gtk_data_notify ((GtkData*) &temp_item->state);

	      temp_list = selection;
	      selection = selection->next;

	      if (temp_list->next)
		temp_list->next->prev = temp_list->prev;
	      if (temp_list->prev)
		temp_list->prev->next = temp_list->next;

	      if (temp_list == list->selection.list)
		list->selection.list = selection;

	      temp_list->next = NULL;
	      temp_list->prev = NULL;
	      g_list_free (temp_list);
	    }
	  else
	    selection = selection->next;
	}
      break;

    case GTK_SELECTION_EXTENDED:
      break;
    }

  g_function_leave ("gtk_list_unselect_child");
}

static void
gtk_list_unselect_all (GtkList *list)
{
  GList *selection;
  GtkListItem *list_item;

  g_function_enter ("gtk_list_unselect_all");

  g_assert (list != NULL);

  if (list->selection.list)
    {
      selection = list->selection.list;

      while (selection)
	{
	  list_item = selection->data;
	  selection = selection->next;

	  list_item->state.value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) &list_item->state);
	}

      g_list_free (list->selection.list);
      list->selection.list = NULL;
      gtk_data_notify ((GtkData*) &list->selection);
    }

  g_function_leave ("gtk_list_unselect_all");
}

static void
gtk_list_add_timer (GtkList *list)
{
  g_function_enter ("gtk_list_add_timer");

  g_assert (list != NULL);

  if (!list->timer)
    list->timer = gtk_timeout_add (LIST_TIMER_LENGTH, gtk_list_timer, (gpointer) list);

  g_function_leave ("gtk_list_add_timer");
}

static void
gtk_list_remove_timer (GtkList *list)
{
  g_function_enter ("gtk_list_remove_timer");

  g_assert (list != NULL);

  if (list->timer)
    {
      gtk_timeout_remove (list->timer);
      list->timer = 0;
    }

  g_function_leave ("gtk_list_remove_timer");
}

static gint
gtk_list_timer (gpointer data)
{
  GtkList *list;
  gfloat new_value;
  gint return_val;

  g_function_enter ("gtk_list_timer");

  g_assert (data != NULL);
  list = (GtkList*) data;

  if (list->scroll_direction == 0)
    {
      new_value = list->vadjustment->value - list->vadjustment->step_increment;
      if (new_value < list->vadjustment->lower)
	new_value = list->vadjustment->lower;
    }
  else
    {
      new_value = list->vadjustment->value + list->vadjustment->step_increment;
      if (new_value > (list->vadjustment->upper - list->vadjustment->page_size))
	new_value = list->vadjustment->upper - list->vadjustment->page_size;
    }

  if (new_value != list->vadjustment->value)
    {
      return_val = TRUE;
      list->vadjustment->value = new_value;
      gtk_data_notify ((GtkData*) list->vadjustment);
    }
  else
    {
      return_val = FALSE;
      list->timer = 0;
    }

  g_function_leave ("gtk_list_timer");
  return return_val;
}

static gint
gtk_list_adjustment_update (GtkObserver *observer,
			    GtkData     *data)
{
  GtkList *list;

  g_function_enter ("gtk_list_adjustment_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  list = observer->user_data;
  g_assert (list != NULL);

  gdk_window_move (list->container.widget.window,
		   -list->hadjustment->value,
		   -list->vadjustment->value);

  g_function_leave ("gtk_list_adjustment_update");
  return FALSE;
}

static void
gtk_list_adjustment_disconnect (GtkObserver *observer,
				GtkData     *data)
{
  g_function_enter ("gtk_list_adjustment_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_list_adjustment_disconnect");
}


static void
gtk_list_item_destroy (GtkWidget *widget)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_destroy");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);

  if (list_item->child)
    if (!gtk_widget_destroy (list_item->child))
      list_item->child->parent = NULL;
  if (list_item->container.widget.window)
    gdk_window_destroy (list_item->container.widget.window);
  g_free (list_item);

  g_function_leave ("gtk_list_item_destroy");
}

static void
gtk_list_item_map (GtkWidget *widget)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_map");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);

  if (list_item->child &&
      GTK_WIDGET_VISIBLE (list_item->child) &&
      !GTK_WIDGET_MAPPED (list_item->child))
    gtk_widget_map (list_item->child);

  g_function_leave ("gtk_list_item_map");
}

static void
gtk_list_item_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_list_item_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_list_item_unmap");
}

static void
gtk_list_item_realize (GtkWidget *widget)
{
  GtkListItem *list_item;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_list_item_realize");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK);

  list_item->container.widget.window =
    gdk_window_new (widget->parent->widget.window,
		    &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (list_item->container.widget.window, list_item);

  list_item->container.widget.style =
    gtk_style_attach (list_item->container.widget.style,
		      list_item->container.widget.window);
  gdk_window_set_background (list_item->container.widget.window,
			     &list_item->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_list_item_realize");
}

static void
gtk_list_item_draw (GtkWidget    *widget,
		    GdkRectangle *area,
		    gint        is_expose)
{
  GtkListItem *list_item;
  GdkRectangle child_area;

  g_function_enter ("gtk_list_item_draw");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      gdk_window_set_background (widget->window,
				 &widget->style->background[list_item->state.value]);
      gdk_window_clear (widget->window);

      if (list_item->child)
	{
	  if (GTK_WIDGET_NO_WINDOW (list_item->child))
	    gtk_widget_draw (list_item->child, NULL, is_expose);
	  else if (!is_expose)
	    if (gtk_widget_intersect (list_item->child, area, &child_area))
	      gtk_widget_draw (list_item->child, &child_area, is_expose);
	}
    }

  g_function_leave ("gtk_list_item_draw");
}

static gint
gtk_list_item_event (GtkWidget *widget,
		     GdkEvent  *event)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_event");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_list_item_event");
  return FALSE;
}

static void
gtk_list_item_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkListItem *list_item;
  gint shadow_thickness;

  g_function_enter ("gtk_list_item_size_request");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);
  g_assert (requisition != NULL);

  shadow_thickness = widget->style->shadow_thickness;

  if (GTK_WIDGET_VISIBLE (widget) && list_item->child)
    {
      list_item->child->requisition.width = 0;
      list_item->child->requisition.height = 0;

      gtk_widget_size_request (list_item->child, &list_item->child->requisition);

      requisition->width = (list_item->child->requisition.width +
			    list_item->container.border_width * 2 +
			    shadow_thickness * 2 + 0);
      requisition->height = (list_item->child->requisition.height +
			     list_item->container.border_width * 2 +
			     shadow_thickness * 2 + 0);
    }
  else
    {
      requisition->width = 1;
      requisition->height = shadow_thickness;
    }

  g_function_leave ("gtk_list_item_size_request");
}

static void
gtk_list_item_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkListItem *list_item;
  GtkAllocation child_allocation;
  gint shadow_thickness;

  g_function_enter ("gtk_list_item_size_allocate");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);
  g_assert (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (widget->window,
                       allocation->x,
                       allocation->y);
      gdk_window_set_size (widget->window,
                           allocation->width,
                           allocation->height);
    }

  if (list_item->child)
    {
      shadow_thickness = widget->style->shadow_thickness;
      child_allocation.x = list_item->container.border_width + shadow_thickness + 0;
      child_allocation.y = list_item->container.border_width + shadow_thickness + 0;
      child_allocation.width = allocation->width - child_allocation.x * 2;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (list_item->child, &child_allocation);
    }

  g_function_leave ("gtk_list_item_size_allocate");
}

static gint
gtk_list_item_is_child (GtkWidget *widget,
			GtkWidget *child)
{
  GtkListItem *list_item;
  gint return_val;

  g_function_enter ("gtk_list_item_is_child");

  list_item = (GtkListItem*) widget;
  g_assert (list_item != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  if (list_item->child == child)
    return_val = TRUE;
  else if (list_item->child)
    return_val = gtk_widget_is_child (list_item->child, child);

  g_function_leave ("gtk_list_item_is_child");
  return return_val;
}

static gint
gtk_list_item_locate (GtkWidget  *widget,
		      GtkWidget **child,
		      gint      x,
		      gint      y)
{
  GtkListItem *list_item;
  gint return_val;
  gint child_x;
  gint child_y;

  g_function_enter ("gtk_list_item_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < widget->allocation.width) &&
      (y < widget->allocation.height))
    {
      return_val = TRUE;

      list_item = (GtkListItem*) widget;
      if (list_item->child)
	{
	  child_x = x - list_item->child->allocation.x;
	  child_y = y - list_item->child->allocation.y;

	  gtk_widget_locate (list_item->child, child, child_x, child_y);
	}

      if (!(*child))
	*child = widget;
    }

  g_function_leave ("gtk_list_item_locate");
  return return_val;
}

static void
gtk_list_item_activate (GtkWidget *widget)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_activate");

  g_assert (widget != NULL);
  list_item = (GtkListItem*) widget;

  if (list_item->state.value == GTK_STATE_SELECTED)
    list_item->state.value = GTK_STATE_NORMAL;
  else if (list_item->state.value == GTK_STATE_NORMAL)
    list_item->state.value = GTK_STATE_SELECTED;
  gtk_data_notify ((GtkData*) &list_item->state);

  g_function_leave ("gtk_list_item_activate");
}

static void
gtk_list_item_set_state (GtkWidget    *widget,
			 GtkStateType  state)
{
  g_function_enter ("gtk_list_item_set_state");
  g_warning ("gtk_list_item_set_state: UNFINISHED");
  g_function_leave ("gtk_list_item_set_state");
}

static gint
gtk_list_item_install_accelerator (GtkWidget *widget,
				   gchar      accelerator_key,
				   guint8     accelerator_mods)
{
  g_function_enter ("gtk_list_item_install_accelerator");
  g_assert (widget != NULL);
  g_function_leave ("gtk_list_item_install_accelerator");
  return TRUE;
}

static void
gtk_list_item_add (GtkContainer *container,
		   GtkWidget    *widget)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_add");

  list_item = (GtkListItem*) container;
  g_assert (list_item != NULL);
  g_assert (widget != NULL);

  if (list_item->child)
    g_error ("list item already has child");
  else
    list_item->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_list_item_add");
}

static void
gtk_list_item_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_remove");

  list_item = (GtkListItem*) container;
  g_assert (list_item != NULL);
  g_assert (widget != NULL);

  if (list_item->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!list_item->child)
    g_error ("menu item has no child to remove");

  list_item->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_list_item_remove");
}

static void
gtk_list_item_foreach (GtkContainer *container,
		       GtkCallback   callback,
		       gpointer      callback_data)
{
  GtkListItem *list_item;

  g_function_enter ("gtk_list_item_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  list_item = (GtkListItem*) container;

  if (list_item->child)
    (* callback) (list_item->child, callback_data, NULL);

  g_function_leave ("gtk_list_item_foreach");
}

static gint
gtk_list_item_state_update (GtkObserver *observer,
			    GtkData     *data)
{
  GtkListItem *list_item;
  g_function_enter ("gtk_list_item_state_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  list_item = (GtkListItem*) observer->user_data;
  g_assert (list_item != NULL);

  gtk_widget_draw ((GtkWidget*) list_item, NULL, FALSE);

  g_function_leave ("gtk_list_item_state_update");
  return FALSE;
}

static void
gtk_list_item_state_disconnect (GtkObserver *observer,
				GtkData     *data)
{
  g_function_enter ("gtk_list_item_state_disconnect");
  g_function_leave ("gtk_list_item_state_disconnect");
}
