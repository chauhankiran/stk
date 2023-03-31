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
#include "gtkdraw.h"
#include "gtkmain.h"
#include "gtkscrollbar.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define EPSILON               0.01

#define HORIZONTAL            0
#define VERTICAL              1

#define SLIDER_WIDTH          11
#define ARROW_LENGTH          11
#define MIN_SLIDER_LENGTH     7
#define ARROW_SLIDER_SPACING  1

#define NONE                 -1
#define UP_LEFT_ARROW         0
#define DOWN_RIGHT_ARROW      1
#define SLIDER                2
#define TROUGH                3

#define TROUGH_NONE           0
#define TROUGH_PAGE_BACK      1
#define TROUGH_PAGE_FORWARD   2

#define SCROLL_NONE           0
#define SCROLL_STEP_BACK      1
#define SCROLL_STEP_FORWARD   2
#define SCROLL_PAGE_BACK      3
#define SCROLL_PAGE_FORWARD   4

#define SCROLL_TIMER_LENGTH   20


typedef struct _GtkScrollBar       GtkScrollBar;

struct _GtkScrollBar
{
  GtkWidget widget;
  GdkWindow *up_left_arrow;
  GdkWindow *down_right_arrow;
  GdkWindow *slider;

  gint16 orientation;
  gint16 in_child;
  gint16 click_child;
  gint16 click_point;
  gint16 scroll_type;
  guint32 timer;

  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;
  gfloat old_page_size;

  GtkDataAdjustment *adjustment;
  GtkObserver adjustment_observer;

  gint slider_size;
};

static void   gtk_scrollbar_destroy               (GtkWidget       *widget);
static void   gtk_scrollbar_realize               (GtkWidget       *widget);
static void   gtk_scrollbar_draw                  (GtkWidget       *widget,
						   GdkRectangle    *area,
						   gint             is_expose);
static void   gtk_scrollbar_draw_trough           (GtkWidget       *widget);
static void   gtk_scrollbar_draw_up_left_arrow    (GtkWidget       *widget);
static void   gtk_scrollbar_draw_down_right_arrow (GtkWidget       *widget);
static void   gtk_scrollbar_draw_slider           (GtkWidget       *widget);
static gint   gtk_scrollbar_event                 (GtkWidget       *widget,
						   GdkEvent        *event);
static gint   gtk_scrollbar_is_child              (GtkWidget       *widget,
						   GtkWidget       *child);
static gint   gtk_scrollbar_locate                (GtkWidget       *widget,
						   GtkWidget      **child,
						   gint             x,
						   gint             y);

static gint   gtk_scrollbar_adjustment_update     (GtkObserver *observer,
						   GtkData     *data);
static void   gtk_scrollbar_adjustment_disconnect (GtkObserver *observer,
						   GtkData     *data);

static void   gtk_hscrollbar_size_request     (GtkWidget      *widget,
					       GtkRequisition *requisition);
static void   gtk_hscrollbar_size_allocate    (GtkWidget      *widget,
					       GtkAllocation  *allocation);
static void   gtk_hscrollbar_calc_child_sizes (GtkWidget      *widget);
static void   gtk_hscrollbar_calc_slider_size (GtkWidget      *widget);
static void   gtk_hscrollbar_calc_slider_pos  (GtkWidget      *widget);
static void   gtk_hscrollbar_motion           (GtkWidget      *widget,
				               gint            delta);

static void   gtk_vscrollbar_size_request     (GtkWidget      *widget,
					       GtkRequisition *requistion);
static void   gtk_vscrollbar_size_allocate    (GtkWidget      *widget,
					       GtkAllocation  *allocation);
static void   gtk_vscrollbar_calc_child_sizes (GtkWidget      *widget);
static void   gtk_vscrollbar_calc_slider_size (GtkWidget      *widget);
static void   gtk_vscrollbar_calc_slider_pos  (GtkWidget      *widget);
static void   gtk_vscrollbar_motion           (GtkWidget      *widget,
				               gint            delta);

static gint   gtk_scrollbar_check_trough_click (GtkWidget  *widget,
						gint        x,
						gint        y);
static void   gtk_scrollbar_add_timer          (GtkWidget  *widget);
static void   gtk_scrollbar_remove_timer       (GtkWidget  *widget);
static gint   gtk_scrollbar_timer              (gpointer    data);


static GtkWidgetFunctions hscrollbar_widget_functions =
{
  gtk_scrollbar_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_scrollbar_realize,
  gtk_scrollbar_draw,
  gtk_widget_default_draw_focus,
  gtk_scrollbar_event,
  gtk_hscrollbar_size_request,
  gtk_hscrollbar_size_allocate,
  gtk_scrollbar_is_child,
  gtk_scrollbar_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions vscrollbar_widget_functions =
{
  gtk_scrollbar_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_scrollbar_realize,
  gtk_scrollbar_draw,
  gtk_widget_default_draw_focus,
  gtk_scrollbar_event,
  gtk_vscrollbar_size_request,
  gtk_vscrollbar_size_allocate,
  gtk_scrollbar_is_child,
  gtk_scrollbar_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};


GtkWidget*
gtk_hscrollbar_new (GtkDataAdjustment *adjustment)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_hscrollbar_new");

  scrollbar = g_new (GtkScrollBar, 1);

  scrollbar->widget.type = gtk_get_scrollbar_type ();
  scrollbar->widget.function_table = &hscrollbar_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) scrollbar);

  scrollbar->up_left_arrow = NULL;
  scrollbar->down_right_arrow = NULL;
  scrollbar->slider = NULL;
  scrollbar->orientation = HORIZONTAL;
  scrollbar->in_child = NONE;
  scrollbar->click_child = NONE;
  scrollbar->click_point = 0;
  scrollbar->scroll_type = SCROLL_NONE;
  scrollbar->timer = 0;

  scrollbar->adjustment_observer.update = gtk_scrollbar_adjustment_update;
  scrollbar->adjustment_observer.disconnect = gtk_scrollbar_adjustment_disconnect;
  scrollbar->adjustment_observer.user_data = scrollbar;

  if (adjustment)
    scrollbar->adjustment = adjustment;
  else
    scrollbar->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  scrollbar->old_value = scrollbar->adjustment->value;
  scrollbar->old_lower = scrollbar->adjustment->lower;
  scrollbar->old_upper = scrollbar->adjustment->upper;
  scrollbar->old_page_size = scrollbar->adjustment->page_size;

  gtk_data_attach ((GtkData*) scrollbar->adjustment, &scrollbar->adjustment_observer);

  scrollbar->slider_size = 0;

  g_function_leave ("gtk_hscrollbar_new");
  return ((GtkWidget*) scrollbar);
}

GtkWidget*
gtk_vscrollbar_new (GtkDataAdjustment *adjustment)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_vscrollbar_new");

  scrollbar = g_new (GtkScrollBar, 1);

  scrollbar->widget.type = gtk_get_scrollbar_type ();
  scrollbar->widget.function_table = &vscrollbar_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) scrollbar);

  scrollbar->up_left_arrow = NULL;
  scrollbar->down_right_arrow = NULL;
  scrollbar->slider = NULL;
  scrollbar->orientation = VERTICAL;
  scrollbar->in_child = NONE;
  scrollbar->click_child = NONE;
  scrollbar->click_point = 0;
  scrollbar->scroll_type = SCROLL_NONE;
  scrollbar->timer = 0;

  scrollbar->adjustment_observer.update = gtk_scrollbar_adjustment_update;
  scrollbar->adjustment_observer.disconnect = gtk_scrollbar_adjustment_disconnect;
  scrollbar->adjustment_observer.user_data = scrollbar;

  if (adjustment)
    scrollbar->adjustment = adjustment;
  else
    scrollbar->adjustment = (GtkDataAdjustment*)
      gtk_data_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  scrollbar->old_value = scrollbar->adjustment->value;
  scrollbar->old_lower = scrollbar->adjustment->lower;
  scrollbar->old_upper = scrollbar->adjustment->upper;
  scrollbar->old_page_size = scrollbar->adjustment->page_size;

  gtk_data_attach ((GtkData*) scrollbar->adjustment, &scrollbar->adjustment_observer);

  scrollbar->slider_size = 0;

  g_function_leave ("gtk_vscrollbar_new");
  return ((GtkWidget*) scrollbar);
}

GtkData*
gtk_scrollbar_get_adjustment (GtkWidget *scrollbar)
{
  GtkScrollBar *rscrollbar;
  GtkData *data;

  g_function_enter ("gtk_scrollbar_get_adjustment");

  g_assert (scrollbar != NULL);

  rscrollbar = (GtkScrollBar*) scrollbar;
  data = (GtkData*) rscrollbar->adjustment;

  g_function_leave ("gtk_scrollbar_get_adjustment");
  return data;
}

guint16
gtk_get_scrollbar_type ()
{
  static guint16 scrollbar_type = 0;

  g_function_enter ("gtk_get_scrollbar_type");

  if (!scrollbar_type)
    gtk_widget_unique_type (&scrollbar_type);

  g_function_leave ("gtk_get_scrollbar_type");
  return scrollbar_type;
}

static void
gtk_scrollbar_destroy (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_scrollbar_destroy");

  g_assert (widget != NULL);
  scrollbar = (GtkScrollBar*) widget;

  if (scrollbar->up_left_arrow)
    gdk_window_destroy (scrollbar->up_left_arrow);
  if (scrollbar->down_right_arrow)
    gdk_window_destroy (scrollbar->down_right_arrow);
  if (scrollbar->slider)
    gdk_window_destroy (scrollbar->slider);
  if (scrollbar->widget.window)
    gdk_window_destroy (scrollbar->widget.window);

  gtk_scrollbar_remove_timer (widget);
  gtk_data_detach ((GtkData*) scrollbar->adjustment, &scrollbar->adjustment_observer);
  gtk_data_destroy ((GtkData*) scrollbar->adjustment);
  g_free (scrollbar);

  g_function_leave ("gtk_scrollbar_destroy");
}

static void
gtk_scrollbar_realize (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_scrollbar_realize");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_realize");

  scrollbar = (GtkScrollBar*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  switch (scrollbar->orientation)
    {
    case HORIZONTAL:
      attributes.width = widget->allocation.width;
      attributes.height = widget->requisition.height;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y + (widget->allocation.height - attributes.height) / 2;
      attributes.wclass = GDK_INPUT_OUTPUT;
      break;
    case VERTICAL:
      attributes.width = widget->requisition.width;
      attributes.height = widget->allocation.height;
      attributes.x = widget->allocation.x + (widget->allocation.width - attributes.width) / 2;
      attributes.y = widget->allocation.y;
      attributes.wclass = GDK_INPUT_OUTPUT;
      break;
    default:
      g_error ("unknown scrollbar orientation");
      break;
    }

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK);

  scrollbar->widget.window = gdk_window_new (widget->parent->widget.window,
					     &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (scrollbar->widget.window, scrollbar);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 1;
  attributes.height = 1;

  scrollbar->up_left_arrow = gdk_window_new (scrollbar->widget.window,
					     &attributes, GDK_WA_X | GDK_WA_Y);
  scrollbar->down_right_arrow = gdk_window_new (scrollbar->widget.window,
						&attributes, GDK_WA_X | GDK_WA_Y);

  attributes.event_mask |= (GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  scrollbar->slider = gdk_window_new (scrollbar->widget.window, &attributes, GDK_WA_X | GDK_WA_Y);

  scrollbar->widget.style = gtk_style_attach (scrollbar->widget.style,
					      scrollbar->widget.window);

  gdk_window_set_user_data (scrollbar->up_left_arrow, scrollbar);
  gdk_window_set_user_data (scrollbar->down_right_arrow, scrollbar);
  gdk_window_set_user_data (scrollbar->slider, scrollbar);

  gdk_window_set_background (scrollbar->widget.window, &widget->style->background[GTK_STATE_ACTIVE]);
  gdk_window_set_background (scrollbar->slider, &widget->style->background[GTK_STATE_NORMAL]);
  gdk_window_set_background (scrollbar->up_left_arrow, &widget->style->background[GTK_STATE_ACTIVE]);
  gdk_window_set_background (scrollbar->down_right_arrow, &widget->style->background[GTK_STATE_ACTIVE]);

  switch (scrollbar->orientation)
    {
    case HORIZONTAL:
      gtk_hscrollbar_calc_child_sizes (widget);
      break;
    case VERTICAL:
      gtk_vscrollbar_calc_child_sizes (widget);
      break;
    default:
      g_error ("unknown scrollbar orientation");
      break;
    }

  gdk_window_show (scrollbar->up_left_arrow);
  gdk_window_show (scrollbar->down_right_arrow);
  gdk_window_show (scrollbar->slider);

  g_function_leave ("gtk_scrollbar_realize");
}

static void
gtk_scrollbar_draw (GtkWidget    *widget,
		    GdkRectangle *area,
		    gint        is_expose)
{
  g_function_enter ("gtk_scrollbar_draw");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_draw");

  if (!area)
    g_error ("passed NULL area to gtk_scrollbar_draw");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      gtk_scrollbar_draw_trough (widget);
      gtk_scrollbar_draw_up_left_arrow (widget);
      gtk_scrollbar_draw_down_right_arrow (widget);
      gtk_scrollbar_draw_slider (widget);
    }

  g_function_leave ("gtk_scrollbar_draw");
}

static void
gtk_scrollbar_draw_trough (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_scrollbar_trough");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_draw_trough");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;

      gtk_draw_shadow (widget->window,
		       widget->style->highlight_gc[GTK_STATE_NORMAL],
		       widget->style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       GTK_SHADOW_IN,
		       0, 0,
		       widget->window->width,
		       widget->window->height,
		       widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_scrollbar_draw_trough");
}

static void
gtk_scrollbar_draw_up_left_arrow (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  GtkArrowType arrow_type;
  GtkStateType state_type;
  GtkShadowType shadow_type;

  g_function_enter ("gtk_scrollbar_draw_up_left_arrow");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_draw_up_left_arrow");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;

      switch (scrollbar->orientation)
	{
	case HORIZONTAL:
	  arrow_type = GTK_ARROW_LEFT;
	  break;
	case VERTICAL:
	  arrow_type = GTK_ARROW_UP;
	  break;
	default:
	  g_error ("unknown scrollbar orientation");
	  arrow_type = 0;
	  break;
	}

      if ((scrollbar->in_child == UP_LEFT_ARROW) || (scrollbar->click_child == UP_LEFT_ARROW))
	state_type = GTK_STATE_PRELIGHT;
      else
	state_type = GTK_STATE_NORMAL;

      if (scrollbar->click_child == UP_LEFT_ARROW)
	shadow_type = GTK_SHADOW_IN;
      else
	shadow_type = GTK_SHADOW_OUT;

      gtk_draw_arrow (scrollbar->up_left_arrow,
		      widget->style->highlight_gc[state_type],
		      widget->style->shadow_gc[state_type],
		      widget->style->background_gc[state_type],
		      arrow_type,
		      shadow_type,
		      0, 0,
		      scrollbar->up_left_arrow->width,
		      scrollbar->up_left_arrow->height,
		      widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_scrollbar_draw_up_left_arrow");
}

static void
gtk_scrollbar_draw_down_right_arrow (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  GtkArrowType arrow_type;
  GtkStateType state_type;
  GtkShadowType shadow_type;

  g_function_enter ("gtk_scrollbar_draw_down_right_arrow");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_draw_down_right_arrow");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;

      switch (scrollbar->orientation)
	{
	case HORIZONTAL:
	  arrow_type = GTK_ARROW_RIGHT;
	  break;
	case VERTICAL:
	  arrow_type = GTK_ARROW_DOWN;
	  break;
	default:
	  g_error ("unknown scrollbar orientation");
	  arrow_type = 0;
	  break;
	}

      if ((scrollbar->in_child == DOWN_RIGHT_ARROW) || (scrollbar->click_child == DOWN_RIGHT_ARROW))
	state_type = GTK_STATE_PRELIGHT;
      else
	state_type = GTK_STATE_NORMAL;

      if (scrollbar->click_child == DOWN_RIGHT_ARROW)
	shadow_type = GTK_SHADOW_IN;
      else
	shadow_type = GTK_SHADOW_OUT;

      gtk_draw_arrow (scrollbar->down_right_arrow,
		      widget->style->highlight_gc[state_type],
		      widget->style->shadow_gc[state_type],
		      widget->style->background_gc[state_type],
		      arrow_type,
		      shadow_type,
		      0, 0,
		      scrollbar->down_right_arrow->width,
		      scrollbar->down_right_arrow->height,
		      widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_scrollbar_draw_down_right_arrow");
}

static void
gtk_scrollbar_draw_slider (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  GtkStateType state_type;

  g_function_enter ("gtk_scrollbar_draw_slider");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_draw_slider");

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;

      if ((scrollbar->in_child == SLIDER) || (scrollbar->click_child == SLIDER))
	state_type = GTK_STATE_PRELIGHT;
      else
	state_type = GTK_STATE_NORMAL;

      gtk_draw_shadow (scrollbar->slider,
		       widget->style->highlight_gc[state_type],
		       widget->style->shadow_gc[state_type],
		       widget->style->background_gc[state_type],
		       GTK_SHADOW_OUT,
		       0, 0,
		       scrollbar->slider->width,
		       scrollbar->slider->height,
		       widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_scrollbar_draw_slider");
}

static gint
gtk_scrollbar_event (GtkWidget *widget,
		     GdkEvent  *event)
{
  GtkScrollBar *scrollbar;
  GdkModifierType mods;
  gint trough_part;
  gint x, y;

  g_function_enter ("gtk_scrollbar_event");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_event");

  if (!event)
    g_error ("passed NULL event to gtk_scrollbar_event");

  scrollbar = (GtkScrollBar*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (event->any.window == widget->window)
	gtk_scrollbar_draw_trough (widget);
      else if (event->any.window == scrollbar->up_left_arrow)
	gtk_scrollbar_draw_up_left_arrow (widget);
      else if (event->any.window == scrollbar->down_right_arrow)
	gtk_scrollbar_draw_down_right_arrow (widget);
      else if (event->any.window == scrollbar->slider)
	gtk_scrollbar_draw_slider (widget);
      break;

    case GDK_MOTION_NOTIFY:
      if (scrollbar->click_child == SLIDER)
	{
	  gdk_window_get_pointer (scrollbar->slider, &x, &y, &mods);
	  if (mods & GDK_BUTTON1_MASK)
	    {
	      switch (scrollbar->orientation)
		{
		case HORIZONTAL:
		  gtk_hscrollbar_motion (widget, x - scrollbar->click_point);
		  break;
		case VERTICAL:
		  gtk_vscrollbar_motion (widget, y - scrollbar->click_point);
		  break;
		default:
		  g_error ("unknown scrollbar orientation");
		  break;
		}
	    }
	}
      break;

    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
	{
	  gtk_grab_add (widget);

	  scrollbar->click_child = scrollbar->in_child;
	  if (scrollbar->click_child == UP_LEFT_ARROW)
	    {
	      gtk_scrollbar_draw_up_left_arrow (widget);

	      scrollbar->click_child = UP_LEFT_ARROW;
	      scrollbar->scroll_type = SCROLL_STEP_BACK;

	      gtk_scrollbar_timer (widget);
	      gtk_scrollbar_add_timer (widget);
	    }
	  else if (scrollbar->click_child == DOWN_RIGHT_ARROW)
	    {
	      gtk_scrollbar_draw_down_right_arrow (widget);

	      scrollbar->click_child = DOWN_RIGHT_ARROW;
	      scrollbar->scroll_type = SCROLL_STEP_FORWARD;

	      gtk_scrollbar_timer (widget);
	      gtk_scrollbar_add_timer (widget);
	    }
	  else if (scrollbar->click_child == SLIDER)
	    {
	      gtk_scrollbar_draw_slider (widget);
	      switch (scrollbar->orientation)
		{
		case HORIZONTAL:
		  scrollbar->click_point = event->button.x;
		  break;
		case VERTICAL:
		  scrollbar->click_point = event->button.y;
		  break;
		default:
		  g_error ("unknown scrollbar orientation");
		  break;
		}
	    }
	  else
	    {
	      trough_part = gtk_scrollbar_check_trough_click (widget,
							      event->button.x,
							      event->button.y);

	      if (trough_part != TROUGH_NONE)
		{
		  scrollbar->click_child = TROUGH;

		  if (trough_part == TROUGH_PAGE_BACK)
		    scrollbar->scroll_type = SCROLL_PAGE_BACK;
		  else if (trough_part == TROUGH_PAGE_FORWARD)
		    scrollbar->scroll_type = SCROLL_PAGE_FORWARD;

		  gtk_scrollbar_timer (widget);
		  gtk_scrollbar_add_timer (widget);
		}
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1)
	{
	  gtk_grab_remove (widget);

	  if (scrollbar->click_child == UP_LEFT_ARROW)
	    {
	      scrollbar->click_child = NONE;
	      gtk_scrollbar_draw_up_left_arrow (widget);
	      gtk_scrollbar_remove_timer (widget);
	    }
	  else if (scrollbar->click_child == DOWN_RIGHT_ARROW)
	    {
	      scrollbar->click_child = NONE;
	      gtk_scrollbar_draw_down_right_arrow (widget);
	      gtk_scrollbar_remove_timer (widget);
	    }
	  else if (scrollbar->click_child == SLIDER)
	    {
	      scrollbar->click_child = NONE;
	      gtk_scrollbar_draw_slider (widget);
	    }
	  else if (scrollbar->click_child == TROUGH)
	    {
	      scrollbar->click_child = NONE;
	      gtk_scrollbar_remove_timer (widget);
	    }
	}
      break;

    case GDK_ENTER_NOTIFY:
      if (event->any.window == scrollbar->up_left_arrow)
	{
	  scrollbar->in_child = UP_LEFT_ARROW;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_up_left_arrow (widget);
	}
      else if (event->any.window == scrollbar->down_right_arrow)
	{
	  scrollbar->in_child = DOWN_RIGHT_ARROW;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_down_right_arrow (widget);
	}
      else if (event->any.window == scrollbar->slider)
	{
	  scrollbar->in_child = SLIDER;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_slider (widget);
	}
      break;

    case GDK_LEAVE_NOTIFY:
      if (event->any.window == scrollbar->up_left_arrow)
	{
	  scrollbar->in_child = NONE;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_up_left_arrow (widget);
	}
      else if (event->any.window == scrollbar->down_right_arrow)
	{
	  scrollbar->in_child = NONE;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_down_right_arrow (widget);
	}
      else if (event->any.window == scrollbar->slider)
	{
	  scrollbar->in_child = NONE;
	  if ((scrollbar->click_child == NONE) || (scrollbar->click_child == TROUGH))
	    gtk_scrollbar_draw_slider (widget);
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_scrollbar_event");
  return FALSE;
}

static gint
gtk_scrollbar_is_child (GtkWidget *widget,
			GtkWidget *child)
{
  g_function_enter ("gtk_scrollbar_is_child");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_is_child");

  if (!child)
    g_error ("passed NULL child to gtk_scrollbar_is_child");

  g_function_leave ("gtk_scrollbar_is_child");
  return FALSE;
}

static gint
gtk_scrollbar_locate (GtkWidget  *widget,
		      GtkWidget **child,
		      gint        x,
		      gint        y)
{
  g_function_enter ("gtk_scrollbar_locate");
  g_warning ("gtk_scrollbar_locate: UNFINISHED");
  g_function_leave ("gtk_scrollbar_locate");
  return FALSE;
}

static gint
gtk_scrollbar_adjustment_update (GtkObserver *observer,
				 GtkData     *data)
{
  GtkScrollBar *scrollbar;
  GtkDataAdjustment *adjustment;
  gfloat new_value;
  gint value_changed;

  g_function_enter ("gtk_scrollbar_adjustment_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  scrollbar = (GtkScrollBar*) observer->user_data;
  g_assert (scrollbar != NULL);

  adjustment = (GtkDataAdjustment*) data;

  value_changed = FALSE;
  if (((scrollbar->old_lower != adjustment->lower) ||
       (scrollbar->old_upper != adjustment->upper) ||
       (scrollbar->old_page_size != adjustment->page_size)) &&
      (scrollbar->old_value == adjustment->value))
    {
      if (adjustment->lower == adjustment->upper)
	{
	  new_value = adjustment->lower;
	}
      else
	{
	  if (scrollbar->old_lower != (scrollbar->old_upper - scrollbar->old_page_size))
	    {
	      new_value = ((((adjustment->value - scrollbar->old_lower) *
			     (adjustment->upper - adjustment->lower - adjustment->page_size)) /
			    (scrollbar->old_upper - scrollbar->old_lower - scrollbar->old_page_size)) +
			   adjustment->lower);
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

  if ((scrollbar->old_value != adjustment->value) ||
      (scrollbar->old_lower != adjustment->lower) ||
      (scrollbar->old_upper != adjustment->upper))
    {
      switch (scrollbar->orientation)
	{
	case HORIZONTAL:
	  gtk_hscrollbar_calc_slider_size ((GtkWidget*) scrollbar);
	  gtk_hscrollbar_calc_slider_pos ((GtkWidget*) scrollbar);
	  break;

	case VERTICAL:
	  gtk_vscrollbar_calc_slider_size ((GtkWidget*) scrollbar);
	  gtk_vscrollbar_calc_slider_pos ((GtkWidget*) scrollbar);
	  break;

	default:
	  break;
	}

      scrollbar->old_value = adjustment->value;
      scrollbar->old_lower = adjustment->lower;
      scrollbar->old_upper = adjustment->upper;
      scrollbar->old_page_size = adjustment->page_size;
    }

  g_function_leave ("gtk_scrollbar_adjustment_update");
  return value_changed;
}

static void
gtk_scrollbar_adjustment_disconnect (GtkObserver *observer,
				     GtkData     *data)
{
  g_function_enter ("gtk_scrollbar_adjustment_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_scrollbar_adjustment_disconnect");
}

static void
gtk_hscrollbar_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  gint shadow_thickness;

  g_function_enter ("gtk_hscrollbar_size_request");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_size_request");

  if (!requisition)
    g_error ("passed NULL requisition to gtk_hscrollbar_size_request");

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = (ARROW_LENGTH * 2 +
			ARROW_SLIDER_SPACING * 2 +
			MIN_SLIDER_LENGTH +
			shadow_thickness * 2);
  requisition->height = SLIDER_WIDTH + shadow_thickness * 2;

  g_function_leave ("gtk_hscrollbar_size_request");
}

static void
gtk_hscrollbar_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  gint height;
  gint y;

  g_function_enter ("gtk_hscrollbar_size_allocate");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_size_allocate");

  if (!allocation)
    g_error ("passed NULL allocation to gtk_hscrollbar_size_allocate");

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      height = widget->requisition.height;
      y = allocation->y + (allocation->height - height) / 2;

      gdk_window_move (widget->window, allocation->x, y);
      gdk_window_set_size (widget->window, allocation->width, height);
    }

  gtk_hscrollbar_calc_child_sizes (widget);

  g_function_leave ("gtk_hscrollbar_size_allocate");
}

static void
gtk_hscrollbar_calc_child_sizes (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;

  g_function_enter ("gtk_hscrollbar_calc_child_sizes");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_calc_child_sizes");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      gdk_window_move (scrollbar->up_left_arrow,
		       shadow_thickness,
		       shadow_thickness);
      gdk_window_move (scrollbar->down_right_arrow,
		       widget->allocation.width - shadow_thickness - ARROW_LENGTH,
		       shadow_thickness);
      gdk_window_set_size (scrollbar->up_left_arrow,
			   ARROW_LENGTH,
			   ARROW_LENGTH);
      gdk_window_set_size (scrollbar->down_right_arrow,
			   ARROW_LENGTH,
			   ARROW_LENGTH);

      gtk_hscrollbar_calc_slider_size (widget);
      gtk_hscrollbar_calc_slider_pos (widget);
    }

  g_function_leave ("gtk_hscrollbar_calc_child_sizes");
}

static void
gtk_hscrollbar_calc_slider_size (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;
  gint left;
  gint right;
  gint width;

  g_function_enter ("gtk_hscrollbar_calc_slider_size");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_calc_slider_size");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      left = (scrollbar->up_left_arrow->x +
	      scrollbar->up_left_arrow->width +
	      ARROW_SLIDER_SPACING);
      right = (scrollbar->down_right_arrow->x -
	       ARROW_SLIDER_SPACING);
      width = right - left;

      if ((scrollbar->adjustment->page_size > 0) &&
	  (scrollbar->adjustment->lower != scrollbar->adjustment->upper))
	{
	  if (scrollbar->adjustment->page_size >
	      (scrollbar->adjustment->upper - scrollbar->adjustment->lower))
	    scrollbar->adjustment->page_size = (scrollbar->adjustment->upper -
						scrollbar->adjustment->lower);

	  width = ((width * scrollbar->adjustment->page_size) /
		   (scrollbar->adjustment->upper - scrollbar->adjustment->lower));

	  if (width < MIN_SLIDER_LENGTH)
	    width = MIN_SLIDER_LENGTH;
	}

      if ((scrollbar->slider->height != SLIDER_WIDTH) ||
	  (scrollbar->slider->width != width))
	{
	  scrollbar->slider_size = width;
	  gdk_window_set_size (scrollbar->slider,
			       width,
			       SLIDER_WIDTH);
	}
    }

  g_function_leave ("gtk_hscrollbar_calc_slider_size");
}

static void
gtk_hscrollbar_calc_slider_pos (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;
  gint left;
  gint right;
  gint x;

  g_function_enter ("gtk_hscrollbar_calc_slider_pos");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_calc_slider_pos");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      left = (scrollbar->up_left_arrow->x +
	      scrollbar->up_left_arrow->width +
	      ARROW_SLIDER_SPACING);
      right = (scrollbar->down_right_arrow->x -
	       ARROW_SLIDER_SPACING);
      x = left;

      if (scrollbar->slider_size > 0)
	{
	  if ((scrollbar->adjustment->value < (scrollbar->adjustment->lower - EPSILON)) ||
	      (scrollbar->adjustment->value >
	       (scrollbar->adjustment->upper - scrollbar->adjustment->page_size + EPSILON)))
	    g_error ("value not in range: %0.2f [%0.2f <--> (%0.2f - %0.2f)]",
		     scrollbar->adjustment->value,
		     scrollbar->adjustment->lower,
		     scrollbar->adjustment->upper,
		     scrollbar->adjustment->page_size);

	  if (scrollbar->adjustment->lower != scrollbar->adjustment->upper)
	    x += ((right - left) *
		  (scrollbar->adjustment->value - scrollbar->adjustment->lower) /
		  (scrollbar->adjustment->upper - scrollbar->adjustment->lower));
	}

      if ((scrollbar->slider->x != x) ||
	  (scrollbar->slider->y != shadow_thickness))
	gdk_window_move (scrollbar->slider,
			 x,
			 shadow_thickness);
    }

  g_function_leave ("gtk_hscrollbar_calc_slider_pos");
}

static void
gtk_hscrollbar_motion (GtkWidget *widget,
		       gint     delta)
{
  GtkScrollBar *scrollbar;
  gint left;
  gint right;
  gint new_pos;

  g_function_enter ("gtk_hscrollbar_motion");

  if (!widget)
    g_error ("passed NULL widget to gtk_hscrollbar_motion");

  scrollbar = (GtkScrollBar*) widget;

  new_pos = scrollbar->slider->x + delta;
  left = (scrollbar->up_left_arrow->x +
	  scrollbar->up_left_arrow->width +
	  ARROW_SLIDER_SPACING);
  right = (scrollbar->down_right_arrow->x -
	   scrollbar->slider_size -
	   ARROW_SLIDER_SPACING);

  if (new_pos < left)
    new_pos = left;
  else if (new_pos > right)
    new_pos = right;

  if (scrollbar->slider->x != new_pos)
    {
      scrollbar->adjustment->value =
	((scrollbar->adjustment->upper - scrollbar->adjustment->lower -
	  scrollbar->adjustment->page_size) *
	 (new_pos - left) / (right - left) + scrollbar->adjustment->lower);
      gtk_data_notify ((GtkData*) scrollbar->adjustment);

      gdk_window_move (scrollbar->slider,
		       new_pos,
		       scrollbar->slider->y);
    }

  g_function_leave ("gtk_hscrollbar_motion");
}


static void
gtk_vscrollbar_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  gint shadow_thickness;

  g_function_enter ("gtk_vscrollbar_size_request");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_size_request");

  if (!requisition)
    g_error ("passed NULL requisition to gtk_vscrollbar_size_request");

  shadow_thickness = widget->style->shadow_thickness;
  requisition->width = SLIDER_WIDTH + shadow_thickness * 2;
  requisition->height = (ARROW_LENGTH * 2 +
			 ARROW_SLIDER_SPACING * 2 +
			 MIN_SLIDER_LENGTH +
			 shadow_thickness * 2);

  g_function_leave ("gtk_vscrollbar_size_request");
}

static void
gtk_vscrollbar_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  gint width;
  gint x;

  g_function_enter ("gtk_vscrollbar_size_allocate");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_size_allocate");

  if (!allocation)
    g_error ("passed NULL allocation to gtk_vscrollbar_size_allocate");

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      width = widget->requisition.width;
      x = allocation->x + (allocation->width - width) / 2;

      gdk_window_move (widget->window, x, allocation->y);
      gdk_window_set_size (widget->window, width, allocation->height);
    }

  gtk_vscrollbar_calc_child_sizes (widget);

  g_function_leave ("gtk_vscrollbar_size_allocate");
}

static void
gtk_vscrollbar_calc_child_sizes (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;

  g_function_enter ("gtk_vscrollbar_calc_child_sizes");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_calc_child_sizes");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      gdk_window_move (scrollbar->up_left_arrow,
		       shadow_thickness,
		       shadow_thickness);
      gdk_window_move (scrollbar->down_right_arrow,
		       shadow_thickness,
		       widget->allocation.height - shadow_thickness - ARROW_LENGTH);
      gdk_window_set_size (scrollbar->up_left_arrow,
			   ARROW_LENGTH,
			   ARROW_LENGTH);
      gdk_window_set_size (scrollbar->down_right_arrow,
			   ARROW_LENGTH,
			   ARROW_LENGTH);

      gtk_vscrollbar_calc_slider_size (widget);
      gtk_vscrollbar_calc_slider_pos (widget);
    }

  g_function_leave ("gtk_vscrollbar_calc_child_sizes");
}

static void
gtk_vscrollbar_calc_slider_size (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;
  gint top;
  gint bottom;
  gint height;

  g_function_enter ("gtk_vscrollbar_calc_slider_size");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_calc_slider_size");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      top = (scrollbar->up_left_arrow->y +
	     scrollbar->up_left_arrow->height +
	     ARROW_SLIDER_SPACING);
      bottom = (scrollbar->down_right_arrow->y -
		ARROW_SLIDER_SPACING);
      height = bottom - top;

      if ((scrollbar->adjustment->page_size > 0) &&
	  (scrollbar->adjustment->lower != scrollbar->adjustment->upper))
	{
	  if (scrollbar->adjustment->page_size >
	      (scrollbar->adjustment->upper - scrollbar->adjustment->lower))
	    g_error ("slider too large: %0.2f (%0.2f - %0.2f)",
		      scrollbar->adjustment->page_size,
		      scrollbar->adjustment->lower,
		      scrollbar->adjustment->upper);

	  height = ((height * scrollbar->adjustment->page_size) /
		    (scrollbar->adjustment->upper - scrollbar->adjustment->lower));

	  if (height < MIN_SLIDER_LENGTH)
	    height = MIN_SLIDER_LENGTH;
	}

      if ((scrollbar->slider->width != SLIDER_WIDTH) ||
	  (scrollbar->slider->height != height))
	{
	  scrollbar->slider_size = height;
	  gdk_window_set_size (scrollbar->slider,
			       SLIDER_WIDTH,
			       height);
	}
    }

  g_function_leave ("gtk_vscrollbar_calc_slider_size");
}

static void
gtk_vscrollbar_calc_slider_pos (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;
  gint top;
  gint bottom;
  gint y;

  g_function_enter ("gtk_vscrollbar_calc_slider_pos");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_calc_slider_pos");

  if (GTK_WIDGET_REALIZED (widget))
    {
      scrollbar = (GtkScrollBar*) widget;
      shadow_thickness = widget->style->shadow_thickness;

      top = (scrollbar->up_left_arrow->y +
	     scrollbar->up_left_arrow->height +
	     ARROW_SLIDER_SPACING);
      bottom = (scrollbar->down_right_arrow->y -
		ARROW_SLIDER_SPACING);
      y = top;

      if (scrollbar->slider_size > 0)
	{
	  if ((scrollbar->adjustment->value < (scrollbar->adjustment->lower - EPSILON)) ||
	      (scrollbar->adjustment->value >
	       (scrollbar->adjustment->upper - scrollbar->adjustment->page_size + EPSILON)))
	    g_error ("value not in range: %0.2f [%0.2f <--> (%0.2f - %0.2f)]",
		     scrollbar->adjustment->value,
		     scrollbar->adjustment->lower,
		     scrollbar->adjustment->upper,
		     scrollbar->adjustment->page_size);

	  if (scrollbar->adjustment->lower != scrollbar->adjustment->upper)
	    y += ((bottom - top) *
		  (scrollbar->adjustment->value - scrollbar->adjustment->lower) /
		  (scrollbar->adjustment->upper - scrollbar->adjustment->lower));
	}

      if ((scrollbar->slider->x != shadow_thickness) ||
	  (scrollbar->slider->y != y))
	gdk_window_move (scrollbar->slider,
			 shadow_thickness,
			 y);
    }

  g_function_leave ("gtk_vscrollbar_calc_slider_pos");
}

static void
gtk_vscrollbar_motion (GtkWidget *widget,
		       gint     delta)
{
  GtkScrollBar *scrollbar;
  gint top;
  gint bottom;
  gint new_pos;

  g_function_enter ("gtk_vscrollbar_motion");

  if (!widget)
    g_error ("passed NULL widget to gtk_vscrollbar_motion");

  scrollbar = (GtkScrollBar*) widget;

  new_pos = scrollbar->slider->y + delta;
  top = (scrollbar->up_left_arrow->y +
	 scrollbar->up_left_arrow->height +
	 ARROW_SLIDER_SPACING);
  bottom = (scrollbar->down_right_arrow->y -
	    scrollbar->slider_size -
	    ARROW_SLIDER_SPACING);

  if (new_pos < top)
    new_pos = top;
  else if (new_pos > bottom)
    new_pos = bottom;

  if (scrollbar->slider->y != new_pos)
    {
      scrollbar->adjustment->value =
	((scrollbar->adjustment->upper - scrollbar->adjustment->lower -
	  scrollbar->adjustment->page_size) *
	 (new_pos - top) / (bottom - top) + scrollbar->adjustment->lower);
      gtk_data_notify ((GtkData*) scrollbar->adjustment);

      gdk_window_move (scrollbar->slider,
		       scrollbar->slider->x,
		       new_pos);
    }

  g_function_leave ("gtk_vscrollbar_motion");
}

static gint
gtk_scrollbar_check_trough_click (GtkWidget *widget,
				  gint     x,
				  gint     y)
{
  GtkScrollBar *scrollbar;
  gint shadow_thickness;
  gint return_val;

  g_function_enter ("gtk_scrollbar_check_trough_click");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_check_trough_click");

  scrollbar = (GtkScrollBar*) widget;
  return_val = TROUGH_NONE;

  shadow_thickness = widget->style->shadow_thickness;
  if ((x > shadow_thickness) &&
      (y > shadow_thickness) &&
      (x < (widget->window->width - shadow_thickness)) &&
      (y < (widget->window->height - shadow_thickness)))
    {
      switch (scrollbar->orientation)
	{
	case HORIZONTAL:
	  if (x < scrollbar->slider->x)
	    return_val = TROUGH_PAGE_BACK;
	  else
	    return_val = TROUGH_PAGE_FORWARD;
	  break;
	case VERTICAL:
	  if (y < scrollbar->slider->y)
	    return_val = TROUGH_PAGE_BACK;
	  else
	    return_val = TROUGH_PAGE_FORWARD;
	  break;
	default:
	  g_error ("unknown scrollbar orientation");
	  break;
	}
    }

  g_function_leave ("gtk_scrollbar_check_trough_click");
  return return_val;
}

static void
gtk_scrollbar_add_timer (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_scrollbar_add_timer");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_add_timer");

  scrollbar = (GtkScrollBar*) widget;

  if (scrollbar->timer)
    g_error ("already a scrollbar timer in progress");

  scrollbar->timer = gtk_timeout_add (SCROLL_TIMER_LENGTH,
				      gtk_scrollbar_timer,
				      (gpointer) scrollbar);

  g_function_leave ("gtk_scrollbar_add_timer");
}

static void
gtk_scrollbar_remove_timer (GtkWidget *widget)
{
  GtkScrollBar *scrollbar;

  g_function_enter ("gtk_scrollbar_remove_timer");

  if (!widget)
    g_error ("passed NULL widget to gtk_scrollbar_add_timer");

  scrollbar = (GtkScrollBar*) widget;
  if (scrollbar->timer)
    {
      gtk_timeout_remove (scrollbar->timer);
      scrollbar->timer = 0;
    }

  g_function_leave ("gtk_scrollbar_remove_timer");
}

static gint
gtk_scrollbar_timer (gpointer data)
{
  GtkScrollBar *scrollbar;
  gfloat new_value;
  gint return_val;

  g_function_enter ("gtk_scrollbar_timer");

  if (!data)
    g_error ("passed NULL data to gtk_scrollbar_timer");

  scrollbar = (GtkScrollBar*) data;
  new_value = scrollbar->adjustment->value;
  return_val = TRUE;

  switch (scrollbar->scroll_type)
    {
    case SCROLL_NONE:
      break;

    case SCROLL_STEP_BACK:
      new_value -= scrollbar->adjustment->step_increment;
      if (new_value < scrollbar->adjustment->lower)
	{
	  new_value = scrollbar->adjustment->lower;
	  return_val = FALSE;
	  scrollbar->timer = 0;
	}
      break;

    case SCROLL_STEP_FORWARD:
      new_value += scrollbar->adjustment->step_increment;
      if (new_value > (scrollbar->adjustment->upper - scrollbar->adjustment->page_size))
	{
	  new_value = scrollbar->adjustment->upper - scrollbar->adjustment->page_size;
	  return_val = FALSE;
	  scrollbar->timer = 0;
	}
      break;

    case SCROLL_PAGE_BACK:
      new_value -= scrollbar->adjustment->page_increment;
      if (new_value < scrollbar->adjustment->lower)
	{
	  new_value = scrollbar->adjustment->lower;
	  return_val = FALSE;
	  scrollbar->timer = 0;
	}
      break;

    case SCROLL_PAGE_FORWARD:
      new_value += scrollbar->adjustment->page_increment;
      if (new_value > (scrollbar->adjustment->upper - scrollbar->adjustment->page_size))
	{
	  new_value = scrollbar->adjustment->upper - scrollbar->adjustment->page_size;
	  return_val = FALSE;
	  scrollbar->timer = 0;
	}
      break;

    default:
      g_error ("unknown scroll type");
      break;
    }

  if (new_value != scrollbar->adjustment->value)
    {
      scrollbar->adjustment->value = new_value;
      gtk_data_notify ((GtkData*) scrollbar->adjustment);

      switch (scrollbar->orientation)
	{
	case HORIZONTAL:
	  gtk_hscrollbar_calc_slider_pos ((GtkWidget*) scrollbar);
	  break;
	case VERTICAL:
	  gtk_vscrollbar_calc_slider_pos ((GtkWidget*) scrollbar);
	  break;
	default:
	  g_error ("unknown scrollbar orientation");
	  break;
	}
    }

  g_function_leave ("gtk_scrollbar_timer");
  return return_val;
}
