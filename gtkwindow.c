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
#include "gtkaccelerator.h"
#include "gtkcontainer.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkwindow.h"
#include "gtkprivate.h"


typedef struct _GtkWindow      GtkWindow;

struct _GtkWindow
{
  GtkContainer container;

  gchar *title;
  GtkWindowType type;
  GtkWidget *child;
  GtkWidget *focus_widget;
  GtkWidget *default_widget;
  gint need_resize;

  GList *accelerator_tables;

  GtkWindowResizeHook resize;
};


static void  gtk_window_destroy       (GtkWidget       *widget);
static void  gtk_window_show          (GtkWidget       *widget);
static void  gtk_window_hide          (GtkWidget       *widget);
static void  gtk_window_map           (GtkWidget       *widget);
static void  gtk_window_unmap         (GtkWidget       *widget);
static void  gtk_window_realize       (GtkWidget       *widget);
static void  gtk_window_draw          (GtkWidget       *widget,
				       GdkRectangle    *area,
				       gint             is_expose);
static gint  gtk_window_event         (GtkWidget       *widget,
				       GdkEvent        *event);
static void  gtk_window_size_request  (GtkWidget       *widget,
				       GtkRequisition  *requisition);
static void  gtk_window_size_allocate (GtkWidget       *widget,
				       GtkAllocation   *allocation);
static gint  gtk_window_is_child      (GtkWidget       *widget,
				       GtkWidget       *child);
static gint  gtk_window_locate        (GtkWidget       *widget,
				       GtkWidget      **child,
				       gint             x,
				       gint             y);
static void  gtk_window_add           (GtkContainer    *container,
				       GtkWidget       *widget);
static void  gtk_window_remove        (GtkContainer    *container,
				       GtkWidget       *widget);
static void  gtk_window_need_resize   (GtkContainer    *container,
				       GtkWidget       *widget);
static void  gtk_window_foreach       (GtkContainer    *container,
				       GtkCallback      callback,
				       gpointer         callback_data);
static void  gtk_window_resize        (GtkWindow       *window);

static gint  gtk_window_check_accelerator (GtkWindow  *window,
					   gchar       accelerator_key,
					   guint8      accelerator_mods);


static GtkWidgetFunctions window_widget_functions =
{
  gtk_window_destroy,
  gtk_window_show,
  gtk_window_hide,
  gtk_window_map,
  gtk_window_unmap,
  gtk_window_realize,
  gtk_window_draw,
  gtk_widget_default_draw_focus,
  gtk_window_event,
  gtk_window_size_request,
  gtk_window_size_allocate,
  gtk_window_is_child,
  gtk_window_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions window_container_functions =
{
  gtk_window_add,
  gtk_window_remove,
  gtk_window_need_resize,
  gtk_container_default_focus_advance,
  gtk_window_foreach,
};


GtkWidget*
gtk_window_new (gchar         *title,
		GtkWindowType  type)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_new");

  window = g_new (GtkWindow, 1);

  window->container.widget.type = gtk_get_window_type ();
  window->container.widget.function_table = &window_widget_functions;
  window->container.function_table = &window_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) window);
  gtk_container_set_defaults ((GtkWidget*) window);

  window->container.widget.parent = gtk_root;

  window->title = g_strdup (title);
  window->type = type;
  window->child = NULL;
  window->focus_widget = NULL;
  window->default_widget = NULL;
  window->need_resize = FALSE;
  window->accelerator_tables = NULL;
  window->resize = NULL;

  g_function_leave ("gtk_window_new");
  return ((GtkWidget*) window);
}

void
gtk_window_set_focus (GtkWidget *widget,
		      GtkWidget *focus)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_set_focus");

  g_assert (widget != NULL);
  window = (GtkWindow*) widget;

  if (window->focus_widget)
    {
      GTK_WIDGET_UNSET_FLAGS (window->focus_widget, GTK_HAS_FOCUS);
      gtk_widget_draw_focus (window->focus_widget);
    }

  window->focus_widget = focus;
  if (window->focus_widget)
    {
      GTK_WIDGET_SET_FLAGS (window->focus_widget, GTK_HAS_FOCUS);
      gtk_widget_draw_focus (window->focus_widget);
    }

  g_function_leave ("gtk_window_set_focus");
}

void
gtk_window_set_default (GtkWidget *widget,
			GtkWidget *defaultw)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_set_default");

  g_assert (widget != NULL);
  window = (GtkWindow*) widget;

  if (window->default_widget)
    {
      GTK_WIDGET_UNSET_FLAGS (window->default_widget, GTK_HAS_DEFAULT);
      /*      gtk_widget_draw_focus (window->default_widget); */
    }

  window->default_widget = defaultw;
  if (window->default_widget)
    {
      GTK_WIDGET_SET_FLAGS (window->default_widget, GTK_HAS_DEFAULT);
      /*      gtk_widget_draw_focus (window->default_widget); */
    }

  g_function_leave ("gtk_window_set_default");
}

void
gtk_window_add_accelerator_table (GtkWidget           *widget,
				  GtkAcceleratorTable *table)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_add_accelerator_table");

  g_assert (widget != NULL);
  g_assert (table != NULL);

  window = (GtkWindow*) widget;
  gtk_accelerator_table_ref (table);
  window->accelerator_tables = g_list_prepend (window->accelerator_tables, table);

  g_function_leave ("gtk_window_add_accelerator_table");
}

guint16
gtk_get_window_type ()
{
  static guint16 window_type = 0;

  g_function_enter ("gtk_get_window_type");

  if (!window_type)
    gtk_widget_unique_type (&window_type);

  g_function_leave ("gtk_get_window_type");
  return window_type;
}

void
gtk_window_set_resize_hook (GtkWidget           *widget,
			    GtkWindowResizeHook  resize)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_set_resize_hook");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_set_resize_hook");

  window = (GtkWindow*) widget;
  window->resize = resize;

  g_function_leave ("gtk_window_set_resize_hook");
}

static void
gtk_window_destroy (GtkWidget *widget)
{
  GtkWindow *window;
  GtkAcceleratorTable *table;
  GList *temp;

  g_function_enter ("gtk_window_destroy");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_destroy");

  window = (GtkWindow*) widget;

  temp = window->accelerator_tables;
  while (temp)
    {
      table = temp->data;
      temp = temp->next;

      gtk_accelerator_table_unref (table);
    }
  g_list_free (window->accelerator_tables);

  if (window->child)
    if (!gtk_widget_destroy (window->child))
      window->child->parent = NULL;
  if (widget->window)
    gdk_window_destroy (widget->window);

  g_free (window->title);
  g_free (window);

  g_function_leave ("gtk_window_destroy");
}

static void
gtk_window_show (GtkWidget *widget)
{
  g_function_enter ("gtk_window_show");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_show");

  GTK_WIDGET_SET_FLAGS (widget, GTK_VISIBLE);
  gtk_widget_map (widget);

  g_function_leave ("gtk_window_show");
}

static void
gtk_window_hide (GtkWidget *widget)
{
  g_function_enter ("gtk_window_hide");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_hide");

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_VISIBLE);
  gtk_widget_unmap (widget);

  g_function_leave ("gtk_window_hide");
}

static void
gtk_window_map (GtkWidget *widget)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_map");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_map");

  window = (GtkWindow*) widget;
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_UNMAPPED);

  gtk_window_resize (window);

  if (window->child &&
      GTK_WIDGET_VISIBLE (window->child) &&
      !GTK_WIDGET_MAPPED (window->child))
    gtk_widget_map (window->child);

  if ((widget->user_allocation.x != -1) &&
      (widget->user_allocation.y != -1))
    {
      gdk_window_set_position (widget->window,
			       widget->user_allocation.x,
			       widget->user_allocation.y);
      gdk_window_move (widget->window,
		       widget->user_allocation.x,
		       widget->user_allocation.y);
    }

  gdk_window_show (widget->window);

  g_function_leave ("gtk_window_map");
}

static void
gtk_window_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_window_unmap");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_unmap");

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  GTK_WIDGET_SET_FLAGS (widget, GTK_UNMAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_window_unmap");
}

static void
gtk_window_realize (GtkWidget *widget)
{
  GtkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_function_enter ("gtk_window_realize");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_realize");

  window = (GtkWindow*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  switch (window->type)
    {
    case GTK_WINDOW_TOPLEVEL:
      attributes.window_type = GDK_WINDOW_TOPLEVEL;
      break;
    case GTK_WINDOW_DIALOG:
      attributes.window_type = GDK_WINDOW_DIALOG;
      break;
    case GTK_WINDOW_POPUP:
      attributes.window_type = GDK_WINDOW_TEMP;
      break;
    }

  attributes.title = window->title;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_peek_visual ();
  attributes.colormap = gtk_peek_colormap ();
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_KEY_PRESS_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK |
			   GDK_STRUCTURE_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  attributes_mask |= (window->title) ? (GDK_WA_TITLE) : (0);

  window->container.widget.window = gdk_window_new (NULL, &attributes, attributes_mask);
  gdk_window_set_user_data (window->container.widget.window, window);

  window->container.widget.style = gtk_style_attach (window->container.widget.style,
						     window->container.widget.window);
  gdk_window_set_background (window->container.widget.window,
			     &window->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_window_realize");
}

static void
gtk_window_draw (GtkWidget    *widget,
		 GdkRectangle *area,
		 gint          is_expose)
{
  GtkWindow *window;
  GdkRectangle child_area;

  g_function_enter ("gtk_window_draw");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_draw");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      window = (GtkWindow*) widget;

      if (window->child)
	if (!is_expose || GTK_WIDGET_NO_WINDOW (window->child))
	  if (gtk_widget_intersect (window->child, area, &child_area))
	    gtk_widget_draw (window->child, &child_area, is_expose);
    }

  g_function_leave ("gtk_window_draw");
}

static gint
gtk_window_event (GtkWidget *widget,
		  GdkEvent  *event)
{
  GtkWindow *window;
  GtkAllocation allocation;
  GtkDirectionType direction;

  g_function_enter ("gtk_window_event");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_event");

  if (!event)
    g_error ("passed NULL event to gtk_window_event");

  window = (GtkWindow*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!GTK_WIDGET_UNMAPPED (widget))
	{
	  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	  gtk_widget_draw (widget, &event->expose.area, TRUE);
	}
      break;

    case GDK_RESIZE:
      allocation.x = 0;
      allocation.y = 0;
      allocation.width = widget->window->width;
      allocation.height = widget->window->height;

      gtk_widget_size_allocate (widget, &allocation);

      if (window->child &&
	  GTK_WIDGET_VISIBLE (window->child) &&
	  !GTK_WIDGET_MAPPED (window->child))
	gtk_widget_map (window->child);
      break;

    case GDK_KEY_PRESS:
      if (!((event->key.keyval >= 0x20) && (event->key.keyval <= 0x7e) &&
	    gtk_window_check_accelerator (window, event->key.keyval, event->key.state)))
	{
	  if (!window->focus_widget || !gtk_widget_event (window->focus_widget, event))
	    {
	      switch (event->key.keyval)
		{
		case XK_space:
		  g_message ("focus widget activate");
		  if (window->focus_widget)
		    gtk_widget_activate (window->focus_widget);
		  break;
		case XK_Return:
		case XK_KP_Enter:
		  g_message ("default widget activate");
		  if (window->default_widget)
		    gtk_widget_activate (window->default_widget);
		  break;
		case XK_Up:
		  g_message ("focus up");

		  /*
		  gtk_container_focus_advance ((GtkContainer*) window->child,
					       &window->focus_widget, GTK_DIR_UP);
		  if (!window->focus_widget)
		    gtk_container_focus_advance ((GtkContainer*) window->child,
						 &window->focus_widget, GTK_DIR_UP);
		  if (window->focus_widget)
		    gtk_window_set_focus (widget, window->focus_widget);
		  */
		  break;
		case XK_Down:
		  g_message ("focus down");

		  /*
		  gtk_container_focus_advance ((GtkContainer*) window->child,
					       &window->focus_widget, GTK_DIR_DOWN);
		  if (!window->focus_widget)
		    gtk_container_focus_advance ((GtkContainer*) window->child,
						 &window->focus_widget, GTK_DIR_DOWN);
		  if (window->focus_widget)
		    gtk_window_set_focus (widget, window->focus_widget);
		  */
		  break;
		case XK_Left:
		  g_message ("focus left");

		  /*
		  gtk_container_focus_advance ((GtkContainer*) window->child,
					       &window->focus_widget, GTK_DIR_LEFT);
		  if (!window->focus_widget)
		    gtk_container_focus_advance ((GtkContainer*) window->child,
						 &window->focus_widget, GTK_DIR_LEFT);
		  if (window->focus_widget)
		    gtk_window_set_focus (widget, window->focus_widget);
		  */
		  break;
		case XK_Right:
		  g_message ("focus right");

		  /*
		  gtk_container_focus_advance ((GtkContainer*) window->child,
					       &window->focus_widget, GTK_DIR_RIGHT);
		  if (!window->focus_widget)
		    gtk_container_focus_advance ((GtkContainer*) window->child,
						 &window->focus_widget, GTK_DIR_RIGHT);
		  if (window->focus_widget)
		    gtk_window_set_focus (widget, window->focus_widget);
		  */
		  break;
		case XK_Tab:
		  g_message ("tab pressed");

		  /*
		  if (window->child)
		    {
		      if (GTK_WIDGET_CONTAINER (window->child))
			{
			  if (event->key.state & GDK_SHIFT_MASK)
			    direction = GTK_DIR_TAB_BACKWARD;
			  else
			    direction = GTK_DIR_TAB_FORWARD;

			  gtk_container_focus_advance ((GtkContainer*) window->child,
						       &window->focus_widget, direction);
			  if (!window->focus_widget)
			    gtk_container_focus_advance ((GtkContainer*) window->child,
							 &window->focus_widget, direction);
			  if (window->focus_widget)
			    gtk_window_set_focus (widget, window->focus_widget);
			}
		      else if (GTK_WIDGET_CAN_FOCUS (window->child) &&
			       !GTK_WIDGET_HAS_FOCUS (window->child))
			{
			  gtk_window_set_focus (widget, window->child);
			}
		    }
		  */
		  break;
		}
	    }
	}
      break;

    case GDK_KEY_RELEASE:
      if (window->focus_widget)
	gtk_widget_event (window->focus_widget, event);
      break;

    case GDK_ENTER_NOTIFY:
      if (event->any.window == widget->window)
	if (window->focus_widget && (event->crossing.detail != GDK_NOTIFY_INFERIOR))
	  if (!GTK_WIDGET_HAS_FOCUS (window->focus_widget))
	    {
	      GTK_WIDGET_SET_FLAGS (window->focus_widget, GTK_HAS_FOCUS);
	      gtk_widget_draw_focus (window->focus_widget);
	    }
      break;

    case GDK_LEAVE_NOTIFY:
      if (event->any.window == widget->window)
	if (window->focus_widget && (event->crossing.detail != GDK_NOTIFY_INFERIOR))
	  if (GTK_WIDGET_HAS_FOCUS (window->focus_widget))
	    {
	      GTK_WIDGET_UNSET_FLAGS (window->focus_widget, GTK_HAS_FOCUS);
	      gtk_widget_draw_focus (window->focus_widget);
	    }
      break;

    case GDK_FOCUS_CHANGE:
      g_message ("%d", event->focus_change.in);
      break;

    case GDK_MAP:
      break;

    case GDK_UNMAP:
      break;

    default:
      break;
    }

  g_function_leave ("gtk_window_event");
  return FALSE;
}

static void
gtk_window_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_size_request");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_size_request");

  if (!requisition)
    g_error ("passed NULL requisition to gtk_window_size_request");

  window = (GtkWindow*) widget;

  if (GTK_WIDGET_VISIBLE (widget) && window->child)
    {
      requisition->width = 0;
      requisition->height = 0;

      if (window->child)
	{
	  window->child->requisition.width = 0;
	  window->child->requisition.height = 0;

	  gtk_widget_size_request (window->child, &window->child->requisition);

	  requisition->width = MAX (requisition->width,
				    window->child->requisition.width);
	  requisition->height += window->child->requisition.height;
	}

      requisition->width += window->container.border_width * 2;
      requisition->height += window->container.border_width * 2;
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (widget))
	window->need_resize = TRUE;

      requisition->width = window->container.border_width * 2;
      requisition->height = window->container.border_width * 2;
    }

  if (GTK_WIDGET_VISIBLE (widget))
    gdk_window_set_sizes (widget->window,
                          requisition->width,
                          requisition->height,
                          0, 0, GDK_MIN_SIZE);

  g_function_leave ("gtk_window_size_request");
}

static void
gtk_window_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkWindow *window;
  GtkAllocation child_allocation;
  gint x, y;

  g_function_enter ("gtk_window_size_allocate");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_size_allocate");

  if (!allocation)
    g_error ("passed NULL allocation to gtk_window_size_allocate");

  window = (GtkWindow*) widget;
  widget->allocation = *allocation;

  x = window->container.border_width;
  y = window->container.border_width;

  if (window->child)
    {
      child_allocation.x = x;
      child_allocation.y = y;
      child_allocation.width = allocation->width - child_allocation.x * 2;
      child_allocation.height = allocation->height - y - window->container.border_width;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (window->child, &child_allocation);
    }

  g_function_leave ("gtk_window_size_allocate");
}

static gint
gtk_window_is_child (GtkWidget *widget,
		     GtkWidget *child)
{
  GtkWindow *window;
  gint return_val;

  g_function_enter ("gtk_window_is_child");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_is_child");

  if (!child)
    g_error ("passed NULL child to gtk_window_is_child");

  window = (GtkWindow*) widget;

  return_val = FALSE;
  if (window->child == child)
    return_val = TRUE;
  else if (window->child)
    return_val = gtk_widget_is_child (window->child, child);

  g_function_leave ("gtk_window_is_child");
  return return_val;
}

static gint
gtk_window_locate (GtkWidget  *widget,
		   GtkWidget **child,
		   gint        x,
		   gint        y)
{
  g_function_enter ("gtk_window_locate");
  g_warning ("gtk_window_locate: UNFINISHED");
  g_function_leave ("gtk_window_locate");
  return FALSE;
}

static void
gtk_window_add (GtkContainer *container,
		GtkWidget    *widget)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_add");

  if (!container)
    g_error ("passed NULL widget to gtk_window_add");

  window = (GtkWindow*) container;

  if (window->child)
    {
      g_error ("window already has a child");
    }
  else
    {
      window->child = widget;
      if (GTK_WIDGET_VISIBLE (window->child))
	{
	  window->need_resize = TRUE;
	  gtk_window_resize (window);
	}
    }

  g_function_leave ("gtk_window_add");
}

static void
gtk_window_remove (GtkContainer *container,
		   GtkWidget    *widget)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_remove");

  if (!container)
    g_error ("passed NULL container to gtk_window_remove");

  if (!widget)
    g_error ("passed NULL widget to gtk_window_remove");

  window = (GtkWindow*) container;

  if (window->child == widget)
    window->child = NULL;
  else
    g_error ("attempted to remove widget which wasn't a child");

  if (GTK_WIDGET_VISIBLE (widget))
    {
      gtk_widget_hide (widget);

      window->need_resize = TRUE;
      gtk_window_resize (window);
    }

  g_function_leave ("gtk_window_remove");
}

static void
gtk_window_need_resize (GtkContainer *container,
			GtkWidget    *widget)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_need_resize");

  if (!container)
    g_error ("passed NULL container to gtk_window_need_resize");

  window = (GtkWindow*) container;

  if (window->child != widget)
    g_error ("invalid child");

  window->need_resize = TRUE;
  if (GTK_WIDGET_VISIBLE (container))
    {
      gtk_window_resize (window);

      if (GTK_WIDGET_VISIBLE (widget))
	{
	  if (GTK_WIDGET_REALIZED (container) &&
	      !GTK_WIDGET_REALIZED (widget))
	    gtk_widget_realize (widget);

	  if (GTK_WIDGET_MAPPED (container) &&
	      !GTK_WIDGET_MAPPED (widget))
	    gtk_widget_map (widget);
	}
    }

  g_function_leave ("gtk_window_need_resize");
}

static void
gtk_window_foreach (GtkContainer *container,
		    GtkCallback   callback,
		    gpointer      callback_data)
{
  GtkWindow *window;

  g_function_enter ("gtk_window_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  window = (GtkWindow*) container;

  if (window->child)
    (* callback) (window->child, callback_data, NULL);

  g_function_leave ("gtk_window_foreach");
}

static void
gtk_window_resize (GtkWindow *window)
{
  GtkWidget *window_widget;
  GdkWindow *gdk_window;
  GtkAllocation allocation;
  gint x, y, width, height;

  g_function_enter ("gtk_window_resize");

  if (!window)
    g_error ("passed NULL window to gtk_window_resize");

  if (window->need_resize && GTK_WIDGET_REALIZED (window))
    {
      window->need_resize = FALSE;

      window_widget = (GtkWidget*) window;

      window_widget->requisition.width = 0;
      window_widget->requisition.height = 0;

      gtk_widget_size_request (window_widget, &window_widget->requisition);

      x = -1;
      y = -1;
      width = window_widget->requisition.width;
      height = window_widget->requisition.height;

      if (window->resize)
	(* window->resize) (window_widget, &x, &y, &width, &height);

      gdk_window = window->container.widget.window;

      if ((x != -1) && (y != -1))
	gdk_window_move (gdk_window, x, y);

      if ((gdk_window->width != window_widget->requisition.width) ||
	  (gdk_window->height != window_widget->requisition.height))
	{
	  gdk_window_set_size (gdk_window,
			       window_widget->requisition.width,
			       window_widget->requisition.height);
	}
      else
	{
	  allocation.x = 0;
	  allocation.y = 0;
	  allocation.width = gdk_window->width;
	  allocation.height = gdk_window->height;

	  gtk_widget_size_allocate ((GtkWidget*) window, &allocation);
	}
    }

  g_function_leave ("gtk_window_resize");
}

static gint
gtk_window_check_accelerator (GtkWindow  *window,
			      gchar       accelerator_key,
			      guint8      accelerator_mods)
{
  GtkAcceleratorTable *table;
  GList *temp;
  gint return_val;

  g_function_enter ("gtk_window_check_accelerator");

  g_assert (window != NULL);

  return_val = FALSE;
  temp = window->accelerator_tables;

  while (temp)
    {
      table = temp->data;
      temp = temp->next;

      if (gtk_accelerator_table_check (table, accelerator_key, accelerator_mods))
	{
	  return_val = TRUE;
	  break;
	}
    }

  g_function_leave ("gtk_window_check_accelerator");
  return return_val;
}
