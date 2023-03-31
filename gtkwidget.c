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
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkwindow.h"
#include "gtkprivate.h"


static void gtk_widget_set_parent_sensitive (GtkWidget *widget,
					     gpointer   client_data,
					     gpointer   call_data);


void
gtk_widget_unique_type (guint16 *type)
{
  static guint16 next_type = 1;

  g_function_enter ("gtk_widget_unique_type");

  g_assert (type != NULL);

  *type = next_type++;

  g_function_leave ("gtk_widget_unique_type");
}

gint
gtk_widget_destroy (GtkWidget *widget)
{
  GtkWidget *toplevel;
  gint return_val;

  g_function_enter ("gtk_widget_destroy");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->destroy);

  if (GTK_WIDGET_IN_CALL (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_NEED_DESTROY);
      return_val = FALSE;
    }
  else
    {
      if (GTK_WIDGET_HAS_FOCUS (widget))
	{
	  toplevel = gtk_widget_get_toplevel (widget);
	  if (toplevel)
	    gtk_window_set_focus (toplevel, NULL);
	}

      gtk_style_unref (widget->style);
      (* widget->function_table->destroy) (widget);

      return_val = TRUE;
    }

  g_function_leave ("gtk_widget_destroy");
  return return_val;
}

void
gtk_widget_show (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_show");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->show);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->show) (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_show");
}

void
gtk_widget_hide (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_hide");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->hide);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->hide) (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_hide");
}

void
gtk_widget_map (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_map");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->map);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      if (!GTK_WIDGET_MAPPED (widget))
	{
	  if (!GTK_WIDGET_REALIZED (widget))
	    gtk_widget_realize (widget);

	  (* widget->function_table->map) (widget);
	}

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_map");
}

void
gtk_widget_unmap (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_unmap");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->unmap);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->unmap) (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_unmap");
}

void
gtk_widget_realize (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_realize");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->realize);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      if (!GTK_WIDGET_REALIZED (widget))
	{
	  if (widget->parent && !GTK_WIDGET_REALIZED (widget->parent))
	    gtk_widget_realize ((GtkWidget*) widget->parent);

	  (* widget->function_table->realize) (widget);
	}

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_realize");
}

void
gtk_widget_draw (GtkWidget    *widget,
		 GdkRectangle *area,
		 gint          is_expose)
{
  GdkRectangle temp_area;
  gint old_value;

  g_function_enter ("gtk_widget_draw");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->draw);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      if (!area)
	{
	  temp_area.x = 0;
	  temp_area.y = 0;
	  temp_area.width = widget->allocation.width;
	  temp_area.height = widget->allocation.height;
	  area = &temp_area;
	}

      (* widget->function_table->draw) (widget, area, is_expose);

      gtk_widget_draw_focus (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_draw");
}

void
gtk_widget_draw_focus (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_draw_focus");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->draw_focus);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->draw_focus) (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_draw_focus");
}

gint
gtk_widget_event (GtkWidget *widget,
		  GdkEvent  *event)
{
  gint return_val;
  gint old_value;

  g_function_enter ("gtk_widget_event");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->event);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      return_val = (* widget->function_table->event) (widget, event);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_event");
  return return_val;
}

void
gtk_widget_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  gint old_value;

  g_function_enter ("gtk_widget_size_request");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->size_request);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->size_request) (widget, requisition);

      if (widget->user_allocation.width > 0)
	requisition->width = widget->user_allocation.width;
      if (widget->user_allocation.height > 0)
	requisition->height = widget->user_allocation.height;

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_size_request");
}

void
gtk_widget_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkAllocation real_allocation;
  gint old_value;

  g_function_enter ("gtk_widget_size_allocate");

  g_assert (widget != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->size_allocate);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      real_allocation = *allocation;
      if (widget->user_allocation.x != -1)
	real_allocation.x = widget->user_allocation.x;
      if (widget->user_allocation.y != -1)
	real_allocation.y = widget->user_allocation.y;

      (* widget->function_table->size_allocate) (widget, &real_allocation);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_size_allocate");
}

gint
gtk_widget_is_child (GtkWidget *widget,
		     GtkWidget *child)
{
  gint return_val;
  gint old_value;

  g_function_enter ("gtk_widget_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->is_child);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      return_val = (* widget->function_table->is_child) (widget, child);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_is_child");
  return return_val;
}

gint
gtk_widget_is_immediate_child (GtkWidget *widget,
			       GtkWidget *child)
{
  gint return_val;

  g_function_enter ("gtk_widget_is_immediate_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = (widget == (GtkWidget*) child->parent);

  g_function_leave ("gtk_widget_is_immediate_child");
  return return_val;
}

gint
gtk_widget_locate (GtkWidget  *widget,
		   GtkWidget **child,
		   gint        x,
		   gint        y)
{
  gint return_val;
  gint old_value;

  g_function_enter ("gtk_widget_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);
  g_assert (widget->function_table);
  g_assert (widget->function_table->locate);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      return_val = (* widget->function_table->locate) (widget, child, x, y);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_locate");
  return return_val;
}

void
gtk_widget_activate (GtkWidget *widget)
{
  gint old_value;

  g_function_enter ("gtk_widget_activate");

  g_assert (widget != NULL);
  g_assert (widget->function_table != NULL);
  g_assert (widget->function_table->activate != NULL);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->activate) (widget);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_activate");
}

void
gtk_widget_set_state (GtkWidget    *widget,
		      GtkStateType  state)
{
  gint old_value;

  g_function_enter ("gtk_widget_set_state");

  g_assert (widget != NULL);
  g_assert (widget->function_table != NULL);
  g_assert (widget->function_table->set_state != NULL);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      (* widget->function_table->set_state) (widget, state);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_set_state");
}

void
gtk_widget_set_sensitive (GtkWidget *widget,
			  gint       sensitive)
{
  gint old_val;

  g_function_enter ("gtk_widget_set_sensitive");

  g_assert (widget != NULL);

  old_val = GTK_WIDGET_IS_SENSITIVE (widget);

  if (sensitive)
    GTK_WIDGET_SET_FLAGS (widget, GTK_SENSITIVE);
  else
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_SENSITIVE);

  if (GTK_WIDGET_CONTAINER (widget))
    gtk_container_foreach ((GtkContainer*) widget, gtk_widget_set_parent_sensitive, (gpointer) sensitive);

  if (old_val != GTK_WIDGET_IS_SENSITIVE (widget))
    gtk_widget_draw (widget, NULL, FALSE);

  g_function_leave ("gtk_widget_set_sensitive");
}

void
gtk_widget_install_accelerator (GtkWidget           *widget,
				GtkAcceleratorTable *table,
				gchar                accelerator_key,
				guint8               accelerator_mods)
{
  gint old_value;

  g_function_enter ("gtk_widget_install_accelerator");

  g_assert (widget != NULL);
  g_assert (widget->function_table != NULL);
  g_assert (widget->function_table->install_accelerator != NULL);

  if (!GTK_WIDGET_NEED_DESTROY (widget))
    {
      old_value = GTK_WIDGET_IN_CALL (widget);
      GTK_WIDGET_SET_FLAGS (widget, GTK_IN_CALL);

      if ((* widget->function_table->install_accelerator) (widget, accelerator_key, accelerator_mods))
	gtk_accelerator_table_install (table, widget, accelerator_key, accelerator_mods);

      if (!old_value)
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_IN_CALL);
    }

  if (GTK_WIDGET_NEED_DESTROY (widget) && !GTK_WIDGET_IN_CALL (widget))
    gtk_widget_destroy (widget);

  g_function_leave ("gtk_widget_install_accelerator");
}

void
gtk_widget_remove_accelerator (GtkWidget           *widget,
			       GtkAcceleratorTable *table)
{
  g_function_enter ("gtk_widget_remove_accelerator");

  g_assert (widget != NULL);
  gtk_accelerator_table_remove (table, widget);

  g_function_leave ("gtk_widget_remove_accelerator");
}

void
gtk_widget_add_accelerator_table (GtkWidget           *widget,
				  GtkAcceleratorTable *table)
{
  g_function_enter ("gtk_widget_add_accelerator_table");

  g_assert (widget != NULL);
  g_assert (table != NULL);

  widget = gtk_widget_get_toplevel (widget);

  if (widget)
    gtk_window_add_accelerator_table (widget, table);

  g_function_leave ("gtk_widget_add_accelerator_table");
}

void
gtk_widget_grab_focus (GtkWidget *widget)
{
  GtkWidget *window;
  guint16 window_type;

  g_function_enter ("gtk_widget_grab_focus");

  g_assert (widget != NULL);

  window_type = gtk_get_window_type ();
  window = widget;

  while (window && (window->type != window_type))
    window = (GtkWidget*) window->parent;

  if (window && (window->type == window_type))
    gtk_window_set_focus (window, widget);

  g_function_leave ("gtk_widget_grab_focus");
}

void
gtk_widget_grab_default (GtkWidget *widget)
{
  GtkWidget *window;
  guint16 window_type;

  g_function_enter ("gtk_widget_grab_default");

  g_assert (widget != NULL);

  window_type = gtk_get_window_type ();
  window = widget;

  while (window && (window->type != window_type))
    window = (GtkWidget*) window->parent;

  if (window && (window->type == window_type))
    gtk_window_set_default (window, widget);

  g_function_leave ("gtk_widget_grab_default");
}

void
gtk_widget_move (GtkWidget *widget,
		 gint       x,
		 gint       y)
{
  g_function_enter ("gtk_widget_move");

  g_assert (widget != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    if (!GTK_WIDGET_NO_WINDOW (widget))
      gdk_window_move (widget->window, x, y);

  g_function_leave ("gtk_widget_move");
}

gint
gtk_widget_intersect (GtkWidget    *widget,
		      GdkRectangle *area,
		      GdkRectangle *dest)
{
  gint return_val;

  g_function_enter ("gtk_widget_intersect");

  g_assert (widget != NULL);
  g_assert (area != NULL);
  g_assert (dest != NULL);

  return_val = gdk_rectangle_intersect ((GdkRectangle*) &widget->allocation, area, dest);

  if (return_val && !GTK_WIDGET_NO_WINDOW (widget))
    {
      dest->x -= widget->allocation.x;
      dest->y -= widget->allocation.y;
    }

  g_function_leave ("gtk_widget_intersect");
  return return_val;
}

void
gtk_widget_reparent (GtkWidget *widget,
		     GtkWidget *new_parent)
{
  g_function_enter ("gtk_widget_reparent");

  g_assert (widget != NULL);
  g_assert (new_parent != NULL);
  g_assert (widget->parent != NULL);

  gtk_container_remove ((GtkWidget*) widget->parent, widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (GTK_WIDGET_REALIZED (new_parent) && !GTK_WIDGET_NO_WINDOW (widget))
	{
	  gdk_window_reparent (widget->window, widget->parent->widget.window, 0, 0);
	}
      else
	{
	  GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED|GTK_MAPPED);
	  if (!GTK_WIDGET_NO_WINDOW (widget))
	    gdk_window_destroy (widget->window);
	  widget->window = NULL;
	}
    }

  gtk_container_add (new_parent, widget);

  g_function_leave ("gtk_widget_reparent");
}

void
gtk_widget_popup (GtkWidget *widget,
		  gint       x,
		  gint       y)
{
  g_function_enter ("gtk_widget_popup");

  g_assert (widget != NULL);

  if (!GTK_WIDGET_VISIBLE (widget))
    {
      if (!GTK_WIDGET_REALIZED (widget))
	gtk_widget_realize (widget);
      gtk_widget_move (widget, x, y);
      gtk_widget_show (widget);
    }

  g_function_leave ("gtk_widget_popup");
}

void
gtk_widget_set_uposition (GtkWidget *widget,
			  gint       x,
			  gint       y)
{
  g_function_enter ("gtk_widget_set_uposition");

  g_assert (widget != NULL);

  widget->user_allocation.x = x;
  widget->user_allocation.y = y;

  if (GTK_WIDGET_REALIZED (widget) &&
      (widget->type == gtk_get_window_type ()) &&
      (widget->user_allocation.x != -1) &&
      (widget->user_allocation.y != -1))
    {
      gdk_window_set_position (widget->window,
			       widget->user_allocation.x,
			       widget->user_allocation.y);
      gdk_window_move (widget->window,
		       widget->user_allocation.x,
		       widget->user_allocation.y);
    }

  if (GTK_WIDGET_VISIBLE (widget) && widget->parent)
    gtk_widget_size_allocate (widget, &widget->allocation);

  g_function_leave ("gtk_widget_set_uposition");
}

void
gtk_widget_set_usize (GtkWidget *widget,
		      gint       width,
		      gint       height)
{
  g_function_enter ("gtk_widget_set_usize");

  g_assert (widget != NULL);

  widget->user_allocation.width = width;
  widget->user_allocation.height = height;

  if (GTK_WIDGET_VISIBLE (widget) && widget->parent)
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_widget_set_usize");
}

void
gtk_widget_set_defaults (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_set_defaults");

  g_assert (widget != NULL);

  widget->flags = GTK_SENSITIVE | GTK_PARENT_SENSITIVE;
  widget->style = gtk_peek_style ();
  widget->parent = NULL;
  widget->requisition.width = 0;
  widget->requisition.height = 0;
  widget->allocation.x = -1;
  widget->allocation.y = -1;
  widget->allocation.width = 1;
  widget->allocation.height = 1;
  widget->user_allocation.x = -1;
  widget->user_allocation.y = -1;
  widget->user_allocation.width = 0;
  widget->user_allocation.height = 0;
  widget->window = NULL;
  widget->user_data = NULL;

  gtk_style_ref (widget->style);

  g_function_leave ("gtk_widget_set_defaults");
}

void
gtk_widget_set_style (GtkWidget *widget,
		      GtkStyle  *style)
{
  g_function_enter ("gtk_widget_set_style");

  g_assert (widget != NULL);
  g_assert (style != NULL);

  if (style != widget->style)
    {
      gtk_style_unref (widget->style);

      widget->style = style;
      gtk_style_ref (widget->style);

      if (GTK_WIDGET_REALIZED (widget))
	widget->style = gtk_style_attach (widget->style, widget->window);

      gtk_widget_draw (widget, NULL, FALSE);
    }

  g_function_leave ("gtk_widget_set_style");
}

void
gtk_widget_set_user_data (GtkWidget *widget,
			  gpointer   data)
{
  g_function_enter ("gtk_widget_set_user_data");

  g_assert (widget != NULL);

  widget->user_data = data;

  g_function_leave ("gtk_widget_set_user_data");
}

gpointer
gtk_widget_get_user_data (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_get_user_data");

  g_assert (widget != NULL);

  g_function_leave ("gtk_widget_get_user_data");
  return widget->user_data;
}

GtkWidget*
gtk_widget_get_toplevel (GtkWidget *widget)
{
  guint16 window_type;

  g_function_enter ("gtk_widget_get_toplevel");

  g_assert (widget != NULL);

  window_type = gtk_get_window_type ();

  while (widget && (widget->type != window_type))
    widget = (GtkWidget*) widget->parent;

  if (!(widget && (widget->type == window_type)))
    widget = NULL;

  g_function_leave ("gtk_widget_get_toplevel");
  return widget;
}

void
gtk_widget_default_show (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_show");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_VISIBLE);
  if (widget->parent)
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_widget_default_show");
}

void
gtk_widget_default_hide (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_hide");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_VISIBLE);

      gtk_widget_unmap (widget);

      if (widget->parent)
	gtk_container_need_resize (widget->parent, widget);
    }

  g_function_leave ("gtk_widget_default_hide");
}

void
gtk_widget_default_map (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_map");

  g_assert (widget != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
      if (!GTK_WIDGET_NO_WINDOW (widget))
	gdk_window_show (widget->window);
    }

  g_function_leave ("gtk_widget_default_map");
}

void
gtk_widget_default_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_unmap");

  g_assert (widget != NULL);

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
      if (!GTK_WIDGET_NO_WINDOW (widget) && GTK_WIDGET_REALIZED (widget))
	gdk_window_hide (widget->window);
    }

  g_function_leave ("gtk_widget_default_unmap");
}

void
gtk_widget_default_draw_focus (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_draw_focus");
  g_assert (widget != NULL);
  g_function_leave ("gtk_widget_default_draw_focus");
}

gint
gtk_widget_default_event (GtkWidget *widget,
			  GdkEvent  *event)
{
  g_function_enter ("gtk_widget_default_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_widget_default_event");
  return FALSE;
}

void
gtk_widget_default_activate (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_activate");
  g_assert (widget != NULL);
  g_function_leave ("gtk_widget_default_activate");
}

void
gtk_widget_default_set_state (GtkWidget    *widget,
			      GtkStateType  state)
{
  g_function_enter ("gtk_widget_default_set_state");
  g_assert (widget != NULL);
  g_function_leave ("gtk_widget_default_set_state");
}

gint
gtk_widget_default_install_accelerator (GtkWidget *widget,
					gchar      accelerator_key,
					guint8     accelerator_mods)
{
  g_function_enter ("gtk_widget_default_install_accelerator");
  g_assert (widget != NULL);
  g_function_leave ("gtk_widget_default_install_accelerator");
  return FALSE;
}

void
gtk_widget_default_remove_accelerator (GtkWidget *widget)
{
  g_function_enter ("gtk_widget_default_remove_accelerator");
  g_assert (widget != NULL);
  g_function_leave ("gtk_widget_default_remove_accelerator");
}


static void
gtk_widget_set_parent_sensitive (GtkWidget *widget,
				 gpointer   client_data,
				 gpointer   call_data)
{
  g_function_enter ("gtk_widget_set_parent_sensitive");

  g_assert (widget != NULL);

  if (client_data)
    GTK_WIDGET_SET_FLAGS (widget, GTK_PARENT_SENSITIVE);
  else
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_PARENT_SENSITIVE);

  if (GTK_WIDGET_CONTAINER (widget))
    gtk_container_foreach ((GtkContainer*) widget, gtk_widget_set_parent_sensitive, client_data);

  g_function_leave ("gtk_widget_set_parent_sensitive");
}
