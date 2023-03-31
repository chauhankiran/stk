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
#include "gtkscale.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define HORIZONTAL            0
#define VERTICAL              1

#define SLIDER_WIDTH          11
#define SLIDER_LENGTH         31
#define VALUE_SPACING         2

#define NONE                 -1
#define SLIDER                0
#define TROUGH                1

#define TROUGH_NONE           0
#define TROUGH_PAGE_BACK      1
#define TROUGH_PAGE_FORWARD   2

#define SCROLL_NONE           0
#define SCROLL_PAGE_BACK      1
#define SCROLL_PAGE_FORWARD   2

#define SCROLL_TIMER_LENGTH   20


typedef struct _GtkScale       GtkScale;

struct _GtkScale
{
  GtkWidget widget;
  GdkWindow *trough;
  GdkWindow *slider;

  gint16 in_child;
  gint16 click_child;
  gint16 click_point;
  guint32 timer;

  guint8 digits;
  unsigned int orientation : 1;
  unsigned int scroll_type : 2;
  unsigned int draw_value : 1;
  unsigned int value_pos : 2;

  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;

  GtkDataAdjustment *adjustment;
  GtkObserver adjustment_observer;
};

static void   gtk_scale_destroy               (GtkWidget       *widget);
static void   gtk_scale_realize               (GtkWidget       *widget);
static void   gtk_scale_draw                  (GtkWidget       *widget,
					       GdkRectangle    *area,
					       gint             is_expose);
static void   gtk_scale_draw_trough           (GtkWidget       *widget);
static void   gtk_scale_draw_slider           (GtkWidget       *widget);
static void   gtk_scale_draw_value            (GtkWidget       *widget);
static gint   gtk_scale_event                 (GtkWidget       *widget,
					       GdkEvent        *event);
static gint   gtk_scale_is_child              (GtkWidget       *widget,
					       GtkWidget       *child);
static gint   gtk_scale_locate                (GtkWidget       *widget,
					       GtkWidget      **child,
					       gint             x,
					       gint             y);

static gint   gtk_scale_adjustment_update     (GtkObserver *observer,
					       GtkData     *data);
static void   gtk_scale_adjustment_disconnect (GtkObserver *observer,
					       GtkData     *data);

static void   gtk_hscale_draw_value       (GtkWidget      *widget);
static void   gtk_hscale_size_request     (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void   gtk_hscale_size_allocate    (GtkWidget      *widget,
					   GtkAllocation  *allocation);
static void   gtk_hscale_calc_slider_pos  (GtkWidget      *widget);
static void   gtk_hscale_motion           (GtkWidget      *widget,
					   gint            delta);

static void   gtk_vscale_draw_value       (GtkWidget      *widget);
static void   gtk_vscale_size_request     (GtkWidget      *widget,
					   GtkRequisition *requistion);
static void   gtk_vscale_size_allocate    (GtkWidget      *widget,
					   GtkAllocation  *allocation);
static void   gtk_vscale_calc_slider_pos  (GtkWidget      *widget);
static void   gtk_vscale_motion           (GtkWidget      *widget,
					   gint            delta);

static gint   gtk_scale_check_trough_click (GtkWidget  *widget,
					    gint        x,
					    gint        y);
static void   gtk_scale_add_timer          (GtkWidget  *widget);
static void   gtk_scale_remove_timer       (GtkWidget  *widget);
static gint   gtk_scale_timer              (gpointer    data);
static gint   gtk_scale_value_width        (GtkWidget  *widget);


static GtkWidgetFunctions hscale_widget_functions =
{
  gtk_scale_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_scale_realize,
  gtk_scale_draw,
  gtk_widget_default_draw_focus,
  gtk_scale_event,
  gtk_hscale_size_request,
  gtk_hscale_size_allocate,
  gtk_scale_is_child,
  gtk_scale_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions vscale_widget_functions =
{
  gtk_scale_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_scale_realize,
  gtk_scale_draw,
  gtk_widget_default_draw_focus,
  gtk_scale_event,
  gtk_vscale_size_request,
  gtk_vscale_size_allocate,
  gtk_scale_is_child,
  gtk_scale_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};


GtkWidget*
gtk_hscale_new (GtkDataAdjustment *adjustment)
{
  GtkScale *scale;

  g_function_enter ("gtk_hscale_new");

  scale = g_new (GtkScale, 1);

  scale->widget.type = gtk_get_scale_type ();
  scale->widget.function_table = &hscale_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) scale);

  scale->trough = NULL;
  scale->slider = NULL;
  scale->in_child = NONE;
  scale->click_child = NONE;
  scale->click_point = 0;
  scale->timer = 0;

  scale->digits = 1;
  scale->orientation = HORIZONTAL;
  scale->scroll_type = SCROLL_NONE;
  scale->draw_value = TRUE;
  scale->value_pos = GTK_POS_BOTTOM;

  scale->adjustment_observer.update = gtk_scale_adjustment_update;
  scale->adjustment_observer.disconnect = gtk_scale_adjustment_disconnect;
  scale->adjustment_observer.user_data = scale;

  if (adjustment)
    scale->adjustment = adjustment;
  else
    scale->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  scale->old_value = scale->adjustment->value;
  scale->old_lower = scale->adjustment->lower;
  scale->old_upper = scale->adjustment->upper;

  gtk_data_attach ((GtkData*) scale->adjustment, &scale->adjustment_observer);

  g_function_leave ("gtk_hscale_new");
  return ((GtkWidget*) scale);
}

GtkWidget*
gtk_vscale_new (GtkDataAdjustment *adjustment)
{
  GtkScale *scale;

  g_function_enter ("gtk_vscale_new");

  scale = g_new (GtkScale, 1);

  scale->widget.type = gtk_get_scale_type ();
  scale->widget.function_table = &vscale_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) scale);

  scale->trough = NULL;
  scale->slider = NULL;
  scale->in_child = NONE;
  scale->click_child = NONE;
  scale->click_point = 0;
  scale->timer = 0;

  scale->digits = 1;
  scale->orientation = VERTICAL;
  scale->scroll_type = SCROLL_NONE;
  scale->draw_value = TRUE;
  scale->value_pos = GTK_POS_RIGHT;

  scale->adjustment_observer.update = gtk_scale_adjustment_update;
  scale->adjustment_observer.disconnect = gtk_scale_adjustment_disconnect;
  scale->adjustment_observer.user_data = scale;

  if (adjustment)
    scale->adjustment = adjustment;
  else
    scale->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  scale->old_value = scale->adjustment->value;
  scale->old_lower = scale->adjustment->lower;
  scale->old_upper = scale->adjustment->upper;

  gtk_data_attach ((GtkData*) scale->adjustment, &scale->adjustment_observer);

  g_function_leave ("gtk_vscale_new");
  return ((GtkWidget*) scale);
}

GtkData*
gtk_scale_get_adjustment (GtkWidget *scale)
{
  GtkScale *rscale;
  GtkData *data;

  g_function_enter ("gtk_scale_get_adjustment");

  g_assert (scale != NULL);

  rscale = (GtkScale*) scale;
  data = (GtkData*) rscale->adjustment;

  g_function_leave ("gtk_scale_get_adjustment");
  return data;
}

void gtk_scale_set_draw_value (GtkWidget *scale,
			       gint       draw_value)
{
  GtkScale *rscale;

  g_function_enter ("gtk_scale_set_draw_value");

  g_assert (scale != NULL);

  rscale = (GtkScale*) scale;
  if (rscale->draw_value != draw_value)
    {
      rscale->draw_value = draw_value;

      if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale))
	gtk_container_need_resize (scale->parent, scale);
    }

  g_function_leave ("gtk_scale_set_draw_value");
}

void gtk_scale_set_value_pos (GtkWidget *scale,
			      gint       value_pos)
{
  GtkScale *rscale;

  g_function_enter ("gtk_scale_set_value_pos");

  g_assert (scale != NULL);

  rscale = (GtkScale*) scale;
  if (rscale->value_pos != value_pos)
    {
      rscale->value_pos = value_pos;

      if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale))
	gtk_container_need_resize (scale->parent, scale);
    }

  g_function_leave ("gtk_scale_set_value_pos");
}

void
gtk_scale_set_digits (GtkWidget *scale,
		      gint       digits)
{
  GtkScale *rscale;

  g_function_enter ("gtk_scale_set_digits");

  g_assert (scale != NULL);

  rscale = (GtkScale*) scale;
  if (rscale->digits != digits)
    {
      rscale->digits = digits;

      if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale))
	gtk_container_need_resize (scale->parent, scale);
    }

  g_function_leave ("gtk_scale_set_digits");
}

guint16
gtk_get_scale_type ()
{
  static guint16 scale_type = 0;

  g_function_enter ("gtk_get_scale_type");

  if (!scale_type)
    gtk_widget_unique_type (&scale_type);

  g_function_leave ("gtk_get_scale_type");
  return scale_type;
}

static void
gtk_scale_destroy (GtkWidget *widget)
{
  GtkScale *scale;

  g_function_enter ("gtk_scale_destroy");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (scale->trough)
    gdk_window_destroy (scale->trough);
  if (scale->slider)
    gdk_window_destroy (scale->slider);
  if (scale->widget.window)
    gdk_window_destroy (scale->widget.window);

  gtk_scale_remove_timer (widget);
  gtk_data_detach ((GtkData*) scale->adjustment, &scale->adjustment_observer);
  gtk_data_destroy ((GtkData*) scale->adjustment);
  g_free (scale);

  g_function_leave ("gtk_scale_destroy");
}

static void
gtk_scale_realize (GtkWidget *widget)
{
  GtkScale *scale;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_scale_realize");

  g_assert (widget != NULL);

  scale = (GtkScale*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  switch (scale->orientation)
    {
    case HORIZONTAL:
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y + (widget->allocation.height - attributes.height) / 2;
      attributes.wclass = GDK_INPUT_OUTPUT;
      break;
    case VERTICAL:
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.x = widget->allocation.x + (widget->allocation.width - attributes.width) / 2;
      attributes.y = widget->allocation.y;
      attributes.wclass = GDK_INPUT_OUTPUT;
      break;
    }

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  scale->widget.window = gdk_window_new (widget->parent->widget.window,
					 &attributes, GDK_WA_X | GDK_WA_Y);

  attributes.x = 0;
  attributes.y = 0;
  switch (scale->orientation)
    {
    case HORIZONTAL:
      attributes.width = widget->allocation.width;
      attributes.height = widget->requisition.height;
      break;
    case VERTICAL:
      attributes.width = widget->requisition.width;
      attributes.height = widget->allocation.height;
      break;
    }

  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);
  scale->trough = gdk_window_new (scale->widget.window, &attributes, GDK_WA_X | GDK_WA_Y);

  switch (scale->orientation)
    {
    case HORIZONTAL:
      attributes.width = SLIDER_LENGTH;
      attributes.height = SLIDER_WIDTH;
      break;
    case VERTICAL:
      attributes.width = SLIDER_WIDTH;
      attributes.height = SLIDER_LENGTH;
      break;
    }

  attributes.event_mask |= (GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  scale->slider = gdk_window_new (scale->trough, &attributes, GDK_WA_X | GDK_WA_Y);

  scale->widget.style = gtk_style_attach (scale->widget.style, scale->widget.window);

  gdk_window_set_user_data (scale->widget.window, scale);
  gdk_window_set_user_data (scale->trough, scale);
  gdk_window_set_user_data (scale->slider, scale);

  gdk_window_set_background (scale->widget.window, &widget->style->background[GTK_STATE_NORMAL]);
  gdk_window_set_background (scale->slider, &widget->style->background[GTK_STATE_NORMAL]);
  gdk_window_set_background (scale->trough, &widget->style->background[GTK_STATE_ACTIVE]);

  switch (scale->orientation)
    {
    case HORIZONTAL:
      gtk_hscale_calc_slider_pos (widget);
      break;
    case VERTICAL:
      gtk_vscale_calc_slider_pos (widget);
      break;
    }

  gdk_window_show (scale->slider);
  gdk_window_show (scale->trough);

  g_function_leave ("gtk_scale_realize");
}

static void
gtk_scale_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint          is_expose)
{
  g_function_enter ("gtk_scale_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      gtk_scale_draw_trough (widget);
      gtk_scale_draw_slider (widget);
      gtk_scale_draw_value (widget);
    }

  g_function_leave ("gtk_scale_draw");
}

static void
gtk_scale_draw_trough (GtkWidget *widget)
{
  GtkScale *scale;

  g_function_enter ("gtk_scale_trough");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scale = (GtkScale*) widget;

      gtk_draw_shadow (scale->trough,
		       widget->style->highlight_gc[GTK_STATE_NORMAL],
		       widget->style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       GTK_SHADOW_IN,
		       0, 0,
		       scale->trough->width,
		       scale->trough->height,
		       widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_scale_draw_trough");
}

static void
gtk_scale_draw_slider (GtkWidget *widget)
{
  GtkScale *scale;
  GtkStateType state_type;
  gint shadow_thickness;

  g_function_enter ("gtk_scale_draw_slider");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scale = (GtkScale*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      if ((scale->in_child == SLIDER) || (scale->click_child == SLIDER))
	state_type = GTK_STATE_PRELIGHT;
      else
	state_type = GTK_STATE_NORMAL;

      gdk_window_set_background (scale->slider, &widget->style->background[state_type]);
      gdk_window_clear (scale->slider);

      switch (scale->orientation)
	{
	case HORIZONTAL:
	  gtk_draw_vline (scale->slider,
			  widget->style->highlight_gc[state_type],
			  widget->style->shadow_gc[state_type],
			  0, scale->slider->height,
			  scale->slider->width / 2,
			  shadow_thickness);
	  break;
	case VERTICAL:
	  gtk_draw_hline (scale->slider,
			  widget->style->highlight_gc[state_type],
			  widget->style->shadow_gc[state_type],
			  0, scale->slider->width,
			  scale->slider->height / 2,
			  shadow_thickness);
	  break;
	}

      gtk_draw_shadow (scale->slider,
		       widget->style->highlight_gc[state_type],
		       widget->style->shadow_gc[state_type],
		       NULL,
		       GTK_SHADOW_OUT,
		       0, 0,
		       scale->slider->width,
		       scale->slider->height,
		       shadow_thickness);
    }

  g_function_leave ("gtk_scale_draw_slider");
}

static void
gtk_scale_draw_value (GtkWidget *widget)
{
  GtkScale *scale;

  g_function_enter ("gtk_scale_draw_value");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale) && scale->draw_value)
    {
      switch (scale->orientation)
	{
	case HORIZONTAL:
	  gtk_hscale_draw_value (widget);
	  break;
	case VERTICAL:
	  gtk_vscale_draw_value (widget);
	  break;
	}
    }

  g_function_leave ("gtk_scale_draw_value");
}

static gint
gtk_scale_event (GtkWidget *widget,
		 GdkEvent  *event)
{
  GtkScale *scale;
  GdkModifierType mods;
  gint trough_part;
  gint x, y;

  g_function_enter ("gtk_scale_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  scale = (GtkScale*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (event->any.window == widget->window)
	gtk_scale_draw_value (widget);
      else if (event->any.window == scale->trough)
	gtk_scale_draw_trough (widget);
      else if (event->any.window == scale->slider)
	gtk_scale_draw_slider (widget);
      break;

    case GDK_MOTION_NOTIFY:
      if (scale->click_child == SLIDER)
	{
	  gdk_window_get_pointer (scale->slider, &x, &y, &mods);

	  if (event->any.send_event || (event->motion.state & GDK_BUTTON1_MASK))
	    {
	      switch (scale->orientation)
		{
		case HORIZONTAL:
		  gtk_hscale_motion (widget, x - scale->click_point);
		  break;
		case VERTICAL:
		  gtk_vscale_motion (widget, y - scale->click_point);
		  break;
		}
	    }
	}
      break;

    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
	{
	  gtk_grab_add (widget);

	  scale->click_child = scale->in_child;
	  if (scale->click_child == SLIDER)
	    {
	      gtk_scale_draw_slider (widget);
	      switch (scale->orientation)
		{
		case HORIZONTAL:
		  scale->click_point = event->button.x;
		  break;
		case VERTICAL:
		  scale->click_point = event->button.y;
		  break;
		}
	    }
	  else
	    {
	      trough_part = gtk_scale_check_trough_click (widget,
							  event->button.x,
							  event->button.y);

	      if (trough_part != TROUGH_NONE)
		{
		  scale->click_child = TROUGH;

		  if (trough_part == TROUGH_PAGE_BACK)
		    scale->scroll_type = SCROLL_PAGE_BACK;
		  else if (trough_part == TROUGH_PAGE_FORWARD)
		    scale->scroll_type = SCROLL_PAGE_FORWARD;

		  gtk_scale_timer (widget);
		  gtk_scale_add_timer (widget);
		}
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1)
	{
	  gtk_grab_remove (widget);

	  if (scale->click_child == SLIDER)
	    {
	      scale->click_child = NONE;
	      gtk_scale_draw_slider (widget);
	    }
	  else if (scale->click_child == TROUGH)
	    {
	      scale->click_child = NONE;
	      gtk_scale_remove_timer (widget);
	    }
	}
      break;

    case GDK_ENTER_NOTIFY:
      if (event->any.window == scale->slider)
	{
	  scale->in_child = SLIDER;
	  if ((scale->click_child == NONE) || (scale->click_child == TROUGH))
	    gtk_scale_draw_slider (widget);
	}
      break;

    case GDK_LEAVE_NOTIFY:
      if (event->any.window == scale->slider)
	{
	  scale->in_child = NONE;
	  if ((scale->click_child == NONE) || (scale->click_child == TROUGH))
	    gtk_scale_draw_slider (widget);
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_scale_event");
  return FALSE;
}

static gint
gtk_scale_is_child (GtkWidget *widget,
		    GtkWidget *child)
{
  g_function_enter ("gtk_scale_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  g_function_leave ("gtk_scale_is_child");
  return FALSE;
}

static gint
gtk_scale_locate (GtkWidget  *widget,
		  GtkWidget **child,
		  gint        x,
		  gint        y)
{
  g_function_enter ("gtk_scale_locate");
  g_warning ("gtk_scale_locate: UNFINISHED");
  g_function_leave ("gtk_scale_locate");
  return FALSE;
}

static gint
gtk_scale_adjustment_update (GtkObserver *observer,
			     GtkData     *data)
{
  GtkScale *scale;
  GtkDataAdjustment *adjustment;
  gfloat new_value;
  gint value_changed;

  g_function_enter ("gtk_scale_adjustment_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  scale = (GtkScale*) observer->user_data;
  g_assert (scale != NULL);

  adjustment = (GtkDataAdjustment*) data;

  value_changed = FALSE;
  if (((scale->old_lower != adjustment->lower) ||
       (scale->old_upper != adjustment->upper)) &&
      (scale->old_value == adjustment->value))
    {
      if (adjustment->lower == adjustment->upper)
	{
	  new_value = adjustment->lower;
	}
      else
	{
	  if (scale->old_lower != scale->old_upper)
	    {
	      new_value = (((adjustment->value - scale->old_lower) *
			    (adjustment->upper - adjustment->lower)) /
			   (scale->old_upper - scale->old_lower)) + adjustment->lower;
	    }
	  else
	    {
	      new_value = adjustment->lower;
	    }
	}

      if (adjustment->value != new_value)
	{
	  adjustment->value = new_value;
	  value_changed = TRUE;
	}
    }

  if ((scale->old_value != adjustment->value) ||
      (scale->old_lower != adjustment->lower) ||
      (scale->old_upper != adjustment->upper))
    {
      switch (scale->orientation)
	{
	case HORIZONTAL:
	  gtk_hscale_calc_slider_pos ((GtkWidget*) scale);
	  break;

	case VERTICAL:
	  gtk_vscale_calc_slider_pos ((GtkWidget*) scale);
	  break;

	default:
	  break;
	}

      scale->old_value = adjustment->value;
      scale->old_lower = adjustment->lower;
      scale->old_upper = adjustment->upper;

      gtk_scale_draw_value ((GtkWidget*) scale);
    }

  g_function_leave ("gtk_scale_adjustment_update");
  return value_changed;
}

static void
gtk_scale_adjustment_disconnect (GtkObserver *observer,
				 GtkData     *data)
{
  g_function_enter ("gtk_scale_adjustment_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_scale_adjustment_disconnect");
}

static void
gtk_hscale_draw_value (GtkWidget *widget)
{
  GtkScale *scale;
  gchar buffer[16];
  gint shadow_thickness;
  gint text_width;
  gint x, y;

  g_function_enter ("gtk_hscale_draw_value");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale) && scale->draw_value)
    {
      gdk_window_clear (widget->window);

      shadow_thickness = widget->style->shadow_thickness;
      sprintf (buffer, "%0.*f", scale->digits, scale->adjustment->value);
      text_width = gdk_string_width (widget->style->font, buffer);

      switch (scale->value_pos)
	{
	case GTK_POS_LEFT:
	  x = scale->trough->x - VALUE_SPACING - text_width;
	  y = (scale->trough->y + (scale->trough->height -
				   (widget->style->font->ascent +
				    widget->style->font->descent)) / 2 +
	       widget->style->font->ascent);
	  break;
	case GTK_POS_RIGHT:
	  x = scale->trough->x + scale->trough->width + VALUE_SPACING;
	  y = (scale->trough->y + (scale->trough->height -
				   (widget->style->font->ascent +
				    widget->style->font->descent)) / 2 +
	       widget->style->font->ascent);
	  break;
	case GTK_POS_TOP:
	  x = scale->slider->x + (scale->slider->width - text_width) / 2;
	  y = scale->trough->y - widget->style->font->descent;
	  break;
	case GTK_POS_BOTTOM:
	  x = scale->slider->x + (scale->slider->width - text_width) / 2;
	  y = scale->trough->y + scale->trough->height + widget->style->font->ascent;
	  break;
	}

      gdk_draw_string (widget->window,
		       widget->style->foreground_gc[GTK_STATE_NORMAL],
		       x, y, buffer);
    }

  g_function_leave ("gtk_hscale_draw_value");
}

static void
gtk_hscale_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint value_width;

  g_function_enter ("gtk_hscale_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  scale = (GtkScale*) widget;

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = SLIDER_LENGTH * 2 + (scale->widget.style->shadow_thickness) * 2;
  requisition->height = SLIDER_WIDTH + shadow_thickness * 2;

  if (scale->draw_value)
    {
      value_width = gtk_scale_value_width (widget);

      if ((scale->value_pos == GTK_POS_LEFT) ||
	  (scale->value_pos == GTK_POS_RIGHT))
	{
	  requisition->width += value_width + VALUE_SPACING;
	  if (requisition->height < (widget->style->font->ascent + widget->style->font->descent))
	    requisition->height = widget->style->font->ascent + widget->style->font->descent;
	}
      else if ((scale->value_pos == GTK_POS_TOP) ||
	       (scale->value_pos == GTK_POS_BOTTOM))
	{
	  if (requisition->width < value_width)
	    requisition->width = value_width;
	  requisition->height += widget->style->font->ascent + widget->style->font->descent;
	}
    }

  g_function_leave ("gtk_hscale_size_request");
}

static void
gtk_hscale_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint width, height;
  gint x, y;

  g_function_enter ("gtk_hscale_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      scale = (GtkScale*) widget;
      gdk_window_move (widget->window, allocation->x, allocation->y);
      gdk_window_set_size (widget->window, allocation->width, allocation->height);

      if (scale->draw_value)
	{
	  shadow_thickness = widget->style->shadow_thickness;
	  width = allocation->width;
	  height = SLIDER_WIDTH + shadow_thickness * 2;
	  x = 0;
	  y = 0;

	  switch (scale->value_pos)
	    {
	    case GTK_POS_LEFT:
	      x += gtk_scale_value_width (widget) + VALUE_SPACING;
	      width -= x;
	      y = (allocation->height - height) / 2;
	      break;
	    case GTK_POS_RIGHT:
	      width -= gtk_scale_value_width (widget) + VALUE_SPACING;
	      y = (allocation->height - height) / 2;
	      break;
	    case GTK_POS_TOP:
	      y = (widget->style->font->ascent + widget->style->font->descent +
		   (allocation->height - widget->requisition.height) / 2);
	      break;
	    case GTK_POS_BOTTOM:
	      y = (allocation->height - widget->requisition.height) / 2;
	      break;
	    }
	}
      else
	{
	  width = allocation->width;
	  height = widget->requisition.height;
	  x = 0;
	  y = (allocation->height - height) / 2;
	}

      gdk_window_move (scale->trough, x, y);
      gdk_window_set_size (scale->trough, width, height);
      gtk_hscale_calc_slider_pos (widget);
    }

  g_function_leave ("gtk_hscale_size_allocate");
}

static void
gtk_hscale_calc_slider_pos (GtkWidget *widget)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint left;
  gint right;
  gint x;

  g_function_enter ("gtk_hscale_calc_slider_pos");

  g_assert (widget != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    {
      scale = (GtkScale*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      left = shadow_thickness;
      right = scale->trough->width - left * 2;
      x = left;

      if ((scale->adjustment->value < scale->adjustment->lower) ||
	  (scale->adjustment->value > scale->adjustment->upper))
	g_error ("value not in range: %0.2f (%0.2f - %)",
		 scale->adjustment->value,
		 scale->adjustment->lower,
		 scale->adjustment->upper);

      if (scale->adjustment->lower != scale->adjustment->upper)
	x += ((right - left - SLIDER_LENGTH) * (scale->adjustment->value - scale->adjustment->lower) /
	      (scale->adjustment->upper - scale->adjustment->lower));

      if (x < left)
	x = left;
      if (x > right)
	x = right;

      if ((scale->slider->x != x) ||
	  (scale->slider->y != shadow_thickness))
	gdk_window_move (scale->slider,
			 x,
			 shadow_thickness);
    }

  g_function_leave ("gtk_hscale_calc_slider_pos");
}

static void
gtk_hscale_motion (GtkWidget *widget,
		   gint       delta)
{
  GtkScale *scale;
  gint left;
  gint right;
  gint new_pos;

  g_function_enter ("gtk_hscale_motion");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  new_pos = scale->slider->x + delta;
  left = widget->style->shadow_thickness;
  right = scale->trough->width - left * 2 - SLIDER_LENGTH + 1;

  if (right < left)
    right = left;
  if (new_pos < left)
    new_pos = left;
  else if (new_pos > right)
    new_pos = right;

  if (scale->slider->x != new_pos)
    {
      scale->adjustment->value = ((scale->adjustment->upper - scale->adjustment->lower) *
				  (new_pos - left) / (right - left) + scale->adjustment->lower);
      gtk_data_notify ((GtkData*) scale->adjustment);

      gdk_window_move (scale->slider,
		       new_pos,
		       scale->slider->y);
    }

  g_function_leave ("gtk_hscale_motion");
}


static void
gtk_vscale_draw_value (GtkWidget *widget)
{
  GtkScale *scale;
  gchar buffer[16];
  gint shadow_thickness;
  gint text_width;
  gint x, y;

  g_function_enter ("gtk_vscale_draw_value");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (GTK_WIDGET_VISIBLE (scale) && GTK_WIDGET_MAPPED (scale) && scale->draw_value)
    {
      gdk_window_clear (widget->window);

      shadow_thickness = widget->style->shadow_thickness;
      sprintf (buffer, "%0.*f", scale->digits, scale->adjustment->value);
      text_width = gdk_string_width (widget->style->font, buffer);

      switch (scale->value_pos)
	{
	case GTK_POS_LEFT:
	  x = scale->trough->x - VALUE_SPACING - text_width;
	  y = (scale->slider->y + (scale->slider->height -
				   (widget->style->font->ascent +
				    widget->style->font->descent)) / 2 +
	       widget->style->font->ascent);
	  break;
	case GTK_POS_RIGHT:
	  x = scale->trough->x + scale->trough->width + VALUE_SPACING;
	  y = (scale->slider->y + (scale->slider->height -
				   (widget->style->font->ascent +
				    widget->style->font->descent)) / 2 +
	       widget->style->font->ascent);
	  break;
	case GTK_POS_TOP:
	  x = scale->trough->x + (scale->trough->width - text_width) / 2;
	  y = scale->trough->y - widget->style->font->descent;
	  break;
	case GTK_POS_BOTTOM:
	  x = scale->trough->x + (scale->trough->width - text_width) / 2;
	  y = scale->trough->y + scale->trough->height + widget->style->font->ascent;
	  break;
	}

      gdk_draw_string (widget->window,
		       widget->style->foreground_gc[GTK_STATE_NORMAL],
		       x, y, buffer);
    }

  g_function_leave ("gtk_vscale_draw_value");
}

static void
gtk_vscale_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint value_width;

  g_function_enter ("gtk_vscale_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  scale = (GtkScale*) widget;

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = SLIDER_WIDTH + shadow_thickness * 2;
  requisition->height = SLIDER_LENGTH * 2 + (scale->widget.style->shadow_thickness) * 2;

  if (scale->draw_value)
    {
      value_width = gtk_scale_value_width (widget);

      if ((scale->value_pos == GTK_POS_LEFT) ||
	  (scale->value_pos == GTK_POS_RIGHT))
	{
	  requisition->width += value_width + VALUE_SPACING;
	  if (requisition->height < (widget->style->font->ascent + widget->style->font->descent))
	    requisition->height = widget->style->font->ascent + widget->style->font->descent;
	}
      else if ((scale->value_pos == GTK_POS_TOP) ||
	       (scale->value_pos == GTK_POS_BOTTOM))
	{
	  if (requisition->width < value_width)
	    requisition->width = value_width;
	  requisition->height += widget->style->font->ascent + widget->style->font->descent;
	}
    }

  g_function_leave ("gtk_vscale_size_request");
}

static void
gtk_vscale_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint width, height;
  gint x, y;

  g_function_enter ("gtk_vscale_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      scale = (GtkScale*) widget;
      gdk_window_move (widget->window, allocation->x, allocation->y);
      gdk_window_set_size (widget->window, allocation->width, allocation->height);

      if (scale->draw_value)
	{
	  shadow_thickness = widget->style->shadow_thickness;

	  width = SLIDER_WIDTH + shadow_thickness * 2;
	  height = widget->allocation.height;
	  x = (allocation->width - width) / 2;
	  y = 0;

	  switch (scale->value_pos)
	    {
	    case GTK_POS_LEFT:
	      x = (gtk_scale_value_width (widget) + (allocation->width - widget->requisition.width) / 2);
	      break;
	    case GTK_POS_RIGHT:
	      x = (allocation->width - widget->requisition.width) / 2;
	      break;
	    case GTK_POS_TOP:
	      y = widget->style->font->ascent + widget->style->font->descent;
	      height -= y;
	      break;
	    case GTK_POS_BOTTOM:
	      height -= widget->style->font->ascent + widget->style->font->descent;
	      break;
	    }
	}
      else
	{
	  width = widget->requisition.width;
	  height = widget->allocation.height;
	  x = (allocation->width - width) / 2;
	  y = 0;
	}

      gdk_window_move (scale->trough, x, y);
      gdk_window_set_size (scale->trough, width, height);
      gtk_vscale_calc_slider_pos (widget);
    }

  g_function_leave ("gtk_vscale_size_allocate");
}

static void
gtk_vscale_calc_slider_pos (GtkWidget *widget)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint top;
  gint bottom;
  gint y;

  g_function_enter ("gtk_vscale_calc_slider_pos");

  g_assert (widget != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    {
      scale = (GtkScale*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      top = shadow_thickness;
      bottom = scale->trough->height - top * 2;
      y = top;

      if ((scale->adjustment->value < scale->adjustment->lower) ||
	  (scale->adjustment->value > scale->adjustment->upper))
	g_error ("value not in range: %0.2f (%0.2f - %0.2f)",
		 scale->adjustment->value,
		 scale->adjustment->lower,
		 scale->adjustment->upper);

      if (scale->adjustment->lower != scale->adjustment->upper)
	y += ((bottom - top - SLIDER_LENGTH) * (scale->adjustment->value - scale->adjustment->lower) /
	      (scale->adjustment->upper - scale->adjustment->lower));

      if (y < top)
	y = top;
      if (y > bottom)
	y = bottom;

      if ((scale->slider->x != shadow_thickness) ||
	  (scale->slider->y != y))
	gdk_window_move (scale->slider,
			 shadow_thickness,
			 y);
    }

  g_function_leave ("gtk_vscale_calc_slider_pos");
}

static void
gtk_vscale_motion (GtkWidget *widget,
		   gint       delta)
{
  GtkScale *scale;
  gint top;
  gint bottom;
  gint new_pos;

  g_function_enter ("gtk_vscale_motion");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  new_pos = scale->slider->y + delta;
  top = widget->style->shadow_thickness;
  bottom = scale->trough->height - top * 2 - SLIDER_LENGTH + 1;

  if (bottom < top)
    bottom = top;
  if (new_pos < top)
    new_pos = top;
  else if (new_pos > bottom)
    new_pos = bottom;

  if (scale->slider->y != new_pos)
    {
      scale->adjustment->value = ((scale->adjustment->upper - scale->adjustment->lower) *
				  (new_pos - top) / (bottom - top) + scale->adjustment->lower);
      gtk_data_notify ((GtkData*) scale->adjustment);

      gdk_window_move (scale->slider,
		       scale->slider->x,
		       new_pos);
    }

  g_function_leave ("gtk_vscale_motion");
}

static gint
gtk_scale_check_trough_click (GtkWidget *widget,
			      gint       x,
			      gint       y)
{
  GtkScale *scale;
  gint shadow_thickness;
  gint return_val;

  g_function_enter ("gtk_scale_check_trough_click");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  return_val = TROUGH_NONE;

  shadow_thickness = widget->style->shadow_thickness;
  if ((x > shadow_thickness) &&
      (y > shadow_thickness) &&
      (x < (widget->window->width - shadow_thickness)) &&
      (y < (widget->window->height - shadow_thickness)))
    {
      switch (scale->orientation)
	{
	case HORIZONTAL:
	  if (x < scale->slider->x)
	    return_val = TROUGH_PAGE_BACK;
	  else
	    return_val = TROUGH_PAGE_FORWARD;
	  break;
	case VERTICAL:
	  if (y < scale->slider->y)
	    return_val = TROUGH_PAGE_BACK;
	  else
	    return_val = TROUGH_PAGE_FORWARD;
	  break;
	}
    }

  g_function_leave ("gtk_scale_check_trough_click");
  return return_val;
}

static void
gtk_scale_add_timer (GtkWidget *widget)
{
  GtkScale *scale;

  g_function_enter ("gtk_scale_add_timer");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (scale->timer)
    g_error ("already a scale timer in progress");

  scale->timer = gtk_timeout_add (SCROLL_TIMER_LENGTH,
				      gtk_scale_timer,
				      (gpointer) scale);

  g_function_leave ("gtk_scale_add_timer");
}

static void
gtk_scale_remove_timer (GtkWidget *widget)
{
  GtkScale *scale;

  g_function_enter ("gtk_scale_remove_timer");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  if (scale->timer)
    {
      gtk_timeout_remove (scale->timer);
      scale->timer = 0;
    }

  g_function_leave ("gtk_scale_remove_timer");
}

static gint
gtk_scale_timer (gpointer data)
{
  GtkScale *scale;
  gfloat new_value;
  gint return_val;

  g_function_enter ("gtk_scale_timer");

  g_assert (data != NULL);
  scale = (GtkScale*) data;

  new_value = scale->adjustment->value;
  return_val = TRUE;

  switch (scale->scroll_type)
    {
    case SCROLL_NONE:
      break;

    case SCROLL_PAGE_BACK:
      new_value -= scale->adjustment->page_increment;
      if (new_value < scale->adjustment->lower)
	{
	  new_value = scale->adjustment->lower;
	  return_val = FALSE;
	  scale->timer = 0;
	}
      break;

    case SCROLL_PAGE_FORWARD:
      new_value += scale->adjustment->page_increment;
      if (new_value > scale->adjustment->upper)
	{
	  new_value = scale->adjustment->upper;
	  return_val = FALSE;
	  scale->timer = 0;
	}
      break;
    }

  if (new_value != scale->adjustment->value)
    {
      scale->adjustment->value = new_value;
      gtk_data_notify ((GtkData*) scale->adjustment);

      switch (scale->orientation)
	{
	case HORIZONTAL:
	  gtk_hscale_calc_slider_pos ((GtkWidget*) scale);
	  break;
	case VERTICAL:
	  gtk_vscale_calc_slider_pos ((GtkWidget*) scale);
	  break;
	}
    }

  g_function_leave ("gtk_scale_timer");
  return return_val;
}

static gint
gtk_scale_value_width (GtkWidget *widget)
{
  GtkScale *scale;
  gchar buffer[16];
  gfloat value;
  gint temp;
  gint return_val;
  gint digits;
  gint i, j;

  g_function_enter ("gtk_scale_value_width");

  g_assert (widget != NULL);
  scale = (GtkScale*) widget;

  return_val = 0;
  if (scale->draw_value)
    {
      value = ABS (scale->adjustment->lower);
      if (value == 0) value = 1;
      digits = log10 (value) + 1;
      if (digits > 13)
	digits = 13;

      i = 0;
      if (scale->adjustment->lower < 0)
	buffer[i++] = '-';
      for (j = 0; j < digits; j++)
	buffer[i++] = '0';
      if (scale->digits)
	buffer[i++] = '.';
      for (j = 0; j < scale->digits; j++)
	buffer[i++] = '0';
      buffer[i] = '\0';

      return_val = gdk_string_width (widget->style->font, buffer);

      value = ABS (scale->adjustment->upper);
      if (value == 0) value = 1;
      digits = log10 (value) + 1;
      if (digits > 13)
	digits = 13;

      i = 0;
      if (scale->adjustment->lower < 0)
	buffer[i++] = '-';
      for (j = 0; j < digits; j++)
	buffer[i++] = '0';
      if (scale->digits)
	buffer[i++] = '.';
      for (j = 0; j < scale->digits; j++)
	buffer[i++] = '0';
      buffer[i] = '\0';

      temp = gdk_string_width (widget->style->font, buffer);
      return_val = MAX (return_val, temp);
    }

  g_function_leave ("gtk_scale_value_width");
  return return_val;
}
