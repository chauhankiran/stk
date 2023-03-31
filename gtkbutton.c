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
#include "gtkbutton.h"
#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define TOGGLE_INDICATOR_SIZE     10
#define TOGGLE_INDICATOR_SPACING  2


typedef struct _GtkButton        GtkButton;
typedef struct _GtkToggleButton  GtkToggleButton;

struct _GtkButton
{
  GtkContainer container;

  GtkWidget *child;

  gint8 previous_state;
  gint8 shadow_type;

  unsigned int in_button   : 1;
  unsigned int button_down : 1;
  unsigned int draw_toggle_indicator : 1;

  GtkDataInt *state;
  GtkObserver state_observer;
};

struct _GtkToggleButton
{
  GtkButton button;

  GtkDataWidget *owner;
  GtkObserver owner_observer;
};


static void  gtk_button_destroy             (GtkWidget       *widget);
static void  gtk_button_map                 (GtkWidget       *widget);
static void  gtk_button_unmap               (GtkWidget       *widget);
static void  gtk_button_realize             (GtkWidget       *widget);
static void  gtk_button_draw_focus          (GtkWidget       *widget);
static gint  gtk_button_event               (GtkWidget       *widget,
					     GdkEvent        *event);
static void  gtk_button_size_request        (GtkWidget       *widget,
					     GtkRequisition  *requisition);
static void  gtk_button_size_allocate       (GtkWidget       *widget,
					     GtkAllocation   *allocation);
static gint  gtk_button_is_child            (GtkWidget       *widget,
					     GtkWidget       *child);
static gint  gtk_button_locate              (GtkWidget       *widget,
					     GtkWidget      **child,
					     gint             x,
					     gint             y);
static void  gtk_button_set_state           (GtkWidget       *widget,
					     GtkStateType     state);
static gint  gtk_button_install_accelerator (GtkWidget       *widget,
					     gchar            accelerator_key,
					     guint8           accelerator_mods);
static void  gtk_button_add                 (GtkContainer    *container,
					     GtkWidget       *widget);
static void  gtk_button_remove              (GtkContainer    *container,
					     GtkWidget       *widget);
static void  gtk_button_foreach             (GtkContainer   *container,
					     GtkCallback     callback,
					     gpointer        callback_data);

static void  gtk_push_button_activate   (GtkWidget    *widget);
static void  gtk_push_button_draw       (GtkWidget    *widget,
					 GdkRectangle *area,
					 gint          is_expose);
static void  gtk_push_button_expose     (GtkWidget    *widget);

static void  gtk_toggle_button_activate (GtkWidget    *widget);
static void  gtk_toggle_button_draw     (GtkWidget    *widget,
					 GdkRectangle *area,
					 gint          is_expose);
static void  gtk_toggle_button_expose   (GtkWidget    *widget);

static gint  gtk_button_state_update     (GtkObserver *observer,
					  GtkData     *data);
static void  gtk_button_state_disconnect (GtkObserver *observer,
					  GtkData     *data);

static gint  gtk_button_owner_update     (GtkObserver *observer,
					  GtkData     *data);
static void  gtk_button_owner_disconnect (GtkObserver *observer,
					  GtkData     *data);


static GtkWidgetFunctions push_button_widget_functions =
{
  gtk_button_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_button_map,
  gtk_button_unmap,
  gtk_button_realize,
  gtk_push_button_draw,
  gtk_button_draw_focus,
  gtk_button_event,
  gtk_button_size_request,
  gtk_button_size_allocate,
  gtk_button_is_child,
  gtk_button_locate,
  gtk_push_button_activate,
  gtk_button_set_state,
  gtk_button_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions toggle_button_widget_functions =
{
  gtk_button_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_button_map,
  gtk_button_unmap,
  gtk_button_realize,
  gtk_toggle_button_draw,
  gtk_button_draw_focus,
  gtk_button_event,
  gtk_button_size_request,
  gtk_button_size_allocate,
  gtk_button_is_child,
  gtk_button_locate,
  gtk_toggle_button_activate,
  gtk_button_set_state,
  gtk_button_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions button_container_functions =
{
  gtk_button_add,
  gtk_button_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_button_foreach,
};


GtkWidget*
gtk_push_button_new ()
{
  GtkButton *button;

  g_function_enter ("gtk_push_button_new");

  button = g_new (GtkButton, 1);

  button->container.widget.type = gtk_get_push_button_type ();
  button->container.widget.function_table = &push_button_widget_functions;
  button->container.function_table = &button_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) button);
  gtk_container_set_defaults ((GtkWidget*) button);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_FOCUS);

  button->child = NULL;
  button->in_button = FALSE;
  button->button_down = FALSE;
  button->previous_state = GTK_STATE_NORMAL;
  button->shadow_type = GTK_SHADOW_OUT;
  button->draw_toggle_indicator = FALSE;

  button->state = (GtkDataInt*) gtk_data_int_new (GTK_STATE_NORMAL);

  button->state_observer.update = gtk_button_state_update;
  button->state_observer.disconnect = gtk_button_state_disconnect;
  button->state_observer.user_data = button;

  gtk_data_attach ((GtkData*) button->state, &button->state_observer);

  g_function_leave ("gtk_push_button_new");
  return ((GtkWidget*) button);
}

GtkWidget*
gtk_toggle_button_new (GtkData *owner)
{
  GtkToggleButton *button;

  g_function_enter ("gtk_toggle_button_new");

  button = g_new (GtkToggleButton, 1);

  button->button.container.widget.type = gtk_get_toggle_button_type ();
  button->button.container.widget.function_table = &toggle_button_widget_functions;
  button->button.container.function_table = &button_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) button);
  gtk_container_set_defaults ((GtkWidget*) button);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_FOCUS);

  button->button.child = NULL;
  button->button.in_button = FALSE;
  button->button.button_down = FALSE;
  button->button.previous_state = GTK_STATE_NORMAL;
  button->button.shadow_type = GTK_SHADOW_OUT;
  button->button.draw_toggle_indicator = FALSE;

  button->button.state = (GtkDataInt*) gtk_data_int_new (GTK_STATE_NORMAL);

  button->button.state_observer.update = gtk_button_state_update;
  button->button.state_observer.disconnect = gtk_button_state_disconnect;
  button->button.state_observer.user_data = button;

  gtk_data_attach ((GtkData*) button->button.state, &button->button.state_observer);

  if (!owner)
    {
      owner = gtk_data_widget_new ((GtkWidget*) button);

      button->button.previous_state = GTK_STATE_ACTIVE;
      button->button.state->value = GTK_STATE_ACTIVE;

      button->button.shadow_type = GTK_SHADOW_IN;
    }

  button->owner = (GtkDataWidget*) owner;
  button->owner_observer.update = gtk_button_owner_update;
  button->owner_observer.disconnect = gtk_button_owner_disconnect;
  button->owner_observer.user_data = button;

  gtk_data_attach ((GtkData*) button->owner, &button->owner_observer);

  g_function_leave ("gtk_toggle_button_new");
  return ((GtkWidget*) button);
}

GtkWidget*
gtk_radio_button_new (GtkData *owner)
{
  GtkButton *toggle;

  g_function_enter ("gtk_radio_button_new");

  toggle = (GtkButton*) gtk_toggle_button_new (owner);
  toggle->draw_toggle_indicator = TRUE;

  g_function_leave ("gtk_radio_button_new");
  return ((GtkWidget*) toggle);
}

GtkWidget*
gtk_check_button_new ()
{
  GtkButton *toggle;

  g_function_enter ("gtk_check_button_new");

  toggle = (GtkButton*) gtk_toggle_button_new (NULL);
  toggle->draw_toggle_indicator = TRUE;

  g_function_leave ("gtk_check_button_new");
  return ((GtkWidget*) toggle);
}

void
gtk_button_reset (GtkWidget *button)
{
  GtkButton *rbutton;
  GdkWindow *window;
  gint x, y;

  g_function_enter ("gtk_button_reset");

  g_assert (button != NULL);

  rbutton = (GtkButton*) button;
  rbutton->button_down = FALSE;
  rbutton->previous_state = GTK_STATE_NORMAL;
  rbutton->shadow_type = GTK_SHADOW_OUT;

  window = gdk_window_get_pointer (button->window, &x, &y, NULL);
  if (window == button->window)
    {
      rbutton->in_button = TRUE;

      if (rbutton->state->value != GTK_STATE_PRELIGHT)
	{
	  rbutton->state->value = GTK_STATE_PRELIGHT;
	  gtk_data_notify ((GtkData*) rbutton->state);
	}
    }
  else
    {
      rbutton->in_button = FALSE;

      if (rbutton->state->value != GTK_STATE_NORMAL)
	{
	  rbutton->state->value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) rbutton->state);
	}
    }

  g_function_leave ("gtk_button_reset");
}

GtkData*
gtk_button_get_state (GtkWidget *button)
{
  GtkButton *rbutton;
  GtkData *data;

  g_function_enter ("gtk_button_get_state");

  g_assert (button != NULL);

  rbutton = (GtkButton*) button;
  data = (GtkData*) rbutton->state;

  g_function_leave ("gtk_button_get_state");
  return data;
}

GtkData*
gtk_button_get_owner (GtkWidget *button)
{
  GtkToggleButton *rbutton;
  GtkData *data;

  g_function_enter ("gtk_button_get_owner");

  g_assert (button != NULL);

  rbutton = (GtkToggleButton*) button;
  data = (GtkData*) rbutton->owner;

  g_function_leave ("gtk_button_get_owner");
  return data;
}

guint16
gtk_get_push_button_type ()
{
  static guint16 push_button_type = 0;

  g_function_enter ("gtk_get_push_button_type");

  if (!push_button_type)
    gtk_widget_unique_type (&push_button_type);

  g_function_leave ("gtk_get_push_button_type");
  return push_button_type;
}

guint16
gtk_get_toggle_button_type ()
{
  static guint16 toggle_button_type = 0;

  g_function_enter ("gtk_get_toggle_button_type");

  if (!toggle_button_type)
    gtk_widget_unique_type (&toggle_button_type);

  g_function_leave ("gtk_get_toggle_button_type");
  return toggle_button_type;
}


static void
gtk_button_destroy (GtkWidget *widget)
{
  GtkButton *button;

  g_function_enter ("gtk_button_destroy");

  g_assert (widget != NULL);

  button = (GtkButton*) widget;
  if (button->child)
    if (!gtk_widget_destroy (button->child))
      button->child->parent = NULL;

  gtk_data_detach ((GtkData*) button->state, &button->state_observer);
  gtk_data_destroy ((GtkData*) button->state);

  if (button->container.widget.window)
    gdk_window_destroy (button->container.widget.window);
  g_free (button);

  g_function_leave ("gtk_button_destroy");
}

static void
gtk_button_map (GtkWidget *widget)
{
  GtkButton *button;

  g_function_enter ("gtk_button_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);

  button = (GtkButton*) widget;

  if (button->child && GTK_WIDGET_VISIBLE (button->child) && !GTK_WIDGET_MAPPED (button->child))
    gtk_widget_map (button->child);

  g_function_leave ("gtk_button_map");
}

static void
gtk_button_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_button_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_button_unmap");
}

static void
gtk_button_realize (GtkWidget *widget)
{
  GtkButton *button;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_function_enter ("gtk_button_realize");

  g_assert (widget != NULL);

  button = (GtkButton*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_peek_visual ();
  attributes.colormap = gtk_peek_colormap ();
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  button->container.widget.window = gdk_window_new (widget->parent->widget.window,
						    &attributes, attributes_mask);

  gdk_window_set_user_data (button->container.widget.window, button);

  button->container.widget.style = gtk_style_attach (button->container.widget.style,
						     button->container.widget.window);
  gdk_window_set_background (button->container.widget.window,
			     &button->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_button_realize");
}

static void
gtk_button_draw_focus (GtkWidget *widget)
{
  g_function_enter ("gtk_button_draw_focus");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      if (GTK_WIDGET_HAS_FOCUS (widget))
	gdk_draw_rectangle (widget->window,
			    widget->style->foreground_gc[GTK_STATE_NORMAL],
			    FALSE, 0, 0, widget->window->width - 1, widget->window->height - 1);
      else
	gdk_draw_rectangle (widget->window,
			    widget->style->background_gc[GTK_STATE_NORMAL],
			    FALSE, 0, 0, widget->window->width - 1, widget->window->height - 1);
    }

  g_function_leave ("gtk_button_draw_focus");
}

static gint
gtk_button_event (GtkWidget *widget,
		  GdkEvent  *event)
{
  GtkButton *button;
  GtkWidget *event_widget;
  gint old_state;

  g_function_enter ("gtk_button_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  button = (GtkButton*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_BUTTON_PRESS:
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);

      if (event->button.button == 1)
	{
	  gtk_grab_add (widget);
	  button->button_down = TRUE;

	  if ((button->in_button && (button->state->value != GTK_STATE_ACTIVE)) ||
	      (!button->in_button && (button->state->value != GTK_STATE_NORMAL)))
	    {
	      if (button->previous_state == GTK_STATE_NORMAL)
		button->state->value = ((button->in_button) ?
					       (GTK_STATE_ACTIVE) :
					       (GTK_STATE_NORMAL));
	      else
		button->state->value = ((button->in_button) ?
					       (GTK_STATE_NORMAL) :
					       (GTK_STATE_ACTIVE));

	      gtk_data_notify ((GtkData*) button->state);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      if ((event->button.button == 1) && button->button_down)
	{
	  gtk_grab_remove (widget);
	  button->button_down = FALSE;

	  if (button->in_button)
	    gtk_widget_activate ((GtkWidget*) button);

	  if ((button->in_button && (button->state->value != GTK_STATE_PRELIGHT)) ||
	      (!button->in_button && (button->state->value != GTK_STATE_NORMAL)))
	    {
	      old_state = button->state->value;
	      button->state->value = ((button->in_button) ?
					     (GTK_STATE_PRELIGHT) :
					     (button->previous_state));

	      if (old_state != button->state->value)
		gtk_data_notify ((GtkData*) button->state);
	    }
	}
      break;

    case GDK_ENTER_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (event_widget == widget)
	{
	  if (event->crossing.detail != GDK_NOTIFY_INFERIOR)
	    {
	      old_state = button->state->value;
	      if (button->previous_state == GTK_STATE_NORMAL)
		button->state->value = ((button->button_down) ?
					       (GTK_STATE_ACTIVE) :
					       (GTK_STATE_PRELIGHT));
	      else
		button->state->value = ((button->button_down) ?
					       (GTK_STATE_NORMAL) :
					       (GTK_STATE_PRELIGHT));

	      if (old_state != button->state->value)
		gtk_data_notify ((GtkData*) button->state);

	      button->in_button = TRUE;
	    }
	}
      break;

    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (event_widget == widget)
	{
	  if (event->crossing.detail != GDK_NOTIFY_INFERIOR)
	    {
	      if (button->state->value != button->previous_state)
		{
		  button->state->value = button->previous_state;
		  gtk_data_notify ((GtkData*) button->state);
		}
	      button->in_button = FALSE;
	    }
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_button_event");
  return FALSE;
}


static void
gtk_button_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkButton *button;
  gint shadow_thickness;
  gint temp;

  g_function_enter ("gtk_button_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  button = (GtkButton*) widget;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      shadow_thickness = widget->style->shadow_thickness;

      if (button->child)
	{
	  button->child->requisition.width = 0;
	  button->child->requisition.height = 0;

	  gtk_widget_size_request (button->child, &button->child->requisition);

	  requisition->width = (button->child->requisition.width +
				button->container.border_width * 2);
	  requisition->height = (button->child->requisition.height +
				 button->container.border_width * 2);

	  if (button->draw_toggle_indicator)
	    {
	      requisition->width += TOGGLE_INDICATOR_SIZE + 3 * TOGGLE_INDICATOR_SPACING;
	      temp = (TOGGLE_INDICATOR_SIZE + 2 * TOGGLE_INDICATOR_SPACING +
		      button->container.border_width * 2 + shadow_thickness * 2);
	      requisition->height = MAX (requisition->height, temp);
	    }
	  else
	    {
	      requisition->width += shadow_thickness * 2;
	      requisition->height += shadow_thickness * 2;
	    }

	  requisition->width += 2;
	  requisition->height += 2;
	}
      else
	{
	  requisition->width = button->container.border_width * 2 + shadow_thickness * 2 + 2;
	  requisition->height = button->container.border_width * 2 + shadow_thickness * 2 + 2;
	}
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  g_function_leave ("gtk_button_size_request");
}

static void
gtk_button_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkButton *button;
  GtkAllocation child_allocation;
  gint shadow_thickness;

  g_function_enter ("gtk_button_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  button = (GtkButton*) widget;

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

  if (button->child)
    {
      if (button->draw_toggle_indicator)
	{
	  child_allocation.x = (button->container.border_width +
				TOGGLE_INDICATOR_SIZE +
				TOGGLE_INDICATOR_SPACING * 3);
	  child_allocation.y = button->container.border_width;
	  child_allocation.width = allocation->width - child_allocation.x - button->container.border_width;
	  child_allocation.height = allocation->height - child_allocation.y * 2;
	}
      else
	{
	  shadow_thickness = widget->style->shadow_thickness;
	  child_allocation.x = button->container.border_width + shadow_thickness;
	  child_allocation.y = button->container.border_width + shadow_thickness;
	  child_allocation.width = allocation->width - child_allocation.x * 2;
	  child_allocation.height = allocation->height - child_allocation.y * 2;
	}

      if (child_allocation.width <= 0)
        child_allocation.width = 1;
      if (child_allocation.height <= 0)
        child_allocation.height = 1;

      gtk_widget_size_allocate (button->child, &child_allocation);
    }

  g_function_leave ("gtk_button_size_allocate");
}

static gint
gtk_button_is_child (GtkWidget *widget,
		     GtkWidget *child)
{
  GtkButton *button;
  gint return_val;

  g_function_enter ("gtk_button_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  button = (GtkButton*) widget;

  return_val = FALSE;
  if (button->child == child)
    return_val = TRUE;
  else if (button->child)
    return_val = gtk_widget_is_child (button->child, child);

  g_function_leave ("gtk_button_is_child");
  return return_val;
}

static gint
gtk_button_locate (GtkWidget  *widget,
		   GtkWidget **child,
		   gint      x,
		   gint      y)
{
  GtkButton *button;
  gint return_val;
  gint child_x;
  gint child_y;

  g_function_enter ("gtk_button_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < widget->allocation.width) &&
      (y < widget->allocation.height))
    {
      return_val = TRUE;

      button = (GtkButton*) widget;
      if (button->child)
	{
	  child_x = x - button->child->allocation.x;
	  child_y = y - button->child->allocation.y;

	  gtk_widget_locate (button->child, child, child_x, child_y);
	}

      if (!(*child))
	*child = widget;
    }

  g_function_leave ("gtk_button_locate");
  return return_val;
}

static void
gtk_button_set_state (GtkWidget    *widget,
		      GtkStateType  state)
{
  GtkButton *button;
  GtkStateType child_state;

  g_function_enter ("gtk_button_set_state");

  g_assert (widget != NULL);
  button = (GtkButton*) widget;

  if (button->child)
    {
      child_state = state;
      if (button->draw_toggle_indicator)
	if ((child_state != GTK_STATE_NORMAL) &&
	    (child_state != GTK_STATE_PRELIGHT))
	  child_state = GTK_STATE_NORMAL;
      gtk_widget_set_state (button->child, child_state);
    }

  if (button->state->value != state)
    {
      button->state->value = state;
      gtk_data_notify ((GtkData*) button->state);
    }

  g_function_leave ("gtk_button_set_state");
}

static gint
gtk_button_install_accelerator (GtkWidget *widget,
				gchar      accelerator_key,
				guint8     accelerator_mods)
{
  g_function_enter ("gtk_button_install_accelerator");
  g_assert (widget != NULL);
  g_function_leave ("gtk_button_install_accelerator");
  return TRUE;
}

static void
gtk_button_add (GtkContainer *container,
		GtkWidget    *widget)
{
  GtkButton *button;

  g_function_enter ("gtk_button_add");

  g_assert (container != NULL);

  button = (GtkButton*) container;

  if (button->child)
    g_error ("button already has a child");
  else
    button->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_button_add");
}

static void
gtk_button_remove (GtkContainer *container,
		   GtkWidget    *widget)
{
  GtkButton *button;

  g_function_enter ("gtk_button_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  button = (GtkButton*) container;

  g_assert (button->child == widget);
  g_assert (button->child != NULL);

  button->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_button_remove");
}

static void
gtk_button_foreach (GtkContainer *container,
		    GtkCallback   callback,
		    gpointer      callback_data)
{
  GtkButton *button;

  g_function_enter ("gtk_button_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  button = (GtkButton*) container;

  if (button->child)
    (* callback) (button->child, callback_data, NULL);

  g_function_leave ("gtk_button_foreach");
}


static void
gtk_push_button_activate (GtkWidget *widget)
{
  GtkButton *rbutton;

  g_function_enter ("gtk_push_button_activate");

  g_assert (widget != NULL);
  rbutton = (GtkButton*) widget;

  rbutton->state->value = GTK_STATE_ACTIVATED;
  gtk_data_notify ((GtkData*) rbutton->state);

  rbutton->state->value = ((rbutton->in_button) ?
			   (GTK_STATE_PRELIGHT) :
			   (GTK_STATE_NORMAL));
  gtk_data_notify ((GtkData*) rbutton->state);

  g_function_leave ("gtk_push_button_activate");
}

static void
gtk_push_button_draw (GtkWidget    *widget,
		      GdkRectangle *area,
		      gint          is_expose)
{
  GtkButton *button;

  g_function_enter ("gtk_push_button_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      button = (GtkButton*) widget;
      gtk_push_button_expose (widget);

      if (button->child)
	gtk_widget_set_state (button->child, button->state->value);

      if (button->child)
	if (!is_expose || GTK_WIDGET_NO_WINDOW (button->child))
	  gtk_widget_draw (button->child, NULL, is_expose);
    }

  g_function_leave ("gtk_push_button_draw");
}

static void
gtk_push_button_expose (GtkWidget *widget)
{
  GtkButton *button;
  gint x, y;
  gint width, height;

  g_function_enter ("gtk_push_button_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      button = (GtkButton*) widget;

      gdk_window_set_background (widget->window,
				 &button->container.widget.style->background[button->state->value]);
      gdk_window_clear (widget->window);

      x = button->container.border_width;
      y = button->container.border_width;
      width = widget->allocation.width - 2 * x;
      height = widget->allocation.height - 2 * y;

      gtk_draw_shadow (widget->window,
		       button->container.widget.style->highlight_gc[button->state->value],
		       button->container.widget.style->shadow_gc[button->state->value],
		       NULL,
		       button->shadow_type,
		       x + 1, y + 1, width - 2, height - 2,
		       button->container.widget.style->shadow_thickness);
    }

  g_function_leave ("gtk_push_button_expose");
}

static void
gtk_toggle_button_activate (GtkWidget *widget)
{
  GtkToggleButton *button;

  g_function_enter ("gtk_toggle_button_activate");

  g_assert (widget != NULL);
  button = (GtkToggleButton*) widget;

  if (button->owner->data.observers->next)
    {
      if (button->button.previous_state == GTK_STATE_NORMAL)
	{
	  button->button.previous_state = GTK_STATE_ACTIVE;
	  button->owner->widget = (GtkWidget*) button;
	  gtk_data_notify ((GtkData*) button->owner);
	}
    }
  else
    {
      if (button->button.previous_state == GTK_STATE_NORMAL)
	button->button.previous_state = GTK_STATE_ACTIVE;
      else
	button->button.previous_state = GTK_STATE_NORMAL;
    }

  if (button->button.previous_state == GTK_STATE_ACTIVE)
    button->button.state->value = GTK_STATE_ACTIVATED;
  else
    button->button.state->value = GTK_STATE_DEACTIVATED;
  gtk_data_notify ((GtkData*) button->button.state);

  button->button.state->value = ((button->button.in_button) ?
				 (GTK_STATE_PRELIGHT) :
				 (button->button.previous_state));
  gtk_data_notify ((GtkData*) button->button.state);

  g_function_leave ("gtk_toggle_button_activate");
}

static void
gtk_toggle_button_draw (GtkWidget    *widget,
			GdkRectangle *area,
			gint          is_expose)
{
  GtkButton *button;
  GtkStateType state;

  g_function_enter ("gtk_toggle_button_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      button = (GtkButton*) widget;
      gtk_toggle_button_expose (widget);

      if (button->child)
	{
	  state = button->state->value;
	  if (button->draw_toggle_indicator)
	    if ((button->state->value != GTK_STATE_NORMAL) &&
		(button->state->value != GTK_STATE_PRELIGHT))
	      state = GTK_STATE_NORMAL;
	  gtk_widget_set_state (button->child, state);

	  if (!is_expose || GTK_WIDGET_NO_WINDOW (button->child))
	    gtk_widget_draw (button->child, NULL, is_expose);
	}
    }

  g_function_leave ("gtk_toggle_button_draw");
}

static void
gtk_toggle_button_expose (GtkWidget *widget)
{
  GtkButton *button;
  GtkToggleButton *toggle;
  GtkStateType state;
  gint x, y;
  gint width, height;

  g_function_enter ("gtk_toggle_button_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      button = (GtkButton*) widget;
      toggle = (GtkToggleButton*) widget;

      if (button->draw_toggle_indicator)
	{
	  state = button->state->value;
	  if ((state != GTK_STATE_NORMAL) &&
	      (state != GTK_STATE_PRELIGHT))
	    state = GTK_STATE_NORMAL;

	  gdk_window_set_background (widget->window,
				     &button->container.widget.style->background[state]);
	  gdk_window_clear (widget->window);

	  x = TOGGLE_INDICATOR_SPACING;
	  y = (widget->allocation.height - TOGGLE_INDICATOR_SIZE) / 2;
	  width = TOGGLE_INDICATOR_SIZE;
	  height = TOGGLE_INDICATOR_SIZE;

	  if (toggle->owner->data.observers->next)
	    {
	      gtk_draw_diamond (widget->window,
				widget->style->highlight_gc[button->state->value],
				widget->style->shadow_gc[button->state->value],
				widget->style->background_gc[button->state->value],
				button->shadow_type,
				x + 1, y, width, height,
				widget->style->shadow_thickness);
	    }
	  else
	    {
	      gtk_draw_shadow (widget->window,
			       widget->style->highlight_gc[button->state->value],
			       widget->style->shadow_gc[button->state->value],
			       widget->style->background_gc[button->state->value],
			       button->shadow_type,
			       x + 1, y + 1, width, height,
			       widget->style->shadow_thickness);
	    }
	}
      else
	{
	  gdk_window_set_background (widget->window,
				     &button->container.widget.style->background[button->state->value]);
	  gdk_window_clear (widget->window);

	  x = button->container.border_width;
	  y = button->container.border_width;
	  width = widget->allocation.width - 2 * x;
	  height = widget->allocation.height - 2 * y;

	  gtk_draw_shadow (widget->window,
			   widget->style->highlight_gc[button->state->value],
			   widget->style->shadow_gc[button->state->value],
			   NULL,
			   button->shadow_type,
			   x + 1, y + 1, width - 2, height - 2,
			   widget->style->shadow_thickness);
	}
    }

  g_function_leave ("gtk_toggle_button_expose");
}

static gint
gtk_button_state_update (GtkObserver *observer,
			 GtkData     *data)
{
  GtkButton *button;

  g_function_enter ("gtk_button_state_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  button = (GtkButton*) observer->user_data;
  g_assert (button != NULL);

  if (button->previous_state == GTK_STATE_NORMAL)
    {
      if (button->state->value == GTK_STATE_ACTIVE)
	button->shadow_type = GTK_SHADOW_IN;
      else
	button->shadow_type = GTK_SHADOW_OUT;
    }
  else
    {
      if ((button->state->value == GTK_STATE_ACTIVE) ||
	  (button->state->value == GTK_STATE_PRELIGHT))
	button->shadow_type = GTK_SHADOW_IN;
      else
	button->shadow_type = GTK_SHADOW_OUT;
    }

  if ((button->state->value != GTK_STATE_ACTIVATED) &&
      (button->state->value != GTK_STATE_DEACTIVATED))
    {
      gtk_widget_set_state ((GtkWidget*) button, button->state->value);
      gtk_widget_draw ((GtkWidget*) button, NULL, FALSE);
    }

  g_function_leave ("gtk_button_state_update");
  return FALSE;
}

static void
gtk_button_state_disconnect (GtkObserver *observer,
			     GtkData     *data)
{
  g_function_enter ("gtk_button_state_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_button_state_disconnect");
}


static gint
gtk_button_owner_update (GtkObserver *observer,
			 GtkData     *data)
{
  GtkToggleButton *button;
  GtkDataWidget *owner;

  g_function_enter ("gtk_button_owner_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  button = (GtkToggleButton*) observer->user_data;
  g_assert (button != NULL);

  owner = (GtkDataWidget*) data;

  if (owner->widget == (GtkWidget*) button)
    {
      if (button->button.state->value == GTK_STATE_NORMAL)
	{
	  button->button.previous_state = GTK_STATE_ACTIVE;
	  button->button.state->value = GTK_STATE_ACTIVE;
	  gtk_data_notify ((GtkData*) button->button.state);
	}
    }
  else
    {
      if (button->button.state->value == GTK_STATE_ACTIVE)
	{
	  button->button.previous_state = GTK_STATE_NORMAL;
	  button->button.state->value = GTK_STATE_NORMAL;
	  gtk_data_notify ((GtkData*) button->button.state);
	}
    }

  g_function_leave ("gtk_button_owner_update");
  return FALSE;
}

static void
gtk_button_owner_disconnect (GtkObserver *observer,
			     GtkData     *data)
{
  g_function_enter ("gtk_button_owner_disconnect");

  g_assert (observer != NULL);
  g_assert (data != NULL);

  g_function_leave ("gtk_button_owner_disconnect");
}
