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
#include <math.h>
#include <stdio.h>

#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkmain.h"
#include "gtkruler.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define HORIZONTAL            0
#define VERTICAL              1

#define RULER_WIDTH           20
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     4

#define ROUND(x) ((int) ((x) + 0.5))


typedef struct _GtkRulerMetric  GtkRulerMetric;

struct _GtkRulerMetric
{
  gchar *metric_name;
  gchar *abrev;
  gfloat pixels_per_unit;
  gint   mark_every_nth;
  gint   subdivide[4];     /*  four possible modes of subdivision  */
};

typedef struct _GtkRuler       GtkRuler;

struct _GtkRuler
{
  GtkWidget widget;

  gint16 orientation;
  GdkPixmap *backing_store;
  GdkGC *non_gr_exp_gc;
  gint xsrc, ysrc;

  GtkRulerMetric *metric;
  GtkDataAdjustment *adjustment;
  GtkObserver adjustment_observer;

  gint slider_size;
};


static void   gtk_ruler_destroy               (GtkWidget       *widget);
static void   gtk_ruler_realize               (GtkWidget       *widget);
static void   gtk_ruler_draw                  (GtkWidget       *widget,
					       GdkRectangle    *area,
					       gint             is_expose);
static void   gtk_ruler_expose                (GtkWidget       *widget);
static void   gtk_ruler_draw_ticks            (GtkWidget       *widget);
static void   gtk_ruler_draw_pos              (GtkWidget       *widget);
static gint   gtk_ruler_event                 (GtkWidget       *widget,
					       GdkEvent        *event);
static gint   gtk_ruler_is_child              (GtkWidget       *widget,
					       GtkWidget       *child);
static gint   gtk_ruler_locate                (GtkWidget       *widget,
					       GtkWidget      **child,
					       gint             x,
					       gint             y);

static gint   gtk_ruler_adjustment_update     (GtkObserver *observer,
					       GtkData     *data);
static void   gtk_ruler_adjustment_disconnect (GtkObserver *observer,
					       GtkData     *data);

static void   gtk_ruler_size_allocate     (GtkWidget      *widget,
					   GtkAllocation  *allocation);

static void   gtk_hruler_size_request     (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void   gtk_hruler_motion           (GtkWidget      *widget,
					   gint            x);

static void   gtk_vruler_size_request     (GtkWidget      *widget,
					   GtkRequisition *requistion);
static void   gtk_vruler_motion           (GtkWidget      *widget,
					   gint            y);


static GtkWidgetFunctions hruler_widget_functions =
{
  gtk_ruler_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_ruler_realize,
  gtk_ruler_draw,
  gtk_widget_default_draw_focus,
  gtk_ruler_event,
  gtk_hruler_size_request,
  gtk_ruler_size_allocate,
  gtk_ruler_is_child,
  gtk_ruler_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions vruler_widget_functions =
{
  gtk_ruler_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_ruler_realize,
  gtk_ruler_draw,
  gtk_widget_default_draw_focus,
  gtk_ruler_event,
  gtk_vruler_size_request,
  gtk_ruler_size_allocate,
  gtk_ruler_is_child,
  gtk_ruler_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkRulerMetric ruler_widget_metrics[] =
{
  {"Pixels", "Pi", 1.0, 50, {2, 5, 5, 5}},
  {"Inches", "In", 72.0, 1, {2, 2, 2, 2}},
  {"Centimeters", "Cn", 28.35, 1, {2, 5, 5, 5}},
};


GtkWidget*
gtk_hruler_new (GtkDataAdjustment *adjustment)
{
  GtkRuler *ruler;

  g_function_enter ("gtk_hruler_new");

  ruler = g_new (GtkRuler, 1);

  ruler->widget.type = gtk_get_ruler_type ();
  ruler->widget.function_table = &hruler_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) ruler);
  /*  ruler->widget.style = gtk_style_new (-1); */

  ruler->orientation = HORIZONTAL;

  ruler->backing_store = NULL;
  ruler->non_gr_exp_gc = NULL;
  ruler->metric = &ruler_widget_metrics[PIXELS];

  ruler->adjustment_observer.update = gtk_ruler_adjustment_update;
  ruler->adjustment_observer.disconnect = gtk_ruler_adjustment_disconnect;
  ruler->adjustment_observer.user_data = ruler;

  if (adjustment)
    ruler->adjustment = adjustment;
  else
    ruler->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  gtk_data_attach ((GtkData*) ruler->adjustment, &ruler->adjustment_observer);

  ruler->slider_size = 0;

  g_function_leave ("gtk_hruler_new");
  return ((GtkWidget*) ruler);
}

GtkWidget*
gtk_vruler_new (GtkDataAdjustment *adjustment)
{
  GtkRuler *ruler;

  g_function_enter ("gtk_vruler_new");

  ruler = g_new (GtkRuler, 1);

  ruler->widget.type = gtk_get_ruler_type ();
  ruler->widget.function_table = &vruler_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) ruler);

  ruler->orientation = VERTICAL;

  ruler->backing_store = NULL;
  ruler->non_gr_exp_gc = NULL;
  ruler->metric = &ruler_widget_metrics[PIXELS];

  ruler->adjustment_observer.update = gtk_ruler_adjustment_update;
  ruler->adjustment_observer.disconnect = gtk_ruler_adjustment_disconnect;
  ruler->adjustment_observer.user_data = ruler;

  if (adjustment)
    ruler->adjustment = adjustment;
  else
    ruler->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  gtk_data_attach ((GtkData*) ruler->adjustment, &ruler->adjustment_observer);

  ruler->slider_size = 0;

  g_function_leave ("gtk_vruler_new");
  return ((GtkWidget*) ruler);
}

GtkData*
gtk_ruler_get_adjustment (GtkWidget *ruler)
{
  GtkRuler *rruler;
  GtkData *data;

  g_function_enter ("gtk_ruler_get_adjustment");

  g_assert (ruler != NULL);

  rruler = (GtkRuler*) ruler;
  data = (GtkData*) rruler->adjustment;

  g_function_leave ("gtk_ruler_get_adjustment");
  return data;
}

void
gtk_ruler_set_metric (GtkWidget *ruler,
		      gint metric)
{
  GtkRuler *rruler;

  g_function_enter ("gtk_ruler_set_metric");

  g_assert (ruler != NULL);

  rruler = (GtkRuler*) ruler;
  rruler->metric = &ruler_widget_metrics[metric];

  if (GTK_WIDGET_VISIBLE (ruler) && GTK_WIDGET_MAPPED (ruler))
    gtk_ruler_expose (ruler);

  g_function_leave ("gtk_ruler_set_metric");
}

guint16
gtk_get_ruler_type ()
{
  static guint16 ruler_type = 0;

  g_function_enter ("gtk_get_ruler_type");

  if (!ruler_type)
    gtk_widget_unique_type (&ruler_type);

  g_function_leave ("gtk_get_ruler_type");
  return ruler_type;
}

static void
gtk_ruler_destroy (GtkWidget *widget)
{
  GtkRuler * ruler;
  g_function_enter ("gtk_ruler_destroy");

  g_assert (widget != NULL);

  ruler = (GtkRuler*) widget;

  /*  Destroy the backing store pixmap  */
  if (ruler->backing_store)
    gdk_pixmap_destroy (ruler->backing_store);
  if (ruler->non_gr_exp_gc)
    gdk_gc_destroy (ruler->non_gr_exp_gc);

  gtk_data_detach ((GtkData*) ruler->adjustment, &ruler->adjustment_observer);
  gtk_data_destroy ((GtkData*) ruler->adjustment);

  g_free (ruler);

  g_function_leave ("gtk_ruler_destroy");
}

static void
gtk_ruler_realize (GtkWidget *widget)
{
  GtkRuler *ruler;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_ruler_realize");

  g_assert (widget != NULL);

  ruler = (GtkRuler*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_POINTER_MOTION_MASK |
			   GDK_POINTER_MOTION_HINT_MASK);

  ruler->widget.window = gdk_window_new (widget->parent->widget.window,
					 &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (ruler->widget.window, ruler);

  ruler->widget.style = gtk_style_attach (ruler->widget.style,
					  ruler->widget.window);

  gdk_window_set_background (ruler->widget.window, &widget->style->background[GTK_STATE_ACTIVE]);

  g_function_leave ("gtk_ruler_realize");
}

static void
gtk_ruler_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint        is_expose)
{
  g_function_enter ("gtk_ruler_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  gtk_ruler_expose (widget);

  g_function_leave ("gtk_ruler_draw");
}

static void
gtk_ruler_expose (GtkWidget *widget)
{
  GtkRuler *ruler;

  g_function_enter ("gtk_ruler_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      ruler = (GtkRuler*) widget;

      gtk_draw_shadow (widget->window,
		       widget->style->highlight_gc[GTK_STATE_NORMAL],
		       widget->style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       GTK_SHADOW_OUT,
		       0, 0,
		       widget->window->width,
		       widget->window->height,
		       widget->style->shadow_thickness);

      gtk_ruler_draw_ticks (widget);
      gtk_ruler_draw_pos (widget);
    }

  g_function_leave ("gtk_ruler_expose");
}

static void
gtk_ruler_draw_ticks (GtkWidget *widget)
{
  GtkRuler *ruler;
  GdkGC *gc;
  gint i, j;
  gint width, height;
  gint shadow_thickness;
  gint subd_levels;
  gint length;
  gfloat subd_incr[MINIMUM_INCR + 1];
  gfloat start, end, cur;
  gfloat pixels_per_mark;
  gchar unit_str[12];
  gint text_height;

  g_function_enter ("gtk_ruler_draw_ticks");

  g_assert (widget != NULL);
  ruler = (GtkRuler*) widget;

  gc = widget->style->foreground_gc[GTK_STATE_NORMAL];
  shadow_thickness = widget->style->shadow_thickness;
  pixels_per_mark = ruler->metric->pixels_per_unit * ruler->metric->mark_every_nth;
  text_height = widget->style->font->ascent + widget->style->font->descent;

  switch (ruler->orientation)
    {
    case HORIZONTAL:
      width = widget->window->width - shadow_thickness * 2;
      height = widget->window->height - shadow_thickness * 2;
      gdk_draw_line (widget->window, gc,
		     shadow_thickness,
		     height + shadow_thickness,
		     widget->window->width - shadow_thickness,
		     height + shadow_thickness);
      break;
    case VERTICAL:
      width = widget->window->height - shadow_thickness * 2;
      height = widget->window->width - shadow_thickness * 2;
      gdk_draw_line (widget->window, gc,
		     height + shadow_thickness,
		     shadow_thickness,
		     height + shadow_thickness,
		     widget->window->height - shadow_thickness);
      break;
    default:
      g_error ("unknown ruler orientation");
      break;
    }

  subd_levels = 0;
  subd_incr[0] = (width * pixels_per_mark) /
    (ruler->adjustment->upper - ruler->adjustment->lower);

  while (((subd_incr[subd_levels] / ruler->metric->subdivide[subd_levels]) > MINIMUM_INCR) &&
	 (subd_levels < MAXIMUM_SUBDIVIDE))
    {
      subd_incr[subd_levels+1] = subd_incr[subd_levels] /
	ruler->metric->subdivide[subd_levels];
      subd_levels++;
    }

  start = floor (ruler->adjustment->lower / pixels_per_mark) *
    subd_incr[0];
  end = ceil (ruler->adjustment->upper / pixels_per_mark) *
    subd_incr[0];

  for (i = 0; i <= subd_levels; i++)
    {
      length = height / (i + 1);

      j = 0;
      cur = start;
      while (cur <= end)
	{
	  sprintf (unit_str, "%d", ROUND (cur * ruler->metric->mark_every_nth / subd_incr[0]));

	  switch (ruler->orientation)
	    {
	    case HORIZONTAL:
	      gdk_draw_line (widget->window, gc,
			     ROUND (cur) + shadow_thickness,
			     height + shadow_thickness,
			     ROUND (cur) + shadow_thickness,
			     height - length + shadow_thickness);
	      if (!i)
		gdk_draw_string (widget->window, gc,
				 ROUND(cur) + shadow_thickness + 1,
				 shadow_thickness + text_height - 1,
				 unit_str);
	      break;
	    case VERTICAL:
	      gdk_draw_line (widget->window, gc,
			     height + shadow_thickness - length,
			     ROUND (cur) + shadow_thickness,
			     height + shadow_thickness,
			     ROUND (cur) + shadow_thickness);
	      if (!i)
		gdk_draw_string (widget->window, gc,
				 shadow_thickness + 1,
				 ROUND(cur) + shadow_thickness + text_height + 1,
				 unit_str);
	      break;
	    default:
	      g_error ("unknown ruler orientation");
	      break;
	    }

	  j++;
	  cur = start + j * subd_incr[i];
	}
    }

  g_function_leave ("gtk_ruler_draw_ticks");
}

static void
gtk_ruler_draw_pos (GtkWidget *widget)
{
  GtkRuler *ruler;
  GdkGC *gc1, *gc2;
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gint shadow_thickness;
  gfloat increment;

  g_function_enter ("gtk_ruler_draw_pos");

  g_assert (widget != NULL);
  ruler = (GtkRuler*) widget;

  gc1 = widget->style->highlight_gc[GTK_STATE_NORMAL];
  gc2 = widget->style->shadow_gc[GTK_STATE_NORMAL];
  shadow_thickness = widget->style->shadow_thickness;
  width = widget->window->width - shadow_thickness * 2;
  height = widget->window->height - shadow_thickness * 2;

  switch (ruler->orientation)
    {
    case HORIZONTAL:
      bs_width = 2;
      bs_height = height / 2 + 1;
      break;
    case VERTICAL:
      bs_width = width / 2 + 1;
      bs_height = 2;
      break;
    default:
      g_error ("unknown ruler orientation");
      break;
    }

  /*  If a backing store exists, restore the ruler  */
  if (ruler->backing_store && ruler->non_gr_exp_gc)
    gdk_draw_pixmap (ruler->widget.window, ruler->non_gr_exp_gc, ruler->backing_store,
		     0, 0, ruler->xsrc, ruler->ysrc, bs_width, bs_height);
  /*  Otherwise, create a new backing store  */
  else
    {
      ruler->backing_store = gdk_pixmap_new (ruler->widget.window,
					     bs_width, bs_height,
					     ruler->widget.window->depth);
      ruler->non_gr_exp_gc = gdk_gc_new (ruler->widget.window);
      gdk_gc_set_exposures (ruler->non_gr_exp_gc, FALSE);
    }

  switch (ruler->orientation)
    {
    case HORIZONTAL:
      increment = (gfloat) width / (ruler->adjustment->upper - ruler->adjustment->lower);

      x = ROUND (ruler->adjustment->value * increment) + shadow_thickness;
      y = height / 2 + shadow_thickness;

      /*  Before drawing the sliding marker, store the underlying ruler  */
      gdk_draw_pixmap (ruler->backing_store, ruler->non_gr_exp_gc, ruler->widget.window,
		       x, y, 0, 0, bs_width, bs_height);

      gtk_draw_vline (widget->window, gc2, gc1,
		      y, y + bs_height - 1, x, bs_width);
      break;
    case VERTICAL:
      increment = (gfloat) height / (ruler->adjustment->upper - ruler->adjustment->lower);

      x = width / 2 + shadow_thickness;
      y = ROUND (ruler->adjustment->value * increment) + shadow_thickness;

      /*  Before drawing the sliding marker, store the underlying ruler  */
      gdk_draw_pixmap (ruler->backing_store, ruler->non_gr_exp_gc, ruler->widget.window,
		       x, y, 0, 0, bs_width, bs_height);

      gtk_draw_hline (widget->window, gc2, gc1,
		      x, x + bs_width - 1, y, bs_height);
      break;
    default:
      g_error ("unknown ruler orientation");
      break;
    }

  ruler->xsrc = x;
  ruler->ysrc = y;

  g_function_leave ("gtk_ruler_draw_pos");
}

static gint
gtk_ruler_event (GtkWidget *widget,
		 GdkEvent  *event)
{
  GtkRuler *ruler;
  gint x, y;

  g_function_enter ("gtk_ruler_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  ruler = (GtkRuler*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (event->any.window == widget->window)
	gtk_ruler_expose (widget);
      break;

    case GDK_MOTION_NOTIFY:
      gdk_window_get_pointer (ruler->widget.window, &x, &y, NULL);

      switch (ruler->orientation)
	{
	case HORIZONTAL:
	  gtk_hruler_motion (widget, x);
	  break;
	case VERTICAL:
	  gtk_vruler_motion (widget, y);
	  break;
	default:
	  g_error ("unknown ruler orientation");
	  break;
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_ruler_event");
  return FALSE;
}

static gint
gtk_ruler_is_child (GtkWidget *widget,
		    GtkWidget *child)
{
  g_function_enter ("gtk_ruler_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  g_function_leave ("gtk_ruler_is_child");
  return FALSE;
}

static gint
gtk_ruler_locate (GtkWidget  *widget,
		  GtkWidget **child,
		  gint        x,
		  gint        y)
{
  g_function_enter ("gtk_ruler_locate");
  g_warning ("gtk_ruler_locate: UNFINISHED");
  g_function_leave ("gtk_ruler_locate");
  return FALSE;
}

static gint
gtk_ruler_adjustment_update (GtkObserver *observer,
			     GtkData     *data)
{
  GtkRuler *ruler;
  GtkDataAdjustment *adjustment;

  g_function_enter ("gtk_ruler_adjustment_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  ruler = (GtkRuler*) observer->user_data;
  g_assert (ruler != NULL);

  /*  Draw the sliding tick mark when the data we're looking at changes  */
  adjustment = (GtkDataAdjustment*) data;
  gtk_ruler_draw_pos (&(ruler->widget));

  g_function_leave ("gtk_ruler_adjustment_update");
  return FALSE;
}

static void
gtk_ruler_adjustment_disconnect (GtkObserver *observer,
				 GtkData     *data)
{
  g_function_enter ("gtk_ruler_adjustment_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_ruler_adjustment_disconnect");
}

static void
gtk_ruler_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  GtkRuler *ruler;

  g_function_enter ("gtk_ruler_size_allocate");

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

      ruler = (GtkRuler *) widget;
      if (ruler->backing_store)
	gdk_pixmap_destroy (ruler->backing_store);
      if (ruler->non_gr_exp_gc)
	gdk_gc_destroy (ruler->non_gr_exp_gc);

      ruler->backing_store = NULL;
      ruler->non_gr_exp_gc = NULL;
    }

  g_function_leave ("gtk_ruler_size_allocate");
}

static void
gtk_hruler_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  gint shadow_thickness;

  g_function_enter ("gtk_hruler_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = shadow_thickness * 2 + 1;
  requisition->height = RULER_WIDTH + shadow_thickness * 2;

  g_function_leave ("gtk_hruler_size_request");
}

static void
gtk_hruler_motion (GtkWidget *widget,
		   gint       x)
{
  GtkRuler *ruler;
  gfloat new_value;
  gint width;

  g_function_enter ("gtk_hruler_motion");

  g_assert (widget != NULL);
  ruler = (GtkRuler*) widget;

  width = widget->window->width - widget->style->shadow_thickness * 2;
  new_value = x * (ruler->adjustment->upper - ruler->adjustment->lower) / (gfloat) width;
  new_value = (new_value < ruler->adjustment->lower) ? ruler->adjustment->lower : new_value;
  new_value = (new_value > ruler->adjustment->upper) ? ruler->adjustment->upper : new_value;

  if (ruler->adjustment->value != new_value)
    {
      ruler->adjustment->value = new_value;
      gtk_data_notify ((GtkData*) ruler->adjustment);
    }

  g_function_leave ("gtk_hruler_motion");
}


static void
gtk_vruler_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  gint shadow_thickness;

  g_function_enter ("gtk_vruler_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = RULER_WIDTH + shadow_thickness * 2;
  requisition->height = shadow_thickness * 2 + 1;

  g_function_leave ("gtk_vruler_size_request");
}

static void
gtk_vruler_motion (GtkWidget *widget,
		   gint y)
{
  GtkRuler *ruler;
  gfloat new_value;
  gint height;

  g_function_enter ("gtk_vruler_motion");

  g_assert (widget != NULL);
  ruler = (GtkRuler*) widget;

  height = widget->window->height - widget->style->shadow_thickness * 2;
  new_value = y * (ruler->adjustment->upper - ruler->adjustment->lower) / (gfloat) height;
  new_value = (new_value < ruler->adjustment->lower) ? ruler->adjustment->lower : new_value;
  new_value = (new_value > ruler->adjustment->upper) ? ruler->adjustment->upper : new_value;

  if (ruler->adjustment->value != new_value)
    {
      ruler->adjustment->value = new_value;
      gtk_data_notify ((GtkData*) ruler->adjustment);
    }

  g_function_leave ("gtk_vruler_motion");
}
