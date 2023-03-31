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
#include "gtkframe.h"
#include "gtkscroll.h"
#include "gtkscrollbar.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkScrolledArea    GtkScrolledArea;
typedef struct _GtkScrolledWindow  GtkScrolledWindow;

struct _GtkScrolledArea
{
  GtkContainer container;

  GtkWidget *child;

  gfloat xalign;
  gfloat yalign;

  GtkDataAdjustment *hadjustment;
  GtkDataAdjustment *vadjustment;
  GtkObserver adjustment_observer;
};

struct _GtkScrolledWindow
{
  GtkContainer container;

  GtkWidget *frame;
  GtkWidget *scrolled_area;
  GtkWidget *hscrollbar;
  GtkWidget *vscrollbar;
};


static void   gtk_scrolled_area_destroy       (GtkWidget       *widget);
static void   gtk_scrolled_area_map           (GtkWidget       *widget);
static void   gtk_scrolled_area_unmap         (GtkWidget       *widget);
static void   gtk_scrolled_area_realize       (GtkWidget       *widget);
static void   gtk_scrolled_area_draw          (GtkWidget       *widget,
					       GdkRectangle    *area,
					       gint             is_expose);
static gint   gtk_scrolled_area_event         (GtkWidget       *widget,
					       GdkEvent        *event);
static void   gtk_scrolled_area_size_request  (GtkWidget       *widget,
					       GtkRequisition  *requisition);
static void   gtk_scrolled_area_size_allocate (GtkWidget       *widget,
					       GtkAllocation   *allocation);
static gint   gtk_scrolled_area_is_child      (GtkWidget       *widget,
					       GtkWidget       *child);
static gint   gtk_scrolled_area_locate        (GtkWidget       *widget,
					       GtkWidget      **child,
					       gint             x,
					       gint             y);
static void   gtk_scrolled_area_add           (GtkContainer    *container,
					       GtkWidget       *widget);
static void   gtk_scrolled_area_remove        (GtkContainer    *container,
					       GtkWidget       *widget);
static void   gtk_scrolled_area_need_resize   (GtkContainer    *container,
					       GtkWidget       *widget);
static void   gtk_scrolled_area_foreach       (GtkContainer    *container,
					       GtkCallback      callback,
					       gpointer         callback_data);

static gint   gtk_scrolled_area_adjustment_update     (GtkObserver *observer,
						       GtkData     *data);
static void   gtk_scrolled_area_adjustment_disconnect (GtkObserver *observer,
						       GtkData     *data);

static void   gtk_scrolled_window_destroy       (GtkWidget       *widget);
static void   gtk_scrolled_window_map           (GtkWidget       *widget);
static void   gtk_scrolled_window_unmap         (GtkWidget       *widget);
static void   gtk_scrolled_window_realize       (GtkWidget       *widget);
static void   gtk_scrolled_window_draw          (GtkWidget       *widget,
						 GdkRectangle    *area,
						 gint             is_expose);
static gint   gtk_scrolled_window_event         (GtkWidget       *widget,
						 GdkEvent        *event);
static void   gtk_scrolled_window_size_request  (GtkWidget       *widget,
						 GtkRequisition  *requisition);
static void   gtk_scrolled_window_size_allocate (GtkWidget       *widget,
						 GtkAllocation   *allocation);
static gint   gtk_scrolled_window_is_child      (GtkWidget       *widget,
						 GtkWidget       *child);
static gint   gtk_scrolled_window_locate        (GtkWidget       *widget,
						 GtkWidget      **child,
						 gint             x,
						 gint             y);
static void   gtk_scrolled_window_add           (GtkContainer    *container,
						 GtkWidget       *widget);
static void   gtk_scrolled_window_remove        (GtkContainer    *container,
						 GtkWidget       *widget);
static void   gtk_scrolled_window_foreach       (GtkContainer    *container,
						 GtkCallback      callback,
						 gpointer         callback_data);


static guint16 scrolled_area_type = 0;
static guint16 scrolled_window_type = 0;

static GtkWidgetFunctions scrolled_area_widget_functions =
{
  gtk_scrolled_area_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_scrolled_area_map,
  gtk_scrolled_area_unmap,
  gtk_scrolled_area_realize,
  gtk_scrolled_area_draw,
  gtk_widget_default_draw_focus,
  gtk_scrolled_area_event,
  gtk_scrolled_area_size_request,
  gtk_scrolled_area_size_allocate,
  gtk_scrolled_area_is_child,
  gtk_scrolled_area_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions scrolled_window_widget_functions =
{
  gtk_scrolled_window_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_scrolled_window_map,
  gtk_scrolled_window_unmap,
  gtk_scrolled_window_realize,
  gtk_scrolled_window_draw,
  gtk_widget_default_draw_focus,
  gtk_scrolled_window_event,
  gtk_scrolled_window_size_request,
  gtk_scrolled_window_size_allocate,
  gtk_scrolled_window_is_child,
  gtk_scrolled_window_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions scrolled_area_container_functions =
{
  gtk_scrolled_area_add,
  gtk_scrolled_area_remove,
  gtk_scrolled_area_need_resize,
  gtk_container_default_focus_advance,
  gtk_scrolled_area_foreach,
};

static GtkContainerFunctions scrolled_window_container_functions =
{
  gtk_scrolled_window_add,
  gtk_scrolled_window_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_scrolled_window_foreach,
};


GtkWidget*
gtk_scrolled_area_new (GtkDataAdjustment *hadjustment,
		       GtkDataAdjustment *vadjustment)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_new");

  scrolled_area = g_new (GtkScrolledArea, 1);

  scrolled_area->container.widget.type = gtk_get_scrolled_area_type ();
  scrolled_area->container.widget.function_table = &scrolled_area_widget_functions;
  scrolled_area->container.function_table = &scrolled_area_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) scrolled_area);
  gtk_container_set_defaults ((GtkWidget*) scrolled_area);

  scrolled_area->child = NULL;
  scrolled_area->xalign = 0.5;
  scrolled_area->yalign = 0.5;

  if (hadjustment)
    scrolled_area->hadjustment = hadjustment;
  else
    scrolled_area->hadjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  if (vadjustment)
    scrolled_area->vadjustment = vadjustment;
  else
    scrolled_area->vadjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  scrolled_area->adjustment_observer.update = gtk_scrolled_area_adjustment_update;
  scrolled_area->adjustment_observer.disconnect = gtk_scrolled_area_adjustment_disconnect;
  scrolled_area->adjustment_observer.user_data = scrolled_area;

  gtk_data_attach ((GtkData*) scrolled_area->hadjustment,
		   &scrolled_area->adjustment_observer);
  gtk_data_attach ((GtkData*) scrolled_area->vadjustment,
		   &scrolled_area->adjustment_observer);

  g_function_leave ("gtk_scrolled_area_new");
  return ((GtkWidget*) scrolled_area);
}

GtkWidget*
gtk_scrolled_window_new (GtkDataAdjustment *hadjustment,
			 GtkDataAdjustment *vadjustment)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_new");

  if (!scrolled_window_type)
    gtk_widget_unique_type (&scrolled_window_type);

  scrolled_win = g_new (GtkScrolledWindow, 1);

  scrolled_win->container.widget.type = scrolled_window_type;
  scrolled_win->container.widget.function_table = &scrolled_window_widget_functions;
  scrolled_win->container.function_table = &scrolled_window_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) scrolled_win);
  gtk_container_set_defaults ((GtkWidget*) scrolled_win);

  scrolled_win->frame = NULL;
  scrolled_win->scrolled_area = NULL;
  scrolled_win->hscrollbar = NULL;
  scrolled_win->vscrollbar = NULL;

  scrolled_win->frame = gtk_frame_new (NULL);
  scrolled_win->frame->parent = (GtkContainer*) scrolled_win;
  gtk_frame_set_type (scrolled_win->frame, GTK_SHADOW_NONE);
  gtk_container_set_border_width (scrolled_win->frame, 0);

  scrolled_win->scrolled_area = gtk_scrolled_area_new (hadjustment, vadjustment);
  gtk_container_add (scrolled_win->frame, scrolled_win->scrolled_area);

  hadjustment = (GtkDataAdjustment*)
    gtk_scrolled_area_get_hadjustment (scrolled_win->scrolled_area);
  vadjustment = (GtkDataAdjustment*)
    gtk_scrolled_area_get_vadjustment (scrolled_win->scrolled_area);

  scrolled_win->hscrollbar = gtk_hscrollbar_new (hadjustment);
  scrolled_win->vscrollbar = gtk_vscrollbar_new (vadjustment);

  scrolled_win->hscrollbar->parent = (GtkContainer*) scrolled_win;
  scrolled_win->vscrollbar->parent = (GtkContainer*) scrolled_win;

  gtk_widget_show (scrolled_win->frame);
  gtk_widget_show (scrolled_win->scrolled_area);
  gtk_widget_show (scrolled_win->hscrollbar);
  gtk_widget_show (scrolled_win->vscrollbar);

  g_function_leave ("gtk_scrolled_window_new");
  return ((GtkWidget*) scrolled_win);
}

GtkData*
gtk_scrolled_area_get_hadjustment (GtkWidget *scrolled_area)
{
  GtkScrolledArea *rscrolled_area;
  GtkData *data;

  g_function_enter ("gtk_scrolled_area_get_hadjustment");

  g_assert (scrolled_area != NULL);

  rscrolled_area = (GtkScrolledArea*) scrolled_area;
  data = (GtkData*) rscrolled_area->hadjustment;

  g_function_leave ("gtk_scrolled_area_get_hadjustment");
  return data;
}

GtkData*
gtk_scrolled_area_get_vadjustment (GtkWidget *scrolled_area)
{
  GtkScrolledArea *rscrolled_area;
  GtkData *data;

  g_function_enter ("gtk_scrolled_area_get_vadjustment");

  g_assert (scrolled_area != NULL);

  rscrolled_area = (GtkScrolledArea*) scrolled_area;
  data = (GtkData*) rscrolled_area->vadjustment;

  g_function_leave ("gtk_scrolled_area_get_vadjustment");
  return data;
}

void
gtk_scrolled_area_set_align (GtkWidget *scrolled_area,
			     gfloat   xalign,
			     gfloat   yalign)
{
  GtkScrolledArea *rscrolled_area;

  g_function_enter ("gtk_scrolled_area_set_align");

  g_assert (scrolled_area != NULL);
  rscrolled_area = (GtkScrolledArea*) scrolled_area;

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);

  if ((rscrolled_area->xalign != xalign) || (rscrolled_area->yalign != yalign))
    {
      rscrolled_area->xalign = xalign;
      rscrolled_area->yalign = yalign;

      if (rscrolled_area->child)
	gtk_container_need_resize ((GtkContainer*) rscrolled_area, rscrolled_area->child);
    }

  g_function_leave ("gtk_scrolled_area_set_align");
}

GtkWidget*
gtk_scrolled_window_get_scrolled_area  (GtkWidget *scrolled_window)
{
  GtkScrolledWindow *scrolled_win;
  GtkWidget *scrolled_area;

  g_function_enter ("gtk_scrolled_window_get_scrolled_area");

  if (!scrolled_window)
    g_error ("passed NULL scrolled_window gto gtk_scrolled_window_get_scrolled_area");

  scrolled_win = (GtkScrolledWindow*) scrolled_window;
  scrolled_area = (GtkWidget*) scrolled_win->scrolled_area;

  g_function_leave ("gtk_scrolled_window_get_scrolled_area");
  return scrolled_area;
}

GtkWidget*
gtk_scrolled_window_get_hscrollbar (GtkWidget *scrolled_window)
{
  GtkScrolledWindow *scrolled_win;
  GtkWidget *hscrollbar;

  g_function_enter ("gtk_scrolled_window_get_hscrollbar");

  if (!scrolled_window)
    g_error ("passed NULL scrolled_window gto gtk_scrolled_window_get_hscrollbar");

  scrolled_win = (GtkScrolledWindow*) scrolled_window;
  hscrollbar = scrolled_win->hscrollbar;

  g_function_leave ("gtk_scrolled_window_get_hscrollbar");
  return hscrollbar;
}

GtkWidget*
gtk_scrolled_window_get_vscrollbar (GtkWidget *scrolled_window)
{
  GtkScrolledWindow *scrolled_win;
  GtkWidget *vscrollbar;

  g_function_enter ("gtk_scrolled_window_get_vscrollbar");

  if (!scrolled_window)
    g_error ("passed NULL scrolled_window gto gtk_scrolled_window_get_vscrollbar");

  scrolled_win = (GtkScrolledWindow*) scrolled_window;
  vscrollbar = scrolled_win->vscrollbar;

  g_function_leave ("gtk_scrolled_window_get_vscrollbar");
  return vscrollbar;
}

void
gtk_scrolled_window_set_shadow_type (GtkWidget     *scrolled_window,
				     GtkShadowType  type)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_set_shadow_type");

  g_assert (scrolled_window != NULL);
  scrolled_win = (GtkScrolledWindow*) scrolled_window;

  gtk_frame_set_type (scrolled_win->frame, type);

  g_function_leave ("gtk_scrolled_window_set_shadow_type");
}

guint16
gtk_get_scrolled_area_type ()
{
  g_function_enter ("gtk_get_scrolled_area_type");

  if (!scrolled_area_type)
    gtk_widget_unique_type (&scrolled_area_type);

  g_function_leave ("gtk_get_scrolled_area_type");
  return scrolled_area_type;
}

guint16
gtk_get_scrolled_window_type ()
{
  g_function_enter ("gtk_get_scrolled_window_type");

  if (!scrolled_window_type)
    gtk_widget_unique_type (&scrolled_window_type);

  g_function_leave ("gtk_get_scrolled_window_type");
  return scrolled_window_type;
}


static void
gtk_scrolled_area_destroy (GtkWidget *widget)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_destroy");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_destroy");

  scrolled_area = (GtkScrolledArea*) widget;
  if (scrolled_area->child)
    if (!gtk_widget_destroy (scrolled_area->child))
      scrolled_area->child->parent = NULL;

  gtk_data_detach ((GtkData*) scrolled_area->hadjustment, &scrolled_area->adjustment_observer);
  gtk_data_detach ((GtkData*) scrolled_area->vadjustment, &scrolled_area->adjustment_observer);
  gtk_data_destroy ((GtkData*) scrolled_area->hadjustment);
  gtk_data_destroy ((GtkData*) scrolled_area->vadjustment);

  if (scrolled_area->container.widget.window)
    gdk_window_destroy (scrolled_area->container.widget.window);
  g_free (scrolled_area);

  g_function_leave ("gtk_scrolled_area_destroy");
}

static void
gtk_scrolled_area_map (GtkWidget *widget)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_map");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_map");

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
      gdk_window_show (widget->window);

      scrolled_area = (GtkScrolledArea*) widget;

      if (scrolled_area->child &&
	  GTK_WIDGET_VISIBLE (scrolled_area->child) &&
	  !GTK_WIDGET_MAPPED (scrolled_area->child))
	gtk_widget_map (scrolled_area->child);
    }

  g_function_leave ("gtk_scrolled_area_map");
}

static void
gtk_scrolled_area_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_scrolled_area_unmap");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_unmap");

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
      gdk_window_hide (widget->window);
    }

  g_function_leave ("gtk_scrolled_area_unmap");
}

static void
gtk_scrolled_area_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;

  g_function_enter ("gtk_scrolled_area_realize");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_realize");

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  widget->window = gdk_window_new (widget->parent->widget.window,
				   &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_background (widget->window, &widget->style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_scrolled_area_realize");
}

static void
gtk_scrolled_area_draw (GtkWidget    *widget,
			GdkRectangle *area,
			gint        is_expose)
{
  GtkScrolledArea *scrolled_area;
  GdkRectangle child_area;

  g_function_enter ("gtk_scrolled_area_draw");

  scrolled_area = (GtkScrolledArea*) widget;
  g_assert (scrolled_area != NULL);

  if (GTK_WIDGET_VISIBLE (widget))
    if (scrolled_area->child && GTK_WIDGET_NO_WINDOW (scrolled_area->child))
      if (gtk_widget_intersect (scrolled_area->child, area, &child_area))
	gtk_widget_draw (scrolled_area->child, &child_area, is_expose);

  g_function_leave ("gtk_scrolled_area_draw");
}

static gint
gtk_scrolled_area_event (GtkWidget *widget,
			 GdkEvent  *event)
{
  g_function_enter ("gtk_scrolled_area_event");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_event");

  if (!event)
    g_error ("passed NULL event to gtk_scrolled_area_event");

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_scrolled_area_event");
  return FALSE;
}

static void
gtk_scrolled_area_size_request (GtkWidget      *widget,
				GtkRequisition *requisition)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_size_request");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_size_request");

  if (!requisition)
    g_error ("passed NULL requisition to gtk_scrolled_area_size_request");

  scrolled_area = (GtkScrolledArea*) widget;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      if (scrolled_area->child)
	{
	  scrolled_area->child->requisition.width = 0;
	  scrolled_area->child->requisition.height = 0;

	  gtk_widget_size_request (scrolled_area->child, &scrolled_area->child->requisition);
	}

      requisition->width = 1;
      requisition->height = 1;
    }

  g_function_leave ("gtk_scrolled_area_size_request");
}

static void
gtk_scrolled_area_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation)
{
  GtkScrolledArea *scrolled_area;
  GtkAllocation child_allocation;
  gfloat h_max_val;
  gfloat v_max_val;
  gfloat h_page_size;
  gfloat v_page_size;

  g_function_enter ("gtk_scrolled_area_size_allocate");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_size_allocate");

  if (!allocation)
    g_error ("passed NULL allocation to gtk_scrolled_area_size_allocate");

  scrolled_area = (GtkScrolledArea*) widget;

  if (GTK_WIDGET_REALIZED (widget))
    {
      if ((widget->allocation.x != allocation->x) ||
	  (widget->allocation.y != allocation->y))
	gdk_window_move (widget->window,
			 allocation->x,
			 allocation->y);

      if ((widget->allocation.width != allocation->width) ||
	  (widget->allocation.height != allocation->height))
	gdk_window_set_size (widget->window,
			     allocation->width,
			     allocation->height);
    }

  widget->allocation = *allocation;

  if (scrolled_area->child)
    {
      child_allocation.x = -scrolled_area->hadjustment->value;
      child_allocation.y = -scrolled_area->vadjustment->value;
      child_allocation.width = scrolled_area->child->requisition.width;
      child_allocation.height = scrolled_area->child->requisition.height;

      if (child_allocation.width < allocation->width)
	child_allocation.x += ((allocation->width - child_allocation.width) *
			       scrolled_area->xalign);
      if (child_allocation.height < allocation->height)
	child_allocation.y += ((allocation->height - child_allocation.height) *
			       scrolled_area->yalign);

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (scrolled_area->child, &child_allocation);
    }

  if (scrolled_area->child &&
      (scrolled_area->child->requisition.width != 0) &&
      (scrolled_area->child->requisition.height != 0))
    {
      h_page_size = MIN (scrolled_area->container.widget.allocation.width,
			 scrolled_area->child->requisition.width);
      v_page_size = MIN (scrolled_area->container.widget.allocation.height,
			 scrolled_area->child->requisition.height);

      h_max_val = scrolled_area->child->requisition.width;
      v_max_val = scrolled_area->child->requisition.height;

      scrolled_area->hadjustment->lower = 0;
      scrolled_area->hadjustment->upper = h_max_val;
      scrolled_area->hadjustment->step_increment = 5;
      scrolled_area->hadjustment->page_increment = h_page_size / 2;
      scrolled_area->hadjustment->page_size = MIN (h_page_size, h_max_val);

      scrolled_area->vadjustment->lower = 0;
      scrolled_area->vadjustment->upper = v_max_val;
      scrolled_area->vadjustment->step_increment = 5;
      scrolled_area->vadjustment->page_increment = v_page_size / 2;
      scrolled_area->vadjustment->page_size = MIN (v_page_size, v_max_val);
    }
  else
    {
      scrolled_area->hadjustment->lower = 0;
      scrolled_area->hadjustment->upper = 0;
      scrolled_area->hadjustment->step_increment = 0;
      scrolled_area->hadjustment->page_increment = 0;
      scrolled_area->hadjustment->page_size = 0;

      scrolled_area->vadjustment->lower = 0;
      scrolled_area->vadjustment->upper = 0;
      scrolled_area->vadjustment->step_increment = 0;
      scrolled_area->vadjustment->page_increment = 0;
      scrolled_area->vadjustment->page_size = 0;
    }

  gtk_data_notify ((GtkData*) scrolled_area->hadjustment);
  gtk_data_notify ((GtkData*) scrolled_area->vadjustment);

  g_function_leave ("gtk_scrolled_area_size_allocate");
}

static gint
gtk_scrolled_area_is_child (GtkWidget *widget,
			    GtkWidget *child)
{
  GtkScrolledArea *scrolled_area;
  gint return_val;

  g_function_enter ("gtk_scrolled_area_is_child");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_is_child");

  if (!child)
    g_error ("passed NULL child to gtk_scrolled_area_is_child");

  scrolled_area = (GtkScrolledArea*) widget;

  return_val = FALSE;
  if (scrolled_area->child == child)
    return_val = TRUE;
  else if (scrolled_area->child)
    return_val = gtk_widget_is_child (scrolled_area->child, child);

  g_function_leave ("gtk_scrolled_area_is_child");
  return return_val;
}

static gint
gtk_scrolled_area_locate (GtkWidget  *widget,
			  GtkWidget **child,
			  gint        x,
			  gint        y)
{
  g_function_enter ("gtk_scrolled_area_locate");
  g_warning ("gtk_scrolled_area_locate: UNFINISHED");
  g_function_leave ("gtk_scrolled_area_locate");
  return FALSE;
}

static void
gtk_scrolled_area_add (GtkContainer *container,
		       GtkWidget    *widget)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_add");

  if (!container)
    g_error ("passed NULL container to gtk_scrolled_area_add");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_add");

  scrolled_area = (GtkScrolledArea*) container;

  if (scrolled_area->child)
    g_error ("scrolled area already has a child");
  else
    scrolled_area->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_scrolled_area_add");
}

static void
gtk_scrolled_area_remove (GtkContainer *container,
			  GtkWidget    *widget)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_remove");

  if (!container)
    g_error ("passed NULL container to gtk_scrolled_area_remove");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_remove");

  scrolled_area = (GtkScrolledArea*) container;

  if (scrolled_area->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!scrolled_area->child)
    g_error ("scrolled area has no child to remove");

  scrolled_area->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_scrolled_area_remove");
}

static void
gtk_scrolled_area_need_resize (GtkContainer *container,
			       GtkWidget    *widget)
{
  GtkAllocation allocation;

  g_function_enter ("gtk_scrolled_area_need_resize");

  if (!container)
    g_error ("passed NULL container to gtk_scrolled_area_need_resize");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_area_need_resize");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    {
      gtk_widget_size_request ((GtkWidget*) container, &container->widget.requisition);

      allocation.x = 0;
      allocation.y = 0;
      allocation.width = widget->requisition.width;
      allocation.height = widget->requisition.height;

      gtk_widget_size_allocate (widget, &allocation);

      if (GTK_WIDGET_REALIZED (container) &&
	  !GTK_WIDGET_REALIZED (widget))
	gtk_widget_realize (widget);

      if (GTK_WIDGET_MAPPED (container) &&
	  !GTK_WIDGET_MAPPED (widget))
	gtk_widget_map (widget);
    }

  g_function_leave ("gtk_scrolled_area_need_resize");
}

static void
gtk_scrolled_area_foreach (GtkContainer *container,
			   GtkCallback   callback,
			   gpointer      callback_data)
{
  GtkScrolledArea *scrolled_area;

  g_function_enter ("gtk_scrolled_area_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  scrolled_area = (GtkScrolledArea*) container;

  if (scrolled_area->child)
    (* callback) (scrolled_area->child, callback_data, NULL);

  g_function_leave ("gtk_scrolled_area_foreach");
}

static gint
gtk_scrolled_area_adjustment_update (GtkObserver *observer,
				     GtkData     *data)
{
  GtkScrolledArea *scrolled_area;
  GtkAllocation allocation;

  g_function_enter ("gtk_scrolled_area_adjustment_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  scrolled_area = observer->user_data;
  g_assert (scrolled_area != NULL);

  if (scrolled_area->child)
    {
      allocation.x = -scrolled_area->hadjustment->value;
      allocation.y = -scrolled_area->vadjustment->value;
      allocation.width = scrolled_area->child->requisition.width;
      allocation.height = scrolled_area->child->requisition.height;

      if (allocation.width < scrolled_area->container.widget.allocation.width)
        allocation.x += (scrolled_area->container.widget.allocation.width -
                         allocation.width) * scrolled_area->xalign;
      if (allocation.height < scrolled_area->container.widget.allocation.height)
        allocation.y += (scrolled_area->container.widget.allocation.height -
                         allocation.height) * scrolled_area->yalign;

      if (allocation.width <= 0)
        allocation.width = 1;
      if (allocation.height <= 0)
        allocation.height = 1;

      gtk_widget_size_allocate (scrolled_area->child, &allocation);
    }

  g_function_leave ("gtk_scrolled_area_adjustment_update");
  return FALSE;
}

static void
gtk_scrolled_area_adjustment_disconnect (GtkObserver *observer,
					 GtkData     *data)
{
  g_function_enter ("gtk_scrolled_area_adjustment_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_scrolled_area_adjustment_disconnect");
}

static void
gtk_scrolled_window_destroy (GtkWidget *widget)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_destroy");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_destroy");

  scrolled_win = (GtkScrolledWindow*) widget;
  if (!gtk_widget_destroy (scrolled_win->frame))
    scrolled_win->frame->parent = NULL;
  if (!gtk_widget_destroy (scrolled_win->hscrollbar))
    scrolled_win->hscrollbar->parent = NULL;
  if (!gtk_widget_destroy (scrolled_win->vscrollbar))
    scrolled_win->vscrollbar->parent = NULL;
  g_free (scrolled_win);

  g_function_leave ("gtk_scrolled_window_destroy");
}

static void
gtk_scrolled_window_map (GtkWidget *widget)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_map");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_map");

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
      gdk_window_show (widget->window);

      scrolled_win = (GtkScrolledWindow*) widget;

      if (scrolled_win->frame &&
	  GTK_WIDGET_VISIBLE (scrolled_win->frame) &&
	  !GTK_WIDGET_MAPPED (scrolled_win->frame))
	gtk_widget_map ((GtkWidget*) scrolled_win->frame);

      if (scrolled_win->scrolled_area &&
	  GTK_WIDGET_VISIBLE (scrolled_win->scrolled_area) &&
	  !GTK_WIDGET_MAPPED (scrolled_win->scrolled_area))
	gtk_widget_map ((GtkWidget*) scrolled_win->scrolled_area);

      if (scrolled_win->hscrollbar &&
	  GTK_WIDGET_VISIBLE (scrolled_win->hscrollbar) &&
	  !GTK_WIDGET_MAPPED (scrolled_win->hscrollbar))
	gtk_widget_map (scrolled_win->hscrollbar);

      if (scrolled_win->vscrollbar &&
	  GTK_WIDGET_VISIBLE (scrolled_win->vscrollbar) &&
	  !GTK_WIDGET_MAPPED (scrolled_win->vscrollbar))
	gtk_widget_map (scrolled_win->vscrollbar);
    }

  g_function_leave ("gtk_scrolled_window_map");
}

static void
gtk_scrolled_window_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_scrolled_window_unmap");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_unmap");

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
      gdk_window_hide (widget->window);
    }

  g_function_leave ("gtk_scrolled_window_unmap");
}

static void
gtk_scrolled_window_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;

  g_function_enter ("gtk_scrolled_window_realize");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_realize");

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  widget->window = gdk_window_new (widget->parent->widget.window,
				   &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_background (widget->window, &widget->style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_scrolled_window_realize");
}

static void
gtk_scrolled_window_draw (GtkWidget    *widget,
			  GdkRectangle *area,
			  gint        is_expose)
{
  g_function_enter ("gtk_scrolled_window_draw");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_draw");

  g_function_leave ("gtk_scrolled_window_draw");
}

static gint
gtk_scrolled_window_event (GtkWidget *widget,
			   GdkEvent  *event)
{
  g_function_enter ("gtk_scrolled_window_event");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_event");

  if (!event)
    g_error ("passed NULL event to gtk_scrolled_window_event");

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_scrolled_window_event");
  return FALSE;
}

static void
gtk_scrolled_window_size_request (GtkWidget      *widget,
				  GtkRequisition *requisition)
{
  GtkScrolledWindow *scrolled_win;
  gint extra_width;
  gint extra_height;

  g_function_enter ("gtk_scrolled_window_size_request");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_size_request");

  if (!requisition)
    g_error ("passed NULL requisition to gtk_scrolled_window_size_request");

  scrolled_win = (GtkScrolledWindow*) widget;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      scrolled_win->frame->requisition.width = 0;
      scrolled_win->frame->requisition.height = 0;

      gtk_widget_size_request (scrolled_win->frame, &scrolled_win->frame->requisition);

      requisition->width = scrolled_win->frame->requisition.width;
      requisition->height = scrolled_win->frame->requisition.height;

      scrolled_win->hscrollbar->requisition.width = 0;
      scrolled_win->hscrollbar->requisition.height = 0;

      scrolled_win->vscrollbar->requisition.width = 0;
      scrolled_win->vscrollbar->requisition.height = 0;

      extra_width = 0;
      extra_height = 0;

      if (GTK_WIDGET_VISIBLE (scrolled_win->hscrollbar))
	{
	  gtk_widget_size_request (scrolled_win->hscrollbar,
				   &scrolled_win->hscrollbar->requisition);

	  extra_height = 5;
	}

      if (GTK_WIDGET_VISIBLE (scrolled_win->vscrollbar))
	{
	  gtk_widget_size_request (scrolled_win->vscrollbar,
				   &scrolled_win->vscrollbar->requisition);

	  extra_width = 5;
	}

      requisition->width = MAX (requisition->width, scrolled_win->hscrollbar->requisition.width);
      requisition->height = MAX (requisition->height, scrolled_win->vscrollbar->requisition.height);

      requisition->width += scrolled_win->vscrollbar->requisition.width + extra_width;
      requisition->height += scrolled_win->hscrollbar->requisition.height + extra_height;
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  g_function_leave ("gtk_scrolled_window_size_request");
}

static void
gtk_scrolled_window_size_allocate (GtkWidget     *widget,
				   GtkAllocation *allocation)
{
  GtkScrolledWindow *scrolled_win;
  GtkAllocation child_allocation;
  gint width_extra;
  gint height_extra;

  g_function_enter ("gtk_scrolled_window_size_allocate");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_size_allocate");

  if (!allocation)
    g_error ("passed NULL allocation to gtk_scrolled_window_size_allocate");

  scrolled_win = (GtkScrolledWindow*) widget;

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

  width_extra = GTK_WIDGET_VISIBLE (scrolled_win->vscrollbar) ? 5 : 0;
  height_extra = GTK_WIDGET_VISIBLE (scrolled_win->hscrollbar) ? 5 : 0;

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = (allocation->width -
			    scrolled_win->vscrollbar->requisition.width -
			    width_extra);
  child_allocation.height = (allocation->height -
			     scrolled_win->hscrollbar->requisition.height -
			     height_extra);

  if (child_allocation.width <= 0)
    child_allocation.width = 1;
  if (child_allocation.height <= 0)
    child_allocation.height = 1;

  gtk_widget_size_allocate ((GtkWidget*) scrolled_win->frame, &child_allocation);

  if (GTK_WIDGET_VISIBLE (scrolled_win->hscrollbar))
    {
      child_allocation.x = 0;
      child_allocation.y = scrolled_win->frame->allocation.height + height_extra;
      child_allocation.width = scrolled_win->frame->allocation.width;
      child_allocation.height = scrolled_win->hscrollbar->requisition.height;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate ((GtkWidget*) scrolled_win->hscrollbar, &child_allocation);
    }

  if (GTK_WIDGET_VISIBLE (scrolled_win->vscrollbar))
    {
      child_allocation.x = scrolled_win->frame->allocation.width + width_extra;
      child_allocation.y = 0;
      child_allocation.width = scrolled_win->vscrollbar->requisition.width;
      child_allocation.height = scrolled_win->frame->allocation.height;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate ((GtkWidget*) scrolled_win->vscrollbar, &child_allocation);
    }

  g_function_leave ("gtk_scrolled_window_size_allocate");
}

static gint
gtk_scrolled_window_is_child (GtkWidget *widget,
			      GtkWidget *child)
{
  GtkScrolledWindow *scrolled_win;
  gint return_val;

  g_function_enter ("gtk_scrolled_window_is_child");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_is_child");

  if (!child)
    g_error ("passed NULL child to gtk_scrolled_window_is_child");

  scrolled_win = (GtkScrolledWindow*) widget;

  return_val = FALSE;
  if ((scrolled_win->frame == child) ||
      (scrolled_win->scrolled_area == child) ||
      (scrolled_win->hscrollbar == child) ||
      (scrolled_win->vscrollbar == child))
    return_val = TRUE;
  else if (scrolled_win->scrolled_area)
    return_val = gtk_widget_is_child (scrolled_win->scrolled_area, child);

  g_function_leave ("gtk_scrolled_window_is_child");
  return return_val;
}

static gint
gtk_scrolled_window_locate (GtkWidget  *widget,
			    GtkWidget **child,
			    gint        x,
			    gint        y)
{
  g_function_enter ("gtk_scrolled_window_locate");
  g_warning ("gtk_scrolled_window_locate: UNFINISHED");
  g_function_leave ("gtk_scrolled_window_locate");
  return FALSE;
}

static void
gtk_scrolled_window_add (GtkContainer *container,
			 GtkWidget    *widget)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_add");

  if (!container)
    g_error ("passed NULL container to gtk_scrolled_window_add");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_add");

  scrolled_win = (GtkScrolledWindow*) container;

  if (scrolled_win->frame &&
      scrolled_win->scrolled_area &&
      scrolled_win->hscrollbar &&
      scrolled_win->vscrollbar)
    g_error ("trying to add child to scrolled window...add to scrolled area instead");

  g_function_leave ("gtk_scrolled_window_add");
}

static void
gtk_scrolled_window_remove (GtkContainer *container,
			    GtkWidget    *widget)
{
  g_function_enter ("gtk_scrolled_window_remove");

  if (!container)
    g_error ("passed NULL container to gtk_scrolled_window_remove");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrolled_window_remove");

  g_error ("tring to remove child from scrolled window...remove from scrolled area instead");

  g_function_leave ("gtk_scrolled_window_remove");
}

static void
gtk_scrolled_window_foreach (GtkContainer *container,
			     GtkCallback   callback,
			     gpointer      callback_data)
{
  GtkScrolledWindow *scrolled_win;

  g_function_enter ("gtk_scrolled_window_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  scrolled_win = (GtkScrolledWindow*) container;

  (* callback) (scrolled_win->frame, callback_data, NULL);
  (* callback) (scrolled_win->hscrollbar, callback_data, NULL);
  (* callback) (scrolled_win->vscrollbar, callback_data, NULL);

  g_function_leave ("gtk_scrolled_window_foreach");
}
