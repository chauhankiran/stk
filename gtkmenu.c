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
#include <stdio.h>
#include <string.h>

#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkframe.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkmisc.h"
#include "gtkoptionmenu.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkwindow.h"
#include "gtkprivate.h"
#include "gtkmenuprivate.h"


static void  gtk_menu_destroy       (GtkWidget       *widget);
static void  gtk_menu_show          (GtkWidget       *widget);
static void  gtk_menu_hide          (GtkWidget       *widget);
static void  gtk_menu_map           (GtkWidget       *widget);
static void  gtk_menu_unmap         (GtkWidget       *widget);
static void  gtk_menu_realize       (GtkWidget       *widget);
static void  gtk_menu_draw          (GtkWidget       *widget,
				     GdkRectangle    *area,
				     gint             is_expose);
static gint  gtk_menu_event         (GtkWidget       *widget,
				     GdkEvent        *event);
static void  gtk_menu_size_request  (GtkWidget       *widget,
				     GtkRequisition  *requisition);
static void  gtk_menu_size_allocate (GtkWidget       *widget,
				     GtkAllocation   *allocation);
static gint  gtk_menu_is_child      (GtkWidget       *widget,
				     GtkWidget       *child);
static gint  gtk_menu_locate        (GtkWidget       *widget,
				     GtkWidget      **child,
				     gint             x,
				     gint             y);
static void  gtk_menu_add           (GtkContainer    *container,
				     GtkWidget       *widget);
static void  gtk_menu_remove        (GtkContainer    *container,
				     GtkWidget       *widget);
static void  gtk_menu_foreach       (GtkContainer   *container,
				     GtkCallback     callback,
				     gpointer        callback_data);
static void  gtk_menu_window_resize (GtkWidget       *widget,
				     gint            *x,
				     gint            *y,
				     gint            *width,
				     gint            *height);

static void  gtk_menu_bar_destroy       (GtkWidget       *widget);
static void  gtk_menu_bar_map           (GtkWidget       *widget);
static void  gtk_menu_bar_unmap         (GtkWidget       *widget);
static void  gtk_menu_bar_realize       (GtkWidget       *widget);
static void  gtk_menu_bar_draw          (GtkWidget       *widget,
					 GdkRectangle    *area,
					 gint             is_expose);
static gint  gtk_menu_bar_event         (GtkWidget       *widget,
					 GdkEvent        *event);
static void  gtk_menu_bar_size_request  (GtkWidget       *widget,
					 GtkRequisition  *requisition);
static void  gtk_menu_bar_size_allocate (GtkWidget       *widget,
					 GtkAllocation   *allocation);
static gint  gtk_menu_bar_is_child      (GtkWidget       *widget,
					 GtkWidget       *child);
static gint  gtk_menu_bar_locate        (GtkWidget       *widget,
					 GtkWidget      **child,
					 gint             x,
					 gint             y);
static void  gtk_menu_bar_add           (GtkContainer    *container,
					 GtkWidget       *widget);
static void  gtk_menu_bar_remove        (GtkContainer    *container,
					 GtkWidget       *widget);
static void  gtk_menu_bar_foreach       (GtkContainer   *container,
					 GtkCallback     callback,
					 gpointer        callback_data);

static void  gtk_menu_item_destroy             (GtkWidget       *widget);
static void  gtk_menu_item_map                 (GtkWidget       *widget);
static void  gtk_menu_item_unmap               (GtkWidget       *widget);
static void  gtk_menu_item_realize             (GtkWidget       *widget);
static void  gtk_menu_item_draw                (GtkWidget       *widget,
						GdkRectangle    *area,
						gint             is_expose);
static void  gtk_menu_item_expose              (GtkWidget       *widget);
static gint  gtk_menu_item_event               (GtkWidget       *widget,
						GdkEvent        *event);
static void  gtk_menu_item_size_request        (GtkWidget       *widget,
						GtkRequisition  *requisition);
static void  gtk_menu_item_size_allocate       (GtkWidget       *widget,
						GtkAllocation   *allocation);
static gint  gtk_menu_item_is_child            (GtkWidget       *widget,
						GtkWidget       *child);
static gint  gtk_menu_item_locate              (GtkWidget       *widget,
						GtkWidget      **child,
						gint             x,
						gint             y);
static void  gtk_menu_item_activate            (GtkWidget       *widget);
static gint  gtk_menu_item_install_accelerator (GtkWidget       *widget,
						gchar            accelerator_key,
						guint8           accelerator_mods);
static void  gtk_menu_item_remove_accelerator  (GtkWidget       *widget);
static void  gtk_menu_item_add                 (GtkContainer    *container,
						GtkWidget       *widget);
static void  gtk_menu_item_remove              (GtkContainer    *container,
						GtkWidget       *widget);
static void  gtk_menu_item_foreach             (GtkContainer   *container,
						GtkCallback     callback,
						gpointer        callback_data);

static void  gtk_menu_toggle_item_destroy      (GtkWidget    *widget);
static void  gtk_menu_toggle_item_draw         (GtkWidget    *widget,
						GdkRectangle *area,
						gint          is_expose);
static void  gtk_menu_toggle_item_expose       (GtkWidget    *widget);
static void  gtk_menu_toggle_item_activate     (GtkWidget    *widget);


static void  gtk_menu_item_select   (GtkWidget *widget);
static void  gtk_menu_item_deselect (GtkWidget *widget);

static gint  gtk_menu_item_state_update     (GtkObserver *observer,
					     GtkData     *data);
static void  gtk_menu_item_state_disconnect (GtkObserver *observer,
					     GtkData     *data);

static gint  gtk_menu_toggle_item_owner_update     (GtkObserver *observer,
						    GtkData     *data);
static void  gtk_menu_toggle_item_owner_disconnect (GtkObserver *observer,
						    GtkData     *data);

static void  gtk_menu_item_calc_accelerator_size (GtkMenuItem *menu_item);
static void  gtk_menu_item_calc_accelerator_text (GtkMenuItem *menu_item,
						  gchar       *buffer);


static GtkWidgetFunctions menu_widget_functions =
{
  gtk_menu_destroy,
  gtk_menu_show,
  gtk_menu_hide,
  gtk_menu_map,
  gtk_menu_unmap,
  gtk_menu_realize,
  gtk_menu_draw,
  gtk_widget_default_draw_focus,
  gtk_menu_event,
  gtk_menu_size_request,
  gtk_menu_size_allocate,
  gtk_menu_is_child,
  gtk_menu_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions menu_bar_widget_functions =
{
  gtk_menu_bar_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_menu_bar_map,
  gtk_menu_bar_unmap,
  gtk_menu_bar_realize,
  gtk_menu_bar_draw,
  gtk_widget_default_draw_focus,
  gtk_menu_bar_event,
  gtk_menu_bar_size_request,
  gtk_menu_bar_size_allocate,
  gtk_menu_bar_is_child,
  gtk_menu_bar_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions menu_item_widget_functions =
{
  gtk_menu_item_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_menu_item_map,
  gtk_menu_item_unmap,
  gtk_menu_item_realize,
  gtk_menu_item_draw,
  gtk_widget_default_draw_focus,
  gtk_menu_item_event,
  gtk_menu_item_size_request,
  gtk_menu_item_size_allocate,
  gtk_menu_item_is_child,
  gtk_menu_item_locate,
  gtk_menu_item_activate,
  gtk_widget_default_set_state,
  gtk_menu_item_install_accelerator,
  gtk_menu_item_remove_accelerator,
};

static GtkWidgetFunctions menu_toggle_item_widget_functions =
{
  gtk_menu_toggle_item_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_menu_item_map,
  gtk_menu_item_unmap,
  gtk_menu_item_realize,
  gtk_menu_toggle_item_draw,
  gtk_widget_default_draw_focus,
  gtk_menu_item_event,
  gtk_menu_item_size_request,
  gtk_menu_item_size_allocate,
  gtk_menu_item_is_child,
  gtk_menu_item_locate,
  gtk_menu_toggle_item_activate,
  gtk_widget_default_set_state,
  gtk_menu_item_install_accelerator,
  gtk_menu_item_remove_accelerator,
};

static GtkContainerFunctions menu_container_functions =
{
  gtk_menu_add,
  gtk_menu_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_menu_foreach,
};

static GtkContainerFunctions menu_bar_container_functions =
{
  gtk_menu_bar_add,
  gtk_menu_bar_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_menu_bar_foreach,
};

static GtkContainerFunctions menu_item_container_functions =
{
  gtk_menu_item_add,
  gtk_menu_item_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_menu_item_foreach,
};


GtkWidget*
gtk_menu_new ()
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_new");

  menu = g_new (GtkMenu, 1);

  menu->window = gtk_window_new ("menu window", GTK_WINDOW_POPUP);
  gtk_widget_set_user_data (menu->window, menu);
  gtk_window_set_resize_hook (menu->window, gtk_menu_window_resize);

  menu->frame = gtk_frame_new (NULL);
  gtk_container_add (menu->window, menu->frame);
  gtk_container_set_border_width (menu->frame, 0);
  gtk_frame_set_type (menu->frame, GTK_SHADOW_OUT);
  gtk_widget_show (menu->frame);

  menu->container.widget.type = gtk_get_menu_type ();;
  menu->container.widget.function_table = &menu_widget_functions;
  menu->container.function_table = &menu_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) menu);
  gtk_container_set_defaults ((GtkWidget*) menu);

  menu->children = NULL;
  menu->parent = NULL;
  menu->old_active_menu_item = NULL;
  menu->active_menu_item = NULL;

  gtk_container_add (menu->frame, (GtkWidget*) menu);

  g_function_leave ("gtk_menu_new");
  return ((GtkWidget*) menu);
}

GtkWidget*
gtk_menu_bar_new ()
{
  GtkMenuBar *menu_bar;

  g_function_enter ("gtk_menu_bar_new");

  menu_bar = g_new (GtkMenuBar, 1);

  menu_bar->container.widget.type = gtk_get_menu_bar_type ();
  menu_bar->container.widget.function_table = &menu_bar_widget_functions;
  menu_bar->container.function_table = &menu_bar_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) menu_bar);
  gtk_container_set_defaults ((GtkWidget*) menu_bar);

  menu_bar->children = NULL;
  menu_bar->active_menu_item = NULL;
  menu_bar->active = FALSE;

  g_function_leave ("gtk_menu_bar_new");
  return ((GtkWidget*) menu_bar);
}

GtkWidget*
gtk_menu_item_new ()
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_new");

  menu_item = g_new (GtkMenuItem, 1);

  menu_item->container.widget.type = gtk_get_menu_item_type ();
  menu_item->container.widget.function_table = &menu_item_widget_functions;
  menu_item->container.function_table = &menu_item_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) menu_item);
  gtk_container_set_defaults ((GtkWidget*) menu_item);

  menu_item->child = NULL;
  menu_item->submenu = NULL;
  menu_item->accelerator_size = 0;
  menu_item->accelerator_key = '\0';
  menu_item->accelerator_mods = 0;
  menu_item->submenu_direction = DIRECTION_RIGHT;
  menu_item->toggle_exists = FALSE;
  menu_item->toggle_draw = FALSE;
  menu_item->previous_state = GTK_STATE_NORMAL;

  gtk_data_init ((GtkData*) &menu_item->state);
  menu_item->state.data.type = gtk_data_int_type ();
  menu_item->state.value = GTK_STATE_NORMAL;

  menu_item->state_observer.update = gtk_menu_item_state_update;
  menu_item->state_observer.disconnect = gtk_menu_item_state_disconnect;
  menu_item->state_observer.user_data = menu_item;

  gtk_data_attach ((GtkData*) &menu_item->state, &menu_item->state_observer);

  g_function_leave ("gtk_menu_item_new");
  return ((GtkWidget*) menu_item);
}

GtkWidget*
gtk_menu_toggle_item_new (GtkData *owner)
{
  GtkMenuToggleItem *toggle_item;

  g_function_enter ("gtk_menu_toggle_item_new");

  toggle_item = g_new (GtkMenuToggleItem, 1);

  toggle_item->menu_item.container.widget.type = gtk_get_menu_toggle_item_type ();
  toggle_item->menu_item.container.widget.function_table = &menu_toggle_item_widget_functions;
  toggle_item->menu_item.container.function_table = &menu_item_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) toggle_item);
  gtk_container_set_defaults ((GtkWidget*) toggle_item);

  toggle_item->menu_item.child = NULL;
  toggle_item->menu_item.submenu = NULL;
  toggle_item->menu_item.accelerator_size = 0;
  toggle_item->menu_item.accelerator_key = '\0';
  toggle_item->menu_item.accelerator_mods = 0;
  toggle_item->menu_item.submenu_direction = DIRECTION_RIGHT;
  toggle_item->menu_item.toggle_exists = TRUE;
  toggle_item->menu_item.toggle_draw = FALSE;
  toggle_item->menu_item.previous_state = GTK_STATE_NORMAL;

  toggle_item->menu_item.state.value = GTK_STATE_NORMAL;
  toggle_item->menu_item.state.data.type = gtk_data_int_type ();
  toggle_item->menu_item.state.data.observers = NULL;

  toggle_item->menu_item.state_observer.update = gtk_menu_item_state_update;
  toggle_item->menu_item.state_observer.disconnect = gtk_menu_item_state_disconnect;
  toggle_item->menu_item.state_observer.user_data = toggle_item;

  gtk_data_attach ((GtkData*) &toggle_item->menu_item.state, &toggle_item->menu_item.state_observer);

  if (!owner)
    {
      owner = gtk_data_widget_new ((GtkWidget*) toggle_item);

      toggle_item->menu_item.previous_state = GTK_STATE_ACTIVE;
      toggle_item->menu_item.state.value = GTK_STATE_ACTIVE;
    }

  toggle_item->owner = (GtkDataWidget*) owner;
  toggle_item->owner_observer.update = gtk_menu_toggle_item_owner_update;
  toggle_item->owner_observer.disconnect = gtk_menu_toggle_item_owner_disconnect;
  toggle_item->owner_observer.user_data = toggle_item;

  gtk_data_attach ((GtkData*) toggle_item->owner, &toggle_item->owner_observer);

  g_function_leave ("gtk_menu_toggle_item_new");
  return ((GtkWidget*) toggle_item);
}

GtkWidget*
gtk_menu_item_new_with_label (gchar *label)
{
  GtkWidget *menu_item;
  GtkWidget *label_widget;

  g_function_enter ("gtk_menu_item_new_with_label");

  g_assert (label != NULL);

  menu_item = gtk_menu_item_new ();
  label_widget = gtk_label_new (label);
  gtk_container_add (menu_item, label_widget);

  gtk_label_set_alignment (label_widget, 0.0, 0.5);
  gtk_widget_show (label_widget);

  g_function_leave ("gtk_menu_item_new_with_label");
  return menu_item;
}

GtkWidget*
gtk_menu_item_new_with_image (GdkImage *image)
{
  GtkWidget *menu_item;
  GtkWidget *image_widget;

  g_function_enter ("gtk_menu_item_new_with_image");

  g_assert (image != NULL);

  menu_item = gtk_menu_item_new ();
  image_widget = gtk_image_new (image);
  gtk_container_add (menu_item, image_widget);

  gtk_image_set_alignment (image_widget, 0.0, 0.5);
  gtk_widget_show (image_widget);

  g_function_leave ("gtk_menu_item_new_with_image");
  return menu_item;
}

GtkWidget*
gtk_menu_item_new_with_pixmap (GdkPixmap *pixmap)
{
  GtkWidget *menu_item;
  GtkWidget *pixmap_widget;

  g_function_enter ("gtk_menu_item_new_with_pixmap");

  g_assert (pixmap != NULL);

  menu_item = gtk_menu_item_new ();
  pixmap_widget = gtk_pixmap_new (pixmap, NULL, NULL);
  gtk_container_add (menu_item, pixmap_widget);

  gtk_pixmap_set_alignment (pixmap_widget, 0.0, 0.5);
  gtk_widget_show (pixmap_widget);

  g_function_leave ("gtk_menu_item_new_with_pixmap");
  return menu_item;
}

GtkWidget*
gtk_menu_toggle_item_new_with_label (GtkData *owner,
				     gchar   *label)
{
  GtkWidget *menu_item;
  GtkWidget *label_widget;

  g_function_enter ("gtk_menu_toggle_item_new_with_label");

  g_assert (label != NULL);

  menu_item = gtk_menu_toggle_item_new (owner);
  label_widget = gtk_label_new (label);
  gtk_container_add (menu_item, label_widget);

  gtk_label_set_alignment (label_widget, 0.0, 0.5);
  gtk_widget_show (label_widget);

  g_function_leave ("gtk_menu_toggle_item_new_with_label");
  return menu_item;
}

GtkWidget*
gtk_menu_toggle_item_new_with_image (GtkData  *owner,
				     GdkImage *image)
{
  GtkWidget *menu_item;
  GtkWidget *image_widget;

  g_function_enter ("gtk_menu_toggle_item_new_with_image");

  g_assert (image != NULL);

  menu_item = gtk_menu_toggle_item_new (owner);
  image_widget = gtk_image_new (image);
  gtk_container_add (menu_item, image_widget);

  gtk_image_set_alignment (image_widget, 0.0, 0.5);
  gtk_widget_show (image_widget);

  g_function_leave ("gtk_menu_toggle_item_new_with_image");
  return menu_item;
}

GtkWidget*
gtk_menu_toggle_item_new_with_pixmap (GtkData   *owner,
				      GdkPixmap *pixmap)
{
  GtkWidget *menu_item;
  GtkWidget *pixmap_widget;

  g_function_enter ("gtk_menu_toggle_item_new_with_pixmap");

  g_assert (pixmap != NULL);

  menu_item = gtk_menu_toggle_item_new (owner);
  pixmap_widget = gtk_pixmap_new (pixmap, NULL, NULL);
  gtk_container_add (menu_item, pixmap_widget);

  gtk_pixmap_set_alignment (pixmap_widget, 0.0, 0.5);
  gtk_widget_show (pixmap_widget);

  g_function_leave ("gtk_menu_toggle_item_new_with_pixmap");
  return menu_item;
}

void
gtk_menu_item_set_submenu (GtkWidget *widget,
			   GtkWidget *submenu)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_set_submenu");

  g_assert (widget != NULL);
  g_assert (submenu != NULL);

  if (GTK_WIDGET_TYPE (widget) != gtk_get_menu_item_type ())
    g_error ("widget is not a menu item");

  if (GTK_WIDGET_TYPE (submenu) != gtk_get_menu_type ())
    g_error ("widget is not a menu");

  menu_item = (GtkMenuItem*) widget;
  if (menu_item->state.value == GTK_STATE_SELECTED)
    g_error ("cannot change menu item submenu while item is active");

  menu_item->submenu = submenu;

  if (widget->parent)
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_menu_item_set_submenu");
}

GtkData*
gtk_menu_item_get_state (GtkWidget *menu_item)
{
  GtkMenuItem *rmenu_item;
  GtkData *data;

  g_function_enter ("gtk_menu_item_get_state");

  g_assert (menu_item != NULL);

  rmenu_item = (GtkMenuItem*) menu_item;
  data = (GtkData*) &rmenu_item->state;

  g_function_leave ("gtk_menu_item_get_state");
  return data;
}

GtkData*
gtk_menu_toggle_item_get_owner (GtkWidget *menu_item)
{
  GtkMenuToggleItem *rmenu_item;
  GtkData *data;

  g_function_enter ("gtk_menu_toggle_item_get_owner");

  g_assert (menu_item != NULL);

  rmenu_item = (GtkMenuToggleItem*) menu_item;
  data = (GtkData*) rmenu_item->owner;

  g_function_leave ("gtk_menu_toggle_item_get_owner");
  return data;
}

GList*
gtk_menu_get_children (GtkWidget *menu)
{
  GtkMenu *rmenu;
  GList *children;

  g_function_enter ("gtk_menu_get_children");

  g_assert (menu != NULL);
  g_assert (menu->type == gtk_get_menu_type ());

  rmenu = (GtkMenu*) menu;
  children = rmenu->children;

  g_function_leave ("gtk_menu_get_children");
  return children;
}

GtkWidget*
gtk_menu_get_active (GtkWidget *menu)
{
  GtkMenu *rmenu;
  GtkMenuItem *active;
  GList *list;

  g_function_enter ("gtk_menu_get_active");

  g_assert (menu != NULL);
  g_assert (menu->type == gtk_get_menu_type ());

  rmenu = (GtkMenu*) menu;
  active = (GtkMenuItem*) rmenu->old_active_menu_item;

  if (!active && rmenu->children)
    {
      list = rmenu->children;
      while (list)
	{
	  active = list->data;
	  list = list->next;

	  if (active->child)
	    break;
	  active = NULL;
	}
      rmenu->old_active_menu_item = (GtkWidget*) active;
    }

  g_function_leave ("gtk_menu_get_active");
  return ((GtkWidget*) active);
}

void
gtk_menu_set_active (GtkWidget *menu,
		     gint       index)
{
  GtkMenu *rmenu;
  GtkMenuItem *active;
  GList *list;

  g_function_enter ("gtk_menu_set_active");

  g_assert (menu != NULL);
  g_assert (menu->type == gtk_get_menu_type ());

  rmenu = (GtkMenu*) menu;

  list = g_list_nth (rmenu->children, index);

  active = list->data;
  if (active && active->child)
    rmenu->old_active_menu_item = (GtkWidget*) active;

  g_function_leave ("gtk_menu_set_active");
}

GtkWidget*
gtk_menu_item_get_child (GtkWidget *menu_item)
{
  GtkMenuItem *rmenu_item;
  GtkWidget *child;

  g_function_enter ("gtk_menu_item_get_child");

  g_assert (menu_item != NULL);
  rmenu_item = (GtkMenuItem*) menu_item;
  child = rmenu_item->child;

  g_function_leave ("gtk_menu_item_get_child");
  return child;
}

guint16
gtk_get_menu_type ()
{
  static guint16 menu_type = 0;

  g_function_enter ("gtk_get_menu_type");

  if (!menu_type)
    gtk_widget_unique_type (&menu_type);

  g_function_leave ("gtk_get_menu_type");
  return menu_type;
}

guint16
gtk_get_menu_bar_type ()
{
  static guint16 menu_bar_type = 0;

  g_function_enter ("gtk_get_menu_bar_type");

  if (!menu_bar_type)
    gtk_widget_unique_type (&menu_bar_type);

  g_function_leave ("gtk_get_menu_bar_type");
  return menu_bar_type;
}

guint16
gtk_get_menu_item_type ()
{
  static guint16 menu_item_type = 0;

  g_function_enter ("gtk_get_menu_item_type");

  if (!menu_item_type)
    gtk_widget_unique_type (&menu_item_type);

  g_function_leave ("gtk_get_menu_item_type");
  return menu_item_type;
}

guint16
gtk_get_menu_toggle_item_type ()
{
  static guint16 toggle_menu_item_type = 0;

  g_function_enter ("gtk_get_menu_item_type");

  if (!toggle_menu_item_type)
    toggle_menu_item_type = gtk_get_menu_item_type ();

  g_function_leave ("gtk_get_menu_item_type");
  return toggle_menu_item_type;
}


static void
gtk_menu_destroy (GtkWidget *widget)
{
  GtkMenu *menu;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_menu_destroy");

  g_assert (widget != NULL);
  menu = (GtkMenu*) widget;

  children = menu->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (!gtk_widget_destroy (child))
	child->parent = NULL;
    }

  g_list_free (menu->children);

  if (menu->window)
    {
      child = menu->window;
      menu->window = NULL;
      menu->frame = NULL;
      menu->children = NULL;

      if (!gtk_widget_destroy (child))
	child->parent = NULL;

      g_free (menu);
    }

  g_function_leave ("gtk_menu_destroy");
}

static void
gtk_menu_show (GtkWidget *widget)
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_show");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  if (!GTK_WIDGET_VISIBLE (widget))
    {
      menu = (GtkMenu*) widget;

      GTK_WIDGET_SET_FLAGS (widget, GTK_VISIBLE);
      gtk_container_need_resize (widget->parent, widget);
      gtk_widget_show (menu->window);

      gtk_grab_add (widget);
    }

  g_function_leave ("gtk_menu_show");
}

static void
gtk_menu_hide (GtkWidget *widget)
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_hide");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  if (GTK_WIDGET_VISIBLE (widget))
    {
      menu = (GtkMenu*) widget;

      GTK_WIDGET_UNSET_FLAGS (widget, GTK_VISIBLE);
      gtk_widget_hide (menu->window);

      gtk_grab_remove (widget);
    }

  g_function_leave ("gtk_menu_hide");
}

static void
gtk_menu_map (GtkWidget *widget)
{
  GtkMenu *menu;
  GtkWidget *child;
  GList *list;

  g_function_enter ("gtk_menu_map");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  menu = (GtkMenu*) widget;
  list = menu->children;

  while (list)
    {
      child = list->data;
      list = list->next;

      if (GTK_WIDGET_VISIBLE (child) && !GTK_WIDGET_MAPPED (child))
	gtk_widget_map (child);
    }

  g_function_leave ("gtk_menu_map");
}

static void
gtk_menu_unmap (GtkWidget *widget)
{
  GtkMenu *menu;
  GtkWidget *child;
  GList *list;

  g_function_enter ("gtk_menu_unmap");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  menu = (GtkMenu*) widget;
  list = menu->children;

  while (list)
    {
      child = list->data;
      list = list->next;

      if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_MAPPED (child))
	gtk_widget_unmap (child);
    }

  g_function_leave ("gtk_menu_unmap");
}

static void
gtk_menu_realize (GtkWidget *widget)
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_realize");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  menu = (GtkMenu*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  widget->window = menu->window->window;
  widget->style = gtk_style_attach (widget->style, widget->window);

  g_function_leave ("gtk_menu_realize");
}

static void
gtk_menu_draw (GtkWidget    *widget,
	       GdkRectangle *area,
	       gint          is_expose)
{
  GtkMenu *menu;
  GList *list;
  GtkWidget *child;
  GdkRectangle child_area;

  g_function_enter ("gtk_menu_draw");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      menu = (GtkMenu*) widget;
      list = menu->children;

      while (list)
	{
	  child = list->data;
	  list = list->next;

	  if (!is_expose || GTK_WIDGET_NO_WINDOW (child))
	    if (gtk_widget_intersect (child, area, &child_area))
	      gtk_widget_draw (child, &child_area, is_expose);
	}
    }

  g_function_leave ("gtk_menu_draw");
}

static gint
gtk_menu_event (GtkWidget *widget,
		GdkEvent  *event)
{
  GtkMenu *menu;
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  menu = (GtkMenu*) widget;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      if (GTK_WIDGET_VISIBLE (widget))
	{
	  if (menu->active_menu_item)
	    {
	      if (menu->active_menu_item->window == event->any.window)
		{
		  if (((GtkMenuItem*) menu->active_menu_item)->child)
		    menu->old_active_menu_item = menu->active_menu_item;

		  gtk_widget_activate (menu->active_menu_item);
		}

	      gtk_menu_item_deselect (menu->active_menu_item);
	      menu->active_menu_item = NULL;
	    }

	  if (menu->parent)
	    gtk_widget_event (menu->parent, event);
	  else
	    gtk_widget_hide (widget);
	}
      break;

    case GDK_ENTER_NOTIFY:
      if (GTK_WIDGET_VISIBLE (widget))
	{
	  menu_item = (GtkMenuItem*) gtk_get_event_widget (event);

	  if (gtk_widget_is_immediate_child (widget, (GtkWidget*) menu_item))
	    {
	      if ((GTK_WIDGET_TYPE (menu_item) == gtk_get_menu_item_type ()) &&
		  (menu->active_menu_item != (GtkWidget*) menu_item))
		{
		  if (menu->active_menu_item)
		    gtk_menu_item_deselect (menu->active_menu_item);

		  menu->active_menu_item = (GtkWidget*) menu_item;
		  gtk_menu_item_select (menu->active_menu_item);
		}
	    }
	  else if (menu->parent)
	    gtk_widget_event (menu->parent, event);
	}
      break;

    case GDK_LEAVE_NOTIFY:
      if (GTK_WIDGET_VISIBLE (widget))
	{
	  menu_item = (GtkMenuItem*) gtk_get_event_widget (event);

	  if ((menu->active_menu_item == (GtkWidget*) menu_item) && (menu_item->submenu == NULL))
	    {
	      gtk_menu_item_deselect (menu->active_menu_item);
	      menu->active_menu_item = NULL;
	    }
	  else if ((menu->parent == (GtkWidget*) menu_item) &&
		   (GTK_WIDGET_TYPE (menu_item->container.widget.parent) == gtk_get_menu_type ()))
	    {
	      if (menu->container.widget.window != gdk_window_get_pointer (NULL, NULL, NULL, NULL))
		{
		  menu = (GtkMenu*) menu_item->container.widget.parent;
		  gtk_menu_item_deselect (menu->active_menu_item);
		  menu->active_menu_item = NULL;
		}
	    }
	  else if (menu->parent)
	    {
	      gtk_widget_event (menu->parent, event);
	    }
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_menu_event");
  return TRUE;
}

static void
gtk_menu_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkMenu *menu;
  GtkMenuItem *menu_item;
  GtkWidget *child;
  GList *list;
  gint nchildren;
  gint max_accelerator_size;
  gint toggle_exists;

  g_function_enter ("gtk_menu_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  menu = (GtkMenu*) widget;

  requisition->width = 0;
  requisition->height = 0;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      nchildren = 0;
      max_accelerator_size = 0;
      toggle_exists = FALSE;

      list = menu->children;
      while (list)
	{
	  child = list->data;
	  list = list->next;

	  if (GTK_WIDGET_VISIBLE (child))
	    {
	      child->requisition.width = 0;
	      child->requisition.height = 0;

	      gtk_widget_size_request (child, &child->requisition);

	      requisition->width = MAX (requisition->width, child->requisition.width);
	      requisition->height += child->requisition.height;

	      nchildren += 1;

	      menu_item = (GtkMenuItem*) child;
	      max_accelerator_size = MAX (max_accelerator_size, menu_item->accelerator_size);
	      if (menu_item->toggle_exists)
		toggle_exists = TRUE;
	    }
	}

      if (nchildren > 0)
	{
	  if (toggle_exists)
	    requisition->width += TOGGLE_MARK_SIZE + TOGGLE_MARK_SPACING * 2;
	  requisition->width += menu->container.border_width * 2 + max_accelerator_size;
	  requisition->height += menu->container.border_width * 2;
	}

      list = menu->children;
      while (list)
	{
	  menu_item = list->data;
	  list = list->next;

	  menu_item->accelerator_size = max_accelerator_size;

	  if (toggle_exists)
	    {
	      menu_item->toggle_draw = TRUE;
	    }
	  else
	    {
	      menu_item->toggle_exists = FALSE;
	      menu_item->toggle_draw = FALSE;
	    }
	}
    }

  g_function_leave ("gtk_menu_size_request");
}

static void
gtk_menu_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkMenu *menu;
  GList *list;
  GtkWidget *child;
  GtkAllocation child_allocation;
  gint nchildren;

  g_function_enter ("gtk_menu_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  menu = (GtkMenu*) widget;
  widget->allocation = *allocation;

  nchildren = 0;
  list = menu->children;

  while (list)
    {
      child = list->data;
      list = list->next;

      if (GTK_WIDGET_VISIBLE (child))
	nchildren += 1;
    }

  if (nchildren > 0)
    {
      child_allocation.x = menu->container.border_width + allocation->x;
      child_allocation.y = menu->container.border_width + allocation->y;
      child_allocation.width = allocation->width - menu->container.border_width * 2;

      list = menu->children;

      while (list)
	{
	  child = list->data;
	  list = list->next;

	  if (GTK_WIDGET_VISIBLE (child))
	    {
	      child_allocation.height = child->requisition.height;

	      gtk_widget_size_allocate (child, &child_allocation);

	      child_allocation.y += child_allocation.height;
	    }
	}
    }

  g_function_leave ("gtk_menu_size_allocate");
}

static gint
gtk_menu_is_child (GtkWidget *widget,
		   GtkWidget *child)
{
  GtkMenu *menu;
  GtkWidget *child_widget;
  GList *list;
  gint return_val;

  g_function_enter ("gtk_menu_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_type ());

  menu = (GtkMenu*) widget;
  return_val = FALSE;

  list = menu->children;
  while (list)
    {
      child_widget = list->data;
      list = list->next;

      if (child_widget == child)
        {
          return_val = TRUE;
          break;
        }
    }

  if (!return_val)
    {
      list = menu->children;
      while (list)
        {
          child_widget = list->data;
          list = list->next;

          if (gtk_widget_is_child (child_widget, child))
            {
              return_val = TRUE;
              break;
            }
        }
    }

  g_function_leave ("gtk_menu_is_child");
  return return_val;
}

static gint
gtk_menu_locate (GtkWidget  *widget,
		 GtkWidget **child,
		 gint        x,
		 gint        y)
{
  g_function_enter ("gtk_menu_locate");
  g_warning ("gtk_menu_locate: UNFINISHED");
  g_function_leave ("gtk_menu_locate");
  return FALSE;
}

static void
gtk_menu_add (GtkContainer *container,
	      GtkWidget    *widget)
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (container) == gtk_get_menu_type ());
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_item_type ());

  menu = (GtkMenu*) container;
  menu->children = g_list_append (menu->children, widget);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_add");
}

static void
gtk_menu_remove (GtkContainer *container,
		 GtkWidget    *widget)
{
  GtkMenu *menu;

  g_function_enter ("gtk_menu_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_TYPE (container) == gtk_get_menu_type ());
  g_assert (GTK_WIDGET_TYPE (widget) == gtk_get_menu_item_type ());

  menu = (GtkMenu*) container;
  menu->children = g_list_remove (menu->children, widget);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_remove");
}

static void
gtk_menu_foreach (GtkContainer *container,
		  GtkCallback   callback,
		  gpointer      callback_data)
{
  GtkMenu *menu;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_menu_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  menu = (GtkMenu*) container;
  children = menu->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data, NULL);
    }

  g_function_leave ("gtk_menu_foreach");
}

static void
gtk_menu_window_resize (GtkWidget *widget,
			gint      *x,
			gint      *y,
			gint      *width,
			gint      *height)
{
  GtkMenu *menu;
  GtkMenu *parent_menu;
  GtkMenuItem *menu_item;
  GtkMenuItem *parent_menu_item;
  GdkWindow *gdk_window;
  gint screen_width, screen_height;
  gint twidth, theight;
  gint tx, ty;

  g_function_enter ("gtk_menu_window_resize");

  g_assert (widget != NULL);
  g_assert (x != NULL);
  g_assert (y != NULL);
  g_assert (width != NULL);
  g_assert (height != NULL);

  menu = (GtkMenu*) gtk_widget_get_user_data (widget);

  gdk_window = widget->window;
  menu_item = (GtkMenuItem*) menu->parent;

  if (menu_item && (GTK_WIDGET_TYPE (menu_item) == gtk_get_option_menu_type ()))
    {
      gtk_option_menu_position ((GtkWidget*) menu_item, x, y, gdk_window->width, gdk_window->height);
    }
  else if (menu_item && (GTK_WIDGET_TYPE (menu_item) == gtk_get_menu_item_type ()))
    {
      parent_menu = (GtkMenu*) menu_item->container.widget.parent;
      parent_menu_item = (GtkMenuItem*) parent_menu->parent;

      twidth = widget->requisition.width;
      theight = widget->requisition.height;

      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();

      if (!gdk_window_get_origin (menu_item->container.widget.window, &tx, &ty))
	g_error ("unable to get window origin");

      if (GTK_WIDGET_TYPE (parent_menu) == gtk_get_menu_bar_type ())
	{
	  if ((ty + menu_item->container.widget.window->height + theight) <= screen_height)
	    ty += menu_item->container.widget.window->height;
	  else if ((ty - theight) >= 0)
	    ty -= theight;
	  else
	    ty += menu_item->container.widget.window->height;

	  if ((tx + twidth) > screen_width)
	    {
	      tx -= ((tx + twidth) - screen_width);
	      if (tx < 0)
		tx = 0;
	    }
	}
      else if (GTK_WIDGET_TYPE (parent_menu) == gtk_get_menu_type ())
	{
	  if (parent_menu_item)
	    menu_item->submenu_direction = parent_menu_item->submenu_direction;
	  else
	    menu_item->submenu_direction = DIRECTION_RIGHT;

	  switch (menu_item->submenu_direction)
	    {
	    case DIRECTION_LEFT:
	      if ((tx - twidth) >= 0)
		tx -= twidth;
	      else
		{
		  menu_item->submenu_direction = DIRECTION_RIGHT;
		  tx += menu_item->container.widget.window->width - 5;
		}
	      break;

	    case DIRECTION_RIGHT:
	      if ((tx + menu_item->container.widget.window->width + twidth - 5) <= screen_width)
		tx += menu_item->container.widget.window->width - 5;
	      else
		{
		  menu_item->submenu_direction = DIRECTION_LEFT;
		  tx -= twidth;
		}
	      break;
	    }

	  if ((ty + menu_item->container.widget.window->height / 4 + theight) <= screen_height)
	    ty += menu_item->container.widget.window->height / 4;
	  else
	    {
	      ty -= ((ty + theight) - screen_height);
	      if (ty < 0)
		ty = 0;
	    }
	}
      else
	g_error ("menu item is not a child of a menu or menu bar");

      *x = tx;
      *y = ty;
      *width = twidth;
      *height = theight;
    }
  else
    {
      tx = gdk_window->x;
      ty = gdk_window->y;

      twidth = gdk_window->width;
      theight = gdk_window->height;

      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();

      if (tx < 0)
	tx = 0;
      else if ((tx + twidth) > screen_width)
	tx -= ((tx + twidth) - screen_width);

      if (ty < 0)
	ty = 0;
      else if ((ty + theight) > screen_height)
	ty -= ((ty + theight) - screen_height);

      *x = tx;
      *y = ty;
    }

  g_function_leave ("gtk_menu_window_resize");
}

static void
gtk_menu_bar_destroy (GtkWidget *widget)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_menu_bar_destroy");

  g_assert (widget != NULL);
  menu_bar = (GtkMenuBar*) widget;

  children = menu_bar->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (!gtk_widget_destroy (child))
	child->parent = NULL;
    }

  g_list_free (menu_bar->children);

  if (menu_bar->container.widget.window)
    gdk_window_destroy (menu_bar->container.widget.window);
  g_free (menu_bar);

  g_function_leave ("gtk_menu_bar_destroy");
}

static void
gtk_menu_bar_map (GtkWidget *widget)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GList *list;

  g_function_enter ("gtk_menu_bar_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);

  menu_bar = (GtkMenuBar*) widget;
  list = menu_bar->children;

  while (list)
    {
      child = list->data;
      list = list->next;

      if (GTK_WIDGET_VISIBLE (child) && !GTK_WIDGET_MAPPED (child))
        gtk_widget_map (child);
    }

  g_function_leave ("gtk_menu_bar_map");
}

static void
gtk_menu_bar_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_menu_bar_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_menu_bar_unmap");
}

static void
gtk_menu_bar_realize (GtkWidget *widget)
{
  GtkMenuBar *menu_bar;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_menu_bar_realize");

  g_assert (widget != NULL);

  menu_bar = (GtkMenuBar*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK);

  menu_bar->container.widget.window = gdk_window_new (widget->parent->widget.window,
						      &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (menu_bar->container.widget.window, menu_bar);

  menu_bar->container.widget.style = gtk_style_attach (menu_bar->container.widget.style,
						       menu_bar->container.widget.window);
  gdk_window_set_background (menu_bar->container.widget.window,
			     &menu_bar->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_menu_bar_realize");
}

static void
gtk_menu_bar_draw (GtkWidget    *widget,
		   GdkRectangle *area,
		   gint          is_expose)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GList *list;
  GdkRectangle child_area;

  g_function_enter ("gtk_menu_bar_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      menu_bar = (GtkMenuBar*) widget;
      gtk_draw_shadow (widget->window,
		       menu_bar->container.widget.style->highlight_gc[GTK_STATE_NORMAL],
		       menu_bar->container.widget.style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       GTK_SHADOW_OUT,
		       0, 0,
		       widget->allocation.width,
		       widget->allocation.height,
		       menu_bar->container.widget.style->shadow_thickness);

      list = menu_bar->children;
      while (list)
	{
	  child = list->data;
	  list = list->next;

	  if (!is_expose || GTK_WIDGET_NO_WINDOW (child))
	    if (gtk_widget_intersect (child, area, &child_area))
	      gtk_widget_draw (child, &child_area, is_expose);
	}
    }

  g_function_leave ("gtk_menu_bar_draw");
}

static gint
gtk_menu_bar_event (GtkWidget *widget,
		    GdkEvent  *event)
{
  GtkMenuBar *menu_bar;
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_bar_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  menu_bar = (GtkMenuBar*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_BUTTON_PRESS:
      if (!menu_bar->active)
	{
	  gtk_grab_add (widget);

	  gdk_window_get_user_data (event->any.window, (void**) &menu_item);

	  menu_bar->active = TRUE;
	  if (GTK_WIDGET_TYPE (menu_item) == gtk_get_menu_item_type ())
	    {
	      menu_bar->active_menu_item = (GtkWidget*) menu_item;
	      gtk_menu_item_select (menu_bar->active_menu_item);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (menu_bar->active)
	{
	  menu_bar->active = FALSE;
	  if (menu_bar->active_menu_item)
	    {
	      gtk_menu_item_deselect (menu_bar->active_menu_item);
	      menu_bar->active_menu_item = NULL;
	    }

	  gtk_grab_remove (widget);
	}
      break;

    case GDK_ENTER_NOTIFY:
      if (menu_bar->active)
	{
	  gdk_window_get_user_data (event->any.window, (void**) &menu_item);

	  if (gtk_widget_is_immediate_child (widget, (GtkWidget*) menu_item))
	    {
	      if ((GTK_WIDGET_TYPE (menu_item) == gtk_get_menu_item_type ()) &&
		  (menu_bar->active_menu_item != (GtkWidget*) menu_item))
		{
		  if (menu_bar->active_menu_item)
		    gtk_menu_item_deselect (menu_bar->active_menu_item);

		  menu_bar->active_menu_item = (GtkWidget*) menu_item;
		  gtk_menu_item_select (menu_bar->active_menu_item);
		}
	    }
	}
      break;

    case GDK_LEAVE_NOTIFY:
      break;

    default:
      break;
    }

  g_function_leave ("gtk_menu_bar_event");
  return TRUE;
}

static void
gtk_menu_bar_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GList *list;
  gint nchildren;
  gint shadow_thickness;

  g_function_enter ("gtk_menu_bar_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  menu_bar = (GtkMenuBar*) widget;

  shadow_thickness = widget->style->shadow_thickness;

  requisition->width = 4 + shadow_thickness * 2;
  requisition->height = 4 + shadow_thickness * 2;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      nchildren = 0;
      list = menu_bar->children;

      while (list)
        {
          child = list->data;
          list = list->next;

          if (GTK_WIDGET_VISIBLE (child))
            {
              child->requisition.width = 0;
              child->requisition.height = 0;

              gtk_widget_size_request (child, &child->requisition);

              requisition->width += child->requisition.width;
              requisition->height = MAX (requisition->height, child->requisition.height);

              nchildren += 1;
            }
        }

      if (nchildren > 0)
        {
	  requisition->width += (10 * (nchildren - 1) +
				 menu_bar->container.border_width * 2 +
				 shadow_thickness * 2 + 4);
	  requisition->height += (menu_bar->container.border_width * 2 +
				  shadow_thickness * 2 + 4);
        }
    }

  g_function_leave ("gtk_menu_bar_size_request");
}

static void
gtk_menu_bar_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GtkAllocation child_allocation;
  GList *list;
  gint nchildren;
  gint shadow_thickness;

  g_function_enter ("gtk_menu_bar_size_allocate");

  g_assert (widget != NULL);
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

  nchildren = 0;
  menu_bar = (GtkMenuBar*) widget;
  list = menu_bar->children;

  while (list)
    {
      child = list->data;
      list = list->next;

      if (GTK_WIDGET_VISIBLE (child))
        nchildren += 1;
    }

  if (nchildren > 0)
    {
      shadow_thickness = widget->style->shadow_thickness;

      child_allocation.x = menu_bar->container.border_width + shadow_thickness + 2;
      child_allocation.y = menu_bar->container.border_width + shadow_thickness + 2;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      list = menu_bar->children;

      while (list)
        {
          child = list->data;
          list = list->next;

          if (GTK_WIDGET_VISIBLE (child))
            {
              child_allocation.width = child->requisition.width;

              gtk_widget_size_allocate (child, &child_allocation);

              child_allocation.x += child_allocation.width + 10;
            }
        }
    }

  g_function_leave ("gtk_menu_bar_size_allocate");
}

static gint
gtk_menu_bar_is_child (GtkWidget *widget,
		       GtkWidget *child)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child_widget;
  GList *list;
  gint return_val;

  g_function_enter ("gtk_menu_bar_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  menu_bar = (GtkMenuBar*) widget;
  return_val = FALSE;

  list = menu_bar->children;
  while (list)
    {
      child_widget = list->data;
      list = list->next;

      if (child_widget == child)
	{
	  return_val = TRUE;
	  break;
	}
    }

  if (!return_val)
    {
      list = menu_bar->children;
      while (list)
	{
          child_widget = list->data;
          list = list->next;

          if (gtk_widget_is_child (child_widget, child))
            {
              return_val = TRUE;
              break;
            }
	}
    }

  g_function_leave ("gtk_menu_bar_is_child");
  return return_val;
}

static gint
gtk_menu_bar_locate (GtkWidget  *widget,
		     GtkWidget **child,
		     gint        x,
		     gint        y)
{
  g_function_enter ("gtk_menu_bar_locate");
  g_warning ("gtk_menu_bar_locate: UNFINISHED");
  g_function_leave ("gtk_menu_bar_locate");
  return FALSE;
}

static void
gtk_menu_bar_add (GtkContainer *container,
		  GtkWidget    *widget)
{
  GtkMenuBar *menu_bar;

  g_function_enter ("gtk_menu_bar_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  menu_bar = (GtkMenuBar*) container;
  menu_bar->children = g_list_append (menu_bar->children, widget);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_bar_add");
}

static void
gtk_menu_bar_remove (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkMenuBar *menu_bar;

  g_function_enter ("gtk_menu_bar_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  menu_bar = (GtkMenuBar*) container;
  menu_bar->children = g_list_remove (menu_bar->children, widget);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_bar_remove");
}

static void
gtk_menu_bar_foreach (GtkContainer *container,
		      GtkCallback   callback,
		      gpointer      callback_data)
{
  GtkMenuBar *menu_bar;
  GtkWidget *child;
  GList *children;

  g_function_enter ("gtk_menu_bar_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  menu_bar = (GtkMenuBar*) container;
  children = menu_bar->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data, NULL);
    }

  g_function_leave ("gtk_menu_bar_foreach");
}

static void
gtk_menu_item_destroy (GtkWidget *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_destroy");

  g_assert (widget != NULL);

  menu_item = (GtkMenuItem*) widget;
  if (menu_item->child)
    if (!gtk_widget_destroy (menu_item->child))
      menu_item->child->parent = NULL;

  gtk_data_detach ((GtkData*) &menu_item->state, &menu_item->state_observer);
  gtk_data_disconnect ((GtkData*) &menu_item->state);

  if (menu_item->container.widget.window)
    gdk_window_destroy (menu_item->container.widget.window);
  g_free (menu_item);

  g_function_leave ("gtk_menu_item_destroy");
}

static void
gtk_menu_item_map (GtkWidget *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);

  menu_item = (GtkMenuItem*) widget;

  if (menu_item->child &&
      GTK_WIDGET_VISIBLE (menu_item->child) &&
      !GTK_WIDGET_MAPPED (menu_item->child))
    gtk_widget_map (menu_item->child);

  g_function_leave ("gtk_menu_item_map");
}

static void
gtk_menu_item_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_menu_item_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_menu_item_unmap");
}

static void
gtk_menu_item_realize (GtkWidget *widget)
{
  GtkMenuItem *menu_item;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_menu_item_realize");

  g_assert (widget != NULL);

  menu_item = (GtkMenuItem*) widget;
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

  menu_item->container.widget.window = gdk_window_new (widget->parent->widget.window,
						       &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (menu_item->container.widget.window, menu_item);

  menu_item->container.widget.style = gtk_style_attach (menu_item->container.widget.style,
							menu_item->container.widget.window);
  gdk_window_set_background (menu_item->container.widget.window,
			     &menu_item->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_menu_item_realize");
}

static void
gtk_menu_item_draw (GtkWidget    *widget,
		    GdkRectangle *area,
		    gint          is_expose)
{
  GtkMenuItem *menu_item;
  GdkRectangle child_area;

  g_function_enter ("gtk_menu_item_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      menu_item = (GtkMenuItem*) widget;

      if (menu_item->child)
	gtk_widget_set_state (menu_item->child, menu_item->state.value);

      gtk_menu_item_expose (widget);

      if (menu_item->child)
	if (!is_expose && !GTK_WIDGET_NO_WINDOW (menu_item->child))
	  if (gtk_widget_intersect (menu_item->child, area, &child_area))
	    gtk_widget_draw (menu_item->child, &child_area, is_expose);
    }

  g_function_leave ("gtk_menu_item_draw");
}

static void
gtk_menu_item_expose (GtkWidget *widget)
{
  GtkMenuItem *menu_item;
  gint x, y;
  gint width, height;
  gchar buf[32];

  g_function_enter ("gtk_menu_item_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      menu_item = (GtkMenuItem*) widget;

      if (menu_item->child)
	{
	  gdk_window_set_background (widget->window,
				     &widget->style->background[menu_item->state.value]);
	  gdk_window_clear (widget->window);

	  if (GTK_WIDGET_NO_WINDOW (menu_item->child))
	    gtk_widget_draw (menu_item->child, NULL, TRUE);

	  x = menu_item->container.border_width;
	  y = menu_item->container.border_width;
	  width = widget->allocation.width - 2 * x;
	  height = widget->allocation.height - 2 * y;

	  if (menu_item->state.value == GTK_STATE_SELECTED)
	    {
	      gtk_draw_shadow (widget->window,
			       widget->style->highlight_gc[menu_item->state.value],
			       widget->style->shadow_gc[menu_item->state.value],
			       NULL,
			       GTK_SHADOW_OUT,
			       x, y, width, height,
			       widget->style->shadow_thickness);
	    }

	  if (menu_item->accelerator_key)
	    {
	      gtk_menu_item_calc_accelerator_text (menu_item, buf);
	      gdk_draw_string (widget->window,
			       widget->style->foreground_gc[menu_item->state.value],
			       x + width - menu_item->accelerator_size + 13 - 4,
			       (y + height / 2 -
				((widget->style->foreground_gc[menu_item->state.value]->font->ascent +
				  widget->style->foreground_gc[menu_item->state.value]->font->descent) / 2) +
				widget->style->foreground_gc[menu_item->state.value]->font->ascent),
			       buf);
	    }
	  else if (menu_item->submenu && (GTK_WIDGET_TYPE (widget->parent) == gtk_get_menu_type ()))
	    {
	      gtk_draw_arrow (widget->window,
			      widget->style->highlight_gc[menu_item->state.value],
			      widget->style->shadow_gc[menu_item->state.value],
			      NULL,
			      GTK_ARROW_RIGHT,
			      ((menu_item->state.value == GTK_STATE_NORMAL) ?
			       (GTK_SHADOW_OUT) : (GTK_SHADOW_IN)),
			      x + width - 15,
			      y + height / 2 - 5,
			      10, 10,
			      widget->style->shadow_thickness);
	    }
	}
      else
	{
	  gtk_draw_hline (widget->window,
			  widget->style->highlight_gc[GTK_STATE_NORMAL],
			  widget->style->shadow_gc[GTK_STATE_NORMAL],
			  0, widget->allocation.width, 0,
			  widget->style->shadow_thickness);
	}
    }

  g_function_leave ("gtk_menu_item_expose");
}

static gint
gtk_menu_item_event (GtkWidget *widget,
		     GdkEvent  *event)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  menu_item = (GtkMenuItem*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      gtk_widget_event ((GtkWidget*) widget->parent, event);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_menu_item_event");
  return TRUE;
}

static void
gtk_menu_item_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkMenuItem *menu_item;
  gint shadow_thickness;

  g_function_enter ("gtk_menu_item_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  menu_item = (GtkMenuItem*) widget;
  shadow_thickness = widget->style->shadow_thickness;

  if (GTK_WIDGET_VISIBLE (widget) && menu_item->child)
    {
      gtk_menu_item_calc_accelerator_size (menu_item);

      menu_item->child->requisition.width = 0;
      menu_item->child->requisition.height = 0;

      gtk_widget_size_request (menu_item->child, &menu_item->child->requisition);

      requisition->width = (menu_item->child->requisition.width +
			    menu_item->container.border_width * 2 +
			    shadow_thickness * 2 + 4);
      requisition->height = (menu_item->child->requisition.height +
			     menu_item->container.border_width * 2 +
			     shadow_thickness * 2 + 4);
    }
  else
    {
      requisition->width = 1;
      requisition->height = shadow_thickness;
    }

  g_function_leave ("gtk_menu_item_size_request");
}

static void
gtk_menu_item_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkMenuItem *menu_item;
  GtkAllocation child_allocation;
  gint shadow_thickness;

  g_function_enter ("gtk_menu_item_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  menu_item = (GtkMenuItem*) widget;

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

  if (menu_item->child)
    {
      shadow_thickness = widget->style->shadow_thickness;
      child_allocation.x = menu_item->container.border_width + shadow_thickness + 2;
      child_allocation.y = menu_item->container.border_width + shadow_thickness + 2;
      child_allocation.width = allocation->width - child_allocation.x * 2 - menu_item->accelerator_size;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      if (menu_item->toggle_draw)
	{
	  child_allocation.x += TOGGLE_MARK_SIZE + TOGGLE_MARK_SPACING * 2;
	  child_allocation.width -= TOGGLE_MARK_SIZE + TOGGLE_MARK_SPACING * 2;
	}

      if (child_allocation.width <= 0)
        child_allocation.width = 1;
      if (child_allocation.height <= 0)
        child_allocation.height = 1;

      gtk_widget_size_allocate (menu_item->child, &child_allocation);
    }

  g_function_leave ("gtk_menu_item_size_allocate");
}

static gint
gtk_menu_item_is_child (GtkWidget *widget,
			GtkWidget *child)
{
  GtkMenuItem *menu_item;
  gint return_val;

  g_function_enter ("gtk_menu_item_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  menu_item = (GtkMenuItem*) widget;

  return_val = FALSE;
  if (menu_item->child == child)
    return_val = TRUE;
  else if (menu_item->child)
    return_val = gtk_widget_is_child (menu_item->child, child);

  g_function_leave ("gtk_menu_item_is_child");
  return return_val;
}

static gint
gtk_menu_item_locate (GtkWidget  *widget,
		      GtkWidget **child,
		      gint        x,
		      gint        y)
{
  g_function_enter ("gtk_menu_item_locate");
  g_warning ("gtk_menu_item_locate: UNFINISHED");
  g_function_leave ("gtk_menu_item_locate");
  return FALSE;
}

static void
gtk_menu_item_activate (GtkWidget *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_activate");

  g_assert (widget != NULL);
  menu_item = (GtkMenuItem*) widget;

  menu_item->state.value = GTK_STATE_ACTIVATED;
  gtk_data_notify ((GtkData*) &menu_item->state);

  menu_item->state.value = GTK_STATE_NORMAL;
  gtk_data_notify ((GtkData*) &menu_item->state);

  g_function_leave ("gtk_menu_item_activate");
}

static gint
gtk_menu_item_install_accelerator (GtkWidget *widget,
				   gchar      accelerator_key,
				   guint8     accelerator_mods)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_install_accelerator");

  g_assert (widget != NULL);
  menu_item = (GtkMenuItem*) widget;

  menu_item->accelerator_key = accelerator_key;
  menu_item->accelerator_mods = accelerator_mods;

  if (widget->parent)
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_menu_item_install_accelerator");
  return TRUE;
}

static void
gtk_menu_item_remove_accelerator (GtkWidget *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_remove_accelerator");

  g_assert (widget != NULL);
  menu_item = (GtkMenuItem*) widget;

  menu_item->accelerator_key = '\0';
  menu_item->accelerator_mods = 0;

  gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_menu_item_remove_accelerator");
}

static void
gtk_menu_item_add (GtkContainer *container,
		   GtkWidget    *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  menu_item = (GtkMenuItem*) container;

  if (menu_item->child)
    g_error ("menu item already has a child");
  else
    menu_item->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_item_add");
}

static void
gtk_menu_item_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  menu_item = (GtkMenuItem*) container;

  if (menu_item->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!menu_item->child)
    g_error ("menu item has no child to remove");

  menu_item->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_menu_item_remove");
}

static void
gtk_menu_item_foreach (GtkContainer *container,
		       GtkCallback   callback,
		       gpointer      callback_data)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  menu_item = (GtkMenuItem*) container;

  if (menu_item->child)
    (* callback) (menu_item->child, callback_data, NULL);

  g_function_leave ("gtk_menu_item_foreach");
}

static void
gtk_menu_toggle_item_destroy (GtkWidget *widget)
{
  GtkMenuToggleItem *toggle_item;

  g_function_enter ("gtk_menu_toggle_item_destroy");

  g_assert (widget != NULL);

  toggle_item = (GtkMenuToggleItem*) widget;
  if (toggle_item->menu_item.child)
    if (!gtk_widget_destroy (toggle_item->menu_item.child))
      toggle_item->menu_item.child->parent = NULL;

  gtk_data_detach ((GtkData*) &toggle_item->menu_item.state, &toggle_item->menu_item.state_observer);
  gtk_data_disconnect ((GtkData*) &toggle_item->menu_item.state);
  gtk_data_detach ((GtkData*) toggle_item->owner, &toggle_item->owner_observer);
  gtk_data_destroy ((GtkData*) toggle_item->owner);

  if (toggle_item->menu_item.container.widget.window)
    gdk_window_destroy (toggle_item->menu_item.container.widget.window);
  g_free (toggle_item);

  g_function_leave ("gtk_menu_toggle_item_destroy");
}

static void
gtk_menu_toggle_item_draw (GtkWidget    *widget,
			   GdkRectangle *area,
			   gint          is_expose)
{
  GtkMenuToggleItem *toggle_item;
  GdkRectangle child_area;

  g_function_enter ("gtk_menu_toggle_item_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      toggle_item = (GtkMenuToggleItem*) widget;

      if (toggle_item->menu_item.child)
	gtk_widget_set_state (toggle_item->menu_item.child, toggle_item->menu_item.state.value);

      gtk_menu_toggle_item_expose (widget);

      if (!is_expose && !GTK_WIDGET_NO_WINDOW (toggle_item->menu_item.child))
	if (gtk_widget_intersect (toggle_item->menu_item.child, area, &child_area))
	  gtk_widget_draw (toggle_item->menu_item.child, &child_area, is_expose);
    }

  g_function_leave ("gtk_menu_toggle_item_draw");
}

static void
gtk_menu_toggle_item_expose (GtkWidget *widget)
{
  GtkMenuToggleItem *toggle_item;
  GtkStateType state;
  GtkShadowType shadow;
  gint x, y;
  gint width, height;
  gchar buf[32];

  g_function_enter ("gtk_menu_toggle_item_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      toggle_item = (GtkMenuToggleItem*) widget;

      if (toggle_item->menu_item.child)
	{
	  state = toggle_item->menu_item.state.value;
	  if ((toggle_item->menu_item.previous_state == GTK_STATE_ACTIVE) &&
 	      (toggle_item->menu_item.state.value == GTK_STATE_ACTIVE))
	    state = GTK_STATE_NORMAL;

	  gdk_window_set_background (widget->window,
				     &widget->style->background[state]);
	  gdk_window_clear (widget->window);

	  if (GTK_WIDGET_NO_WINDOW (toggle_item->menu_item.child))
	    gtk_widget_draw (toggle_item->menu_item.child, NULL, TRUE);

	  x = toggle_item->menu_item.container.border_width;
	  y = toggle_item->menu_item.container.border_width;
	  width = widget->allocation.width - 2 * x;
	  height = widget->allocation.height - 2 * y;

	  if (toggle_item->menu_item.state.value == GTK_STATE_SELECTED)
	    {
	      gtk_draw_shadow (widget->window,
			       widget->style->highlight_gc[toggle_item->menu_item.state.value],
			       widget->style->shadow_gc[toggle_item->menu_item.state.value],
			       NULL,
			       GTK_SHADOW_OUT,
			       x, y, width, height,
			       widget->style->shadow_thickness);
	    }

	  if (toggle_item->menu_item.accelerator_key)
	    {
	      gtk_menu_item_calc_accelerator_text (&toggle_item->menu_item, buf);
	      gdk_draw_string (widget->window,
			       widget->style->foreground_gc[toggle_item->menu_item.state.value],
			       x + width - toggle_item->menu_item.accelerator_size + 13 - 4,
			       (y + height / 2 -
				((widget->style->foreground_gc[toggle_item->menu_item.state.value]->font->ascent +
				  widget->style->foreground_gc[toggle_item->menu_item.state.value]->font->descent) / 2) +
				widget->style->foreground_gc[toggle_item->menu_item.state.value]->font->ascent),
			       buf);
	    }
	  else if (toggle_item->menu_item.submenu && (GTK_WIDGET_TYPE (widget->parent) == gtk_get_menu_type ()))
	    {
	      gtk_draw_arrow (widget->window,
			      widget->style->highlight_gc[toggle_item->menu_item.state.value],
			      widget->style->shadow_gc[toggle_item->menu_item.state.value],
			      NULL,
			      GTK_ARROW_RIGHT,
			      ((toggle_item->menu_item.state.value == GTK_STATE_NORMAL) ?
			       (GTK_SHADOW_OUT) : (GTK_SHADOW_IN)),
			      x + width - 15,
			      y + height / 2 - 5,
			      10, 10,
			      widget->style->shadow_thickness);
	    }

	  if (toggle_item->menu_item.previous_state == GTK_STATE_ACTIVE)
	    {
	      if (((toggle_item->menu_item.previous_state == GTK_STATE_ACTIVE) &&
		   (toggle_item->menu_item.state.value != GTK_STATE_SELECTED)) ||
		  ((toggle_item->menu_item.previous_state == GTK_STATE_NORMAL) &&
		   (toggle_item->menu_item.state.value == GTK_STATE_SELECTED)))
		shadow = GTK_SHADOW_IN;
	      else
		shadow = GTK_SHADOW_OUT;

	      state = toggle_item->menu_item.state.value;
	      if (toggle_item->owner->data.observers->next)
		{
		  gtk_draw_diamond (widget->window,
				    widget->style->highlight_gc[state],
				    widget->style->shadow_gc[state],
				    widget->style->background_gc[state],
				    shadow,
				    x + 4,
				    y + (height - TOGGLE_MARK_SIZE) / 2,
				    TOGGLE_MARK_SIZE,
				    TOGGLE_MARK_SIZE,
				    widget->style->shadow_thickness);
		}
	      else
		{
		  gtk_draw_shadow (widget->window,
				   widget->style->highlight_gc[state],
				   widget->style->shadow_gc[state],
				   widget->style->background_gc[state],
				   shadow,
				   x + 4,
				   y + (height - TOGGLE_MARK_SIZE) / 2,
				   TOGGLE_MARK_SIZE,
				   TOGGLE_MARK_SIZE,
				   widget->style->shadow_thickness);
		}
	    }
	}
      else
	{
	  gtk_draw_hline (widget->window,
			  widget->style->highlight_gc[GTK_STATE_NORMAL],
			  widget->style->shadow_gc[GTK_STATE_NORMAL],
			  0, widget->allocation.width, 0,
			  widget->style->shadow_thickness);
	}
    }

  g_function_leave ("gtk_menu_toggle_item_expose");
}

static void
gtk_menu_toggle_item_activate (GtkWidget *widget)
{
  GtkMenuToggleItem *toggle_item;

  g_function_enter ("gtk_menu_toggle_item_activate");

  g_assert (widget != NULL);
  toggle_item = (GtkMenuToggleItem*) widget;

  if (toggle_item->owner->data.observers->next)
    {
      if (toggle_item->menu_item.previous_state == GTK_STATE_NORMAL)
	{
	  toggle_item->menu_item.previous_state = GTK_STATE_ACTIVE;
	  toggle_item->owner->widget = (GtkWidget*) toggle_item;
	  gtk_data_notify ((GtkData*) toggle_item->owner);
	}
    }
  else
    {
      if (toggle_item->menu_item.previous_state == GTK_STATE_NORMAL)
	toggle_item->menu_item.previous_state = GTK_STATE_ACTIVE;
      else
	toggle_item->menu_item.previous_state = GTK_STATE_NORMAL;
    }

  toggle_item->menu_item.state.value = GTK_STATE_ACTIVATED;
  gtk_data_notify ((GtkData*) &toggle_item->menu_item.state);

  toggle_item->menu_item.state.value = GTK_STATE_NORMAL;
  gtk_data_notify ((GtkData*) &toggle_item->menu_item.state);

  g_function_leave ("gtk_menu_toggle_item_activate");
}

static void
gtk_menu_item_select (GtkWidget *widget)
{
  GtkMenuItem *menu_item;
  GtkMenu *menu;

  g_function_enter ("gtk_menu_item_select");

  g_assert (widget != NULL);
  menu_item = (GtkMenuItem*) widget;

  menu_item->state.value = GTK_STATE_SELECTED;
  gtk_data_notify ((GtkData*) &menu_item->state);

  if (menu_item->submenu)
    {
      if (GTK_WIDGET_VISIBLE (menu_item->submenu))
	g_error ("submenu already visible");

      menu = (GtkMenu*) menu_item->submenu;
      menu->parent = (GtkWidget*) menu_item;

      gtk_widget_show (menu_item->submenu);
    }

  g_function_leave ("gtk_menu_item_select");
}

static void
gtk_menu_item_deselect (GtkWidget *widget)
{
  GtkMenuItem *menu_item;
  GtkMenu *menu;

  g_function_enter ("gtk_menu_item_deselect");

  g_assert (widget != NULL);
  menu_item = (GtkMenuItem*) widget;

  if (menu_item->submenu)
    {
      menu = (GtkMenu*) menu_item->submenu;
      menu->parent = NULL;

      if (menu->active_menu_item)
	{
	  gtk_menu_item_deselect (menu->active_menu_item);
	  menu->active_menu_item = NULL;
	}

      gtk_widget_hide (menu_item->submenu);
    }

  menu_item->state.value = menu_item->previous_state;
  gtk_data_notify ((GtkData*) &menu_item->state);

  g_function_leave ("gtk_menu_item_deselect");
}


static gint
gtk_menu_item_state_update (GtkObserver *observer,
			    GtkData     *data)
{
  GtkMenuItem *menu_item;

  g_function_enter ("gtk_menu_item_state_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  menu_item = (GtkMenuItem*) observer->user_data;
  g_assert (menu_item != NULL);

  if (menu_item->state.value != GTK_STATE_ACTIVATED)
    gtk_widget_draw ((GtkWidget*) menu_item, NULL, FALSE);

  g_function_leave ("gtk_menu_item_state_update");
  return FALSE;
}

static void
gtk_menu_item_state_disconnect (GtkObserver *observer,
				GtkData     *data)
{
  g_function_enter ("gtk_menu_item_state_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_menu_item_state_disconnect");
}

static gint
gtk_menu_toggle_item_owner_update (GtkObserver *observer,
				   GtkData     *data)
{
  GtkMenuToggleItem *toggle_item;
  GtkDataWidget *owner;

  g_function_enter ("gtk_menu_toggle_item_owner_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  toggle_item = (GtkMenuToggleItem*) observer->user_data;
  g_assert (toggle_item != NULL);

  owner = (GtkDataWidget*) data;

  if (owner->widget == (GtkWidget*) toggle_item)
    {
      if (toggle_item->menu_item.state.value == GTK_STATE_NORMAL)
        {
          toggle_item->menu_item.previous_state = GTK_STATE_ACTIVE;
	  toggle_item->menu_item.state.value = GTK_STATE_ACTIVE;
          gtk_data_notify ((GtkData*) &toggle_item->menu_item.state);
        }
    }
  else
    {
      if (toggle_item->menu_item.state.value == GTK_STATE_ACTIVE)
        {
          toggle_item->menu_item.previous_state = GTK_STATE_NORMAL;
          toggle_item->menu_item.state.value = GTK_STATE_NORMAL;
          gtk_data_notify ((GtkData*) &toggle_item->menu_item.state);
        }
    }

  g_function_leave ("gtk_menu_toggle_item_owner_update");
  return FALSE;
}

static void
gtk_menu_toggle_item_owner_disconnect (GtkObserver *observer,
				       GtkData     *data)
{
  g_function_enter ("gtk_menu_toggle_item_owner_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_menu_toggle_item_owner_disconnect");
}


static void
gtk_menu_item_calc_accelerator_size (GtkMenuItem *menu_item)
{
  char buf[32];

  g_function_enter ("gtk_menu_item_calc_accelerator_size");

  g_assert (menu_item);
  g_assert (!(menu_item->accelerator_key && menu_item->submenu));

  if (menu_item->accelerator_key)
    {
      gtk_menu_item_calc_accelerator_text (menu_item, buf);
      menu_item->accelerator_size = gdk_string_width (menu_item->container.widget.style->font, buf) + 13;
    }
  else if (menu_item->submenu &&
	   (GTK_WIDGET_TYPE (menu_item->container.widget.parent) == gtk_get_menu_type ()))
    {
      menu_item->accelerator_size = 21;
    }
  else
    {
      menu_item->accelerator_size = 0;
    }

  g_function_leave ("gtk_menu_item_calc_accelerator_size");
}

static void
gtk_menu_item_calc_accelerator_text (GtkMenuItem *menu_item,
				     gchar       *buffer)
{
  g_function_enter ("gtk_menu_item_calc_accelerator_text");

  g_assert (menu_item);

  if (menu_item->accelerator_key)
    {
      buffer[0] = '\0';
      if (menu_item->accelerator_mods & GDK_SHIFT_MASK)
	{
	  strcat (buffer, SHIFT_TEXT);
	  strcat (buffer, SEPARATOR);
	}
      if (menu_item->accelerator_mods & GDK_CONTROL_MASK)
	{
	  strcat (buffer, CONTROL_TEXT);
	  strcat (buffer, SEPARATOR);
	}
      if (menu_item->accelerator_mods & GDK_MOD1_MASK)
	{
	  strcat (buffer, ALT_TEXT);
	  strcat (buffer, SEPARATOR);
	}
      strncat (buffer, &menu_item->accelerator_key, 1);
    }

  g_function_leave ("gtk_menu_item_calc_accelerator_text");
}
