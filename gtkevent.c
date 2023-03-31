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
#include "gtkdraw.h"
#include "gtkevent.h"
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkEventWidget  GtkEventWidget;

struct _GtkEventWidget
{
  GtkContainer container;

  GtkWidget *child;
  GtkEventFunction event_function;
  GdkEventMask event_mask;
  unsigned int create_window : 1;
};


static void  gtk_event_widget_destroy       (GtkWidget       *widget);
static void  gtk_event_widget_map           (GtkWidget       *widget);
static void  gtk_event_widget_unmap         (GtkWidget       *widget);
static void  gtk_event_widget_realize       (GtkWidget       *widget);
static void  gtk_event_widget_draw          (GtkWidget       *widget,
					     GdkRectangle    *area,
					     gint             is_expose);
static gint  gtk_event_widget_event         (GtkWidget       *widget,
					     GdkEvent        *event);
static void  gtk_event_widget_size_request  (GtkWidget       *widget,
					     GtkRequisition  *requisition);
static void  gtk_event_widget_size_allocate (GtkWidget       *widget,
					     GtkAllocation   *allocation);
static gint  gtk_event_widget_is_child      (GtkWidget       *widget,
					     GtkWidget       *child);
static gint  gtk_event_widget_locate        (GtkWidget       *widget,
					     GtkWidget      **child,
					     gint             x,
					     gint             y);
static void  gtk_event_widget_set_state     (GtkWidget       *widget,
					     GtkStateType     state);
static void  gtk_event_widget_add           (GtkContainer    *container,
					     GtkWidget       *widget);
static void  gtk_event_widget_remove        (GtkContainer    *container,
					     GtkWidget       *widget);


static GtkWidgetFunctions event_widget_functions =
{
  gtk_event_widget_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_event_widget_map,
  gtk_event_widget_unmap,
  gtk_event_widget_realize,
  gtk_event_widget_draw,
  gtk_widget_default_draw_focus,
  gtk_event_widget_event,
  gtk_event_widget_size_request,
  gtk_event_widget_size_allocate,
  gtk_event_widget_is_child,
  gtk_event_widget_locate,
  gtk_widget_default_activate,
  gtk_event_widget_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions event_widget_container_functions =
{
  gtk_event_widget_add,
  gtk_event_widget_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
};


GtkWidget*
gtk_event_widget_new (GtkEventFunction event_function,
		      GdkEventMask     event_mask,
		      gint             create_window)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_new");

  event_widget = g_new (GtkEventWidget, 1);

  event_widget->container.widget.type = gtk_get_event_widget_type ();
  event_widget->container.widget.function_table = &event_widget_functions;
  event_widget->container.function_table = &event_widget_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) event_widget);
  gtk_container_set_defaults ((GtkWidget*) event_widget);

  event_widget->child = NULL;
  event_widget->event_function = event_function;
  event_widget->event_mask = event_mask;
  event_widget->create_window = (create_window != FALSE);

  if (!event_widget->create_window)
    GTK_WIDGET_SET_FLAGS (event_widget, GTK_NO_WINDOW);

  g_function_leave ("gtk_event_widget_new");
  return ((GtkWidget*) event_widget);
}

guint16
gtk_get_event_widget_type ()
{
  static guint16 event_widget_type = 0;

  g_function_enter ("gtk_get_event_widget_type");

  if (!event_widget_type)
    gtk_widget_unique_type (&event_widget_type);

  g_function_leave ("gtk_get_event_widget_type");
  return event_widget_type;
}


static void
gtk_event_widget_destroy (GtkWidget *widget)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_destroy");

  g_assert (widget != NULL);
  event_widget = (GtkEventWidget*) widget;

  if (event_widget->child)
    if (!gtk_widget_destroy (event_widget->child))
      event_widget->child->parent = NULL;
  if (!GTK_WIDGET_NO_WINDOW (widget) && event_widget->container.widget.window)
    gdk_window_destroy (event_widget->container.widget.window);
  g_free (event_widget);

  g_function_leave ("gtk_event_widget_destroy");
}

static void
gtk_event_widget_map (GtkWidget *widget)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  if (!GTK_WIDGET_NO_WINDOW (widget))
    gdk_window_show (widget->window);

  event_widget = (GtkEventWidget*) widget;

  if (event_widget->child &&
      GTK_WIDGET_VISIBLE (event_widget->child) &&
      !GTK_WIDGET_MAPPED (event_widget->child))
    gtk_widget_map (event_widget->child);

  g_function_leave ("gtk_event_widget_map");
}

static void
gtk_event_widget_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_event_widget_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  if (!GTK_WIDGET_NO_WINDOW (widget))
    gdk_window_hide (widget->window);

  g_function_leave ("gtk_event_widget_unmap");
}

static void
gtk_event_widget_realize (GtkWidget *widget)
{
  GtkEventWidget *event_widget;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_function_enter ("gtk_event_widget_realize");

  g_assert (widget != NULL);

  event_widget = (GtkEventWidget*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (event_widget->create_window)
    {
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.visual = gtk_peek_visual ();
      attributes.colormap = gtk_peek_colormap ();
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.event_mask = event_widget->event_mask;
      attributes_mask = GDK_WA_X | GDK_WA_Y;

      event_widget->container.widget.window = gdk_window_new (widget->parent->widget.window,
							      &attributes, attributes_mask);
      gdk_window_set_user_data (event_widget->container.widget.window, event_widget);

      event_widget->container.widget.style = gtk_style_attach (event_widget->container.widget.style,
							       event_widget->container.widget.window);
      gdk_window_set_background (event_widget->container.widget.window,
				 &event_widget->container.widget.style->background[GTK_STATE_NORMAL]);
    }
  else
    {
      widget->window = widget->parent->widget.window;
    }

  g_function_leave ("gtk_event_widget_realize");
}

static void
gtk_event_widget_draw (GtkWidget    *widget,
		       GdkRectangle *area,
		       gint          is_expose)
{
  GtkEventWidget *event_widget;
  GdkRectangle child_area;

  g_function_enter ("gtk_event_widget_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      event_widget = (GtkEventWidget*) widget;

      if (event_widget->child)
	if (!is_expose || GTK_WIDGET_NO_WINDOW (event_widget->child))
	  if (gtk_widget_intersect (event_widget->child, area, &child_area))
	    gtk_widget_draw (event_widget->child, &child_area, is_expose);
    }

  g_function_leave ("gtk_event_widget_draw");
}

static gint
gtk_event_widget_event (GtkWidget *widget,
			GdkEvent  *event)
{
  GtkEventWidget *event_widget;
  gint return_val;

  g_function_enter ("gtk_event_widget_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  event_widget = (GtkEventWidget*) widget;
  if (event_widget->event_function)
    return_val = (* event_widget->event_function) (widget, event);

  g_function_leave ("gtk_event_widget_event");
  return return_val;
}

static void
gtk_event_widget_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  event_widget = (GtkEventWidget*) widget;

  if (event_widget->child)
    {
      event_widget->child->requisition.width = 0;
      event_widget->child->requisition.height = 0;

      gtk_widget_size_request (event_widget->child, &event_widget->child->requisition);

      requisition->width = (event_widget->child->requisition.width +
			    event_widget->container.border_width);
      requisition->height = (event_widget->child->requisition.height +
			     event_widget->container.border_width * 2);
    }
  else
    {
      requisition->width = event_widget->container.border_width * 2;
      requisition->height = event_widget->container.border_width * 2;
    }

  g_function_leave ("gtk_event_widget_size_request");
}

static void
gtk_event_widget_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  GtkEventWidget *event_widget;
  GtkAllocation child_allocation;

  g_function_enter ("gtk_event_widget_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  event_widget = (GtkEventWidget*) widget;

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_NO_WINDOW (widget))
    {
      gdk_window_move (widget->window,
		       allocation->x,
		       allocation->y);
      gdk_window_set_size (widget->window,
			   allocation->width,
			   allocation->height);
    }

  if (event_widget->child)
    {
      child_allocation.x = event_widget->container.border_width;
      child_allocation.y = event_widget->container.border_width;
      child_allocation.width = allocation->width - child_allocation.x * 2;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      if (GTK_WIDGET_NO_WINDOW (widget))
	{
	  child_allocation.x += allocation->x;
	  child_allocation.y += allocation->y;
	}

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (event_widget->child, &child_allocation);
    }

  g_function_leave ("gtk_event_widget_size_allocate");
}

static gint
gtk_event_widget_is_child (GtkWidget *widget,
			   GtkWidget *child)
{
  GtkEventWidget *event_widget;
  gint return_val;

  g_function_enter ("gtk_event_widget_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  event_widget = (GtkEventWidget*) widget;

  return_val = FALSE;
  if (event_widget->child == child)
    return_val = TRUE;
  else if (event_widget->child)
    return_val = gtk_widget_is_child (event_widget->child, child);

  g_function_leave ("gtk_event_widget_is_child");
  return return_val;
}

static gint
gtk_event_widget_locate (GtkWidget  *widget,
			 GtkWidget **child,
			 gint        x,
			 gint        y)
{
  g_function_enter ("gtk_event_widget_locate");
  g_warning ("gtk_event_widget_locate: UNFINISHED");
  g_function_leave ("gtk_event_widget_locate");
  return FALSE;
}

static void
gtk_event_widget_set_state (GtkWidget    *widget,
			    GtkStateType  state)
{
  g_function_enter ("gtk_event_widget_set_state");
  g_warning ("gtk_event_widget_set_state: UNFINISHED");
  g_function_leave ("gtk_event_widget_set_state");
}

static void
gtk_event_widget_add (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_add");

  g_assert (container != NULL);
  event_widget = (GtkEventWidget*) container;

  if (event_widget->child)
    g_error ("event_widget already has a child");
  else
    event_widget->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_event_widget_add");
}

static void
gtk_event_widget_remove (GtkContainer *container,
			 GtkWidget    *widget)
{
  GtkEventWidget *event_widget;

  g_function_enter ("gtk_event_widget_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  event_widget = (GtkEventWidget*) container;

  if (event_widget->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!event_widget->child)
    g_error ("event_widget has no child to remove");

  event_widget->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_event_widget_remove");
}
