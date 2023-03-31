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
#include "gtkalignment.h"
#include "gtkcontainer.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkAlignment  GtkAlignment;

struct _GtkAlignment
{
  GtkContainer container;

  GtkWidget *child;
  gdouble xalign;
  gdouble yalign;
  gdouble xscale;
  gdouble yscale;
};


static void  gtk_alignment_destroy       (GtkWidget      *widget);
static void  gtk_alignment_map           (GtkWidget      *widget);
static void  gtk_alignment_unmap         (GtkWidget      *widget);
static void  gtk_alignment_realize       (GtkWidget      *widget);
static void  gtk_alignment_draw          (GtkWidget      *widget,
					  GdkRectangle   *area,
					  gint            is_expose);
static void  gtk_alignment_size_request  (GtkWidget      *widget,
					  GtkRequisition *requisition);
static void  gtk_alignment_size_allocate (GtkWidget      *widget,
					  GtkAllocation  *allocation);
static gint  gtk_alignment_is_child      (GtkWidget      *widget,
					  GtkWidget      *child);
static gint  gtk_alignment_locate        (GtkWidget      *widget,
					  GtkWidget     **child,
					  gint            x,
					  gint            y);
static void  gtk_alignment_set_state     (GtkWidget      *widget,
					  GtkStateType    state);
static void  gtk_alignment_add           (GtkContainer   *container,
					  GtkWidget      *widget);
static void  gtk_alignment_remove        (GtkContainer   *container,
					  GtkWidget      *widget);
static void  gtk_alignment_foreach       (GtkContainer   *container,
					  GtkCallback     callback,
					  gpointer        callback_data);


static GtkWidgetFunctions alignment_widget_functions =
{
  gtk_alignment_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_alignment_map,
  gtk_alignment_unmap,
  gtk_alignment_realize,
  gtk_alignment_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_alignment_size_request,
  gtk_alignment_size_allocate,
  gtk_alignment_is_child,
  gtk_alignment_locate,
  gtk_widget_default_activate,
  gtk_alignment_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions alignment_container_functions =
{
  gtk_alignment_add,
  gtk_alignment_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_alignment_foreach,
};


GtkWidget*
gtk_alignment_new (gdouble xalign,
		   gdouble yalign,
		   gdouble xscale,
		   gdouble yscale)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_new");

  alignment = g_new (GtkAlignment, 1);

  alignment->container.widget.type = gtk_get_alignment_type ();
  alignment->container.widget.function_table = &alignment_widget_functions;
  alignment->container.function_table = &alignment_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) alignment);
  gtk_container_set_defaults ((GtkWidget*) alignment);
  GTK_WIDGET_SET_FLAGS (alignment, GTK_NO_WINDOW);

  alignment->child = NULL;
  alignment->xalign = CLAMP (xalign, 0.0, 1.0);
  alignment->yalign = CLAMP (yalign, 0.0, 1.0);
  alignment->xscale = CLAMP (xscale, 0.0, 1.0);
  alignment->yscale = CLAMP (yscale, 0.0, 1.0);

  g_function_leave ("gtk_alignment_new");
  return ((GtkWidget*) alignment);
}

void
gtk_alignment_set (GtkWidget *widget,
		   gdouble    xalign,
		   gdouble    yalign,
		   gdouble    xscale,
		   gdouble    yscale)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_set");

  g_assert (widget != NULL);

  alignment = (GtkAlignment*) widget;

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);
  xscale = CLAMP (xscale, 0.0, 1.0);
  yscale = CLAMP (yscale, 0.0, 1.0);

  if ((xalign != alignment->xalign) || (yalign != alignment->yalign) ||
      (xscale != alignment->xscale) || (yscale != alignment->yscale))
    {
      alignment->xalign = xalign;
      alignment->yalign = yalign;
      alignment->xscale = xscale;
      alignment->yscale = yscale;

      gtk_widget_size_allocate ((GtkWidget*) alignment, &alignment->container.widget.allocation);
      gtk_widget_draw ((GtkWidget*) alignment, NULL, FALSE);
    }

  g_function_leave ("gtk_alignment_set");
}

guint16
gtk_get_alignment_type ()
{
  static guint16 alignment_type = 0;

  g_function_enter ("gtk_get_alignment_type");

  if (!alignment_type)
    gtk_widget_unique_type (&alignment_type);

  g_function_leave ("gtk_alignment_type");
  return alignment_type;
}


static void
gtk_alignment_destroy (GtkWidget *widget)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_destroy");

  g_assert (widget != NULL);

  alignment = (GtkAlignment*) widget;
  if (alignment->child)
    if (!gtk_widget_destroy (alignment->child))
      alignment->child->parent = NULL;

  g_free (alignment);

  g_function_leave ("gtk_alignment_destroy");
}

static void
gtk_alignment_map (GtkWidget *widget)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  alignment = (GtkAlignment*) widget;

  if (alignment->child &&
      GTK_WIDGET_VISIBLE (alignment->child) &&
      !GTK_WIDGET_MAPPED (alignment->child))
    gtk_widget_map (alignment->child);

  g_function_leave ("gtk_alignment_map");
}

static void
gtk_alignment_unmap (GtkWidget *widget)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  alignment = (GtkAlignment*) widget;

  if (alignment->child &&
      GTK_WIDGET_VISIBLE (alignment->child) &&
      GTK_WIDGET_MAPPED (alignment->child))
    gtk_widget_unmap (alignment->child);

  g_function_leave ("gtk_alignment_unmap");
}

static void
gtk_alignment_realize (GtkWidget *widget)
{
  g_function_enter ("gtk_alignment_realize");

  g_assert (widget != NULL);

  if (!widget)
    g_error ("passed NULL widget to gtk_alignment_realize");

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  widget->window = widget->parent->widget.window;

  g_function_leave ("gtk_alignment_realize");
}

static void
gtk_alignment_draw (GtkWidget    *widget,
		    GdkRectangle *area,
		    gint          is_expose)
{
  GtkAlignment *alignment;
  GdkRectangle child_area;

  g_function_enter ("gtk_alignment_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      alignment = (GtkAlignment*) widget;

      if (is_expose)
	{
	  if (alignment->child && GTK_WIDGET_NO_WINDOW (alignment->child))
	    if (gtk_widget_intersect (alignment->child, area, &child_area))
	      gtk_widget_draw (alignment->child, &child_area, is_expose);
	}
      else
	{
	  if (alignment->child)
	    if (gtk_widget_intersect (alignment->child, area, &child_area))
	      gtk_widget_draw (alignment->child, &child_area, is_expose);
	}
    }

  g_function_leave ("gtk_alignment_draw");
}

static void
gtk_alignment_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  alignment = (GtkAlignment*) widget;

  if (alignment->child)
    {
      alignment->child->requisition.width = 0;
      alignment->child->requisition.height = 0;

      gtk_widget_size_request (alignment->child, &alignment->child->requisition);

      requisition->width = (alignment->child->requisition.width +
			    alignment->container.border_width * 2);
      requisition->height = (alignment->child->requisition.height +
			     alignment->container.border_width * 2);
    }
  else
    {
      requisition->width = alignment->container.border_width * 2;
      requisition->height = alignment->container.border_width * 2;
    }

  g_function_leave ("gtk_alignment_size_request");
}

static void
gtk_alignment_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkAlignment *alignment;
  GtkAllocation child_allocation;
  gint width, height;
  gint x, y;

  g_function_enter ("gtk_alignment_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  alignment = (GtkAlignment*) widget;
  widget->allocation = *allocation;

  if (alignment->child)
    {
      x = alignment->container.border_width;
      y = alignment->container.border_width;
      width = allocation->width - 2 * x;
      height = allocation->height - 2 * y;

      if (width > alignment->child->requisition.width)
	child_allocation.width = (alignment->child->requisition.width *
				  (1.0 - alignment->xscale) +
				  width * alignment->xscale);
      else
	child_allocation.width = width;

      if (height > alignment->child->requisition.height)
	child_allocation.height = (alignment->child->requisition.height *
				   (1.0 - alignment->yscale) +
				   height * alignment->yscale);
      else
	child_allocation.height = height;

      child_allocation.x = alignment->xalign * (width - child_allocation.width) + allocation->x + x;
      child_allocation.y = alignment->yalign * (height - child_allocation.height) + allocation->y + y;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (alignment->child, &child_allocation);
    }

  g_function_leave ("gtk_alignment_size_allocate");
}

static gint
gtk_alignment_is_child (GtkWidget *widget,
			GtkWidget *child)
{
  GtkAlignment *alignment;
  gint return_val;

  g_function_enter ("gtk_alignment_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  alignment = (GtkAlignment*) widget;

  return_val = FALSE;
  if (alignment->child == child)
    return_val = TRUE;
  else if (alignment->child)
    return_val = gtk_widget_is_child (alignment->child, child);

  g_function_leave ("gtk_alignment_is_child");
  return return_val;
}

static gint
gtk_alignment_locate (GtkWidget  *widget,
		      GtkWidget **child,
		      gint        x,
		      gint        y)
{
  GtkAlignment *alignment;
  gint return_val;
  gint child_x;
  gint child_y;

  g_function_enter ("gtk_alignment_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < widget->allocation.width) &&
      (y < widget->allocation.height))
    {
      return_val = TRUE;

      alignment = (GtkAlignment*) widget;
      if (alignment->child)
	{
	  child_x = x - alignment->child->allocation.x;
	  child_y = y - alignment->child->allocation.y;

	  gtk_widget_locate (alignment->child, child, child_x, child_y);
	}

      if (!(*child))
	*child = widget;
    }

  g_function_leave ("gtk_alignment_locate");
  return return_val;
}

static void
gtk_alignment_set_state (GtkWidget    *widget,
			 GtkStateType  state)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_set_state");

  g_assert (widget != NULL);
  alignment = (GtkAlignment*) widget;

  if (alignment->child)
    gtk_widget_set_state (alignment->child, state);

  g_function_leave ("gtk_alignment_set_state");
}

static void
gtk_alignment_add (GtkContainer *container,
		   GtkWidget    *widget)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_add");

  g_assert (container != NULL);
  alignment = (GtkAlignment*) container;

  if (alignment->child)
    g_error ("alignment already has a child");
  else
    alignment->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_alignment_add");
}

static void
gtk_alignment_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  alignment = (GtkAlignment*) container;

  g_assert (alignment->child == widget);
  g_assert (alignment->child != NULL);

  alignment->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_alignment_remove");
}

static void
gtk_alignment_foreach (GtkContainer *container,
		       GtkCallback   callback,
		       gpointer      callback_data)
{
  GtkAlignment *alignment;

  g_function_enter ("gtk_alignment_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  alignment = (GtkAlignment*) container;

  if (alignment->child)
    (* callback) (alignment->child, callback_data, NULL);

  g_function_leave ("gtk_alignment_foreach");
}
