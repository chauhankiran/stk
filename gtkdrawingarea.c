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
#include "gtkdrawingarea.h"
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkDrawingArea  GtkDrawingArea;

struct _GtkDrawingArea
{
  GtkWidget widget;

  gint width;
  gint height;
  GtkEventFunction event_function;
  GdkEventMask event_mask;
};


static void  gtk_drawing_area_destroy       (GtkWidget       *widget);
static void  gtk_drawing_area_realize       (GtkWidget       *widget);
static void  gtk_drawing_area_draw          (GtkWidget       *widget,
					     GdkRectangle    *area,
					     gint             is_expose);
static gint  gtk_drawing_area_event         (GtkWidget       *widget,
					     GdkEvent        *event);
static void  gtk_drawing_area_size_request  (GtkWidget       *widget,
					     GtkRequisition  *requisition);
static void  gtk_drawing_area_size_allocate (GtkWidget       *widget,
					     GtkAllocation   *allocation);
static gint  gtk_drawing_area_is_child      (GtkWidget       *widget,
					     GtkWidget       *child);
static gint  gtk_drawing_area_locate        (GtkWidget       *widget,
					     GtkWidget      **child,
					     gint             x,
					     gint             y);


static GtkWidgetFunctions drawing_area_widget_functions =
{
  gtk_drawing_area_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_drawing_area_realize,
  gtk_drawing_area_draw,
  gtk_widget_default_draw_focus,
  gtk_drawing_area_event,
  gtk_drawing_area_size_request,
  gtk_drawing_area_size_allocate,
  gtk_drawing_area_is_child,
  gtk_drawing_area_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};


GtkWidget*
gtk_drawing_area_new (gint             width,
		      gint             height,
		      GtkEventFunction event_function,
		      GdkEventMask     event_mask)
{
  GtkDrawingArea *darea;

  g_function_enter ("gtk_drawing_area_new");

  darea = g_new (GtkDrawingArea, 1);

  darea->widget.type = gtk_get_drawing_area_type ();
  darea->widget.function_table = &drawing_area_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) darea);

  darea->width = width;
  darea->height = height;
  darea->event_function = event_function;
  darea->event_mask = event_mask;

  g_function_leave ("gtk_drawing_area_new");
  return ((GtkWidget*) darea);
}

guint16
gtk_get_drawing_area_type ()
{
  static guint16 drawing_area_type = 0;

  g_function_enter ("gtk_get_drawing_area_type");

  if (!drawing_area_type)
    gtk_widget_unique_type (&drawing_area_type);

  g_function_leave ("gtk_get_drawing_area_type");
  return drawing_area_type;
}


static void
gtk_drawing_area_destroy (GtkWidget *widget)
{
  GdkEvent event;

  g_function_enter ("gtk_drawing_area_destroy");

  g_assert (widget != NULL);

  event.type = GDK_DESTROY;
  event.any.window = widget->window;
  gtk_widget_event (widget, &event);

  if (widget->window)
    gdk_window_destroy (widget->window);
  g_free (widget);

  g_function_leave ("gtk_drawing_area_destroy");
}

static void
gtk_drawing_area_realize (GtkWidget *widget)
{
  GtkDrawingArea *darea;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint width, height;
  gint x, y;

  g_function_enter ("gtk_drawing_area_realize");

  g_assert (widget != NULL);

  darea = (GtkDrawingArea*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (darea->width == -1)
    {
      x = widget->allocation.x;
      width = widget->allocation.width;
    }
  else
    {
      x = widget->allocation.x + (widget->allocation.width - widget->requisition.width) / 2;
      width = widget->requisition.width;
    }

  if (darea->height == -1)
    {
      y = widget->allocation.y;
      height = widget->allocation.height;
    }
  else
    {
      y = widget->allocation.y + (widget->allocation.height - widget->requisition.height) / 2;
      height = widget->requisition.height;
    }

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = x;
  attributes.y = y;
  attributes.width = width;
  attributes.height = height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_peek_visual ();
  attributes.colormap = gtk_peek_colormap ();
  attributes.event_mask = darea->event_mask;
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  widget->window = gdk_window_new (widget->parent->widget.window,
				   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, darea);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_background (widget->window, &widget->style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_drawing_area_realize");
}

static void
gtk_drawing_area_draw (GtkWidget    *widget,
		       GdkRectangle *area,
		       gint          is_expose)
{
  GdkEvent event;

  g_function_enter ("gtk_drawing_area_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  event.type = GDK_EXPOSE;
  event.expose.window = widget->window;
  event.expose.area = *area;

  gtk_widget_event (widget, &event);

  g_function_leave ("gtk_drawing_area_draw");
}

static gint
gtk_drawing_area_event (GtkWidget *widget,
			GdkEvent  *event)
{
  GtkDrawingArea *darea;
  gint return_val;

  g_function_enter ("gtk_drawing_area_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  darea = (GtkDrawingArea*) widget;
  if (darea->event_function)
    return_val = (* darea->event_function) (widget, event);

  g_function_leave ("gtk_drawing_area_event");
  return return_val;
}

static void
gtk_drawing_area_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  GtkDrawingArea *darea;

  g_function_enter ("gtk_drawing_area_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  darea = (GtkDrawingArea*) widget;

  if (darea->width != -1)
    requisition->width = darea->width;
  else
    requisition->width = 1;

  if (darea->height != -1)
    requisition->height = darea->height;
  else
    requisition->height = 1;

  g_function_leave ("gtk_drawing_area_size_request");
}

static void
gtk_drawing_area_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  GtkDrawingArea *darea;
  gint width, height;
  gint x, y;

  g_function_enter ("gtk_drawing_area_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  darea = (GtkDrawingArea*) widget;

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      if (darea->width == -1)
	{
	  x = allocation->x;
	  width = allocation->width;
	}
      else
	{
	  x = allocation->x + (allocation->width - widget->requisition.width) / 2;
	  width = widget->requisition.width;
	}

      if (darea->height == -1)
	{
	  y = allocation->y;
	  height = allocation->height;
	}
      else
	{
	  y = allocation->y + (allocation->height - widget->requisition.height) / 2;
	  height = widget->requisition.height;
	}

      gdk_window_move (widget->window, x, y);
      gdk_window_set_size (widget->window, width, height);
    }

  g_function_leave ("gtk_drawing_area_size_allocate");
}

static gint
gtk_drawing_area_is_child (GtkWidget *widget,
			   GtkWidget *child)
{
  return FALSE;
}

static gint
gtk_drawing_area_locate (GtkWidget  *widget,
			 GtkWidget **child,
			 gint        x,
			 gint        y)
{
  return FALSE;
}
