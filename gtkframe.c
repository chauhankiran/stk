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
#include "gtkframe.h"
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkFrame  GtkFrame;

struct _GtkFrame
{
  GtkContainer container;

  GtkWidget *child;
  gchar *label;
  gint16 shadow_type;
  gint16 label_width;
  gint16 label_height;
  gfloat label_xalign;
  gfloat label_yalign;
};


static void  gtk_frame_destroy       (GtkWidget       *widget);
static void  gtk_frame_map           (GtkWidget       *widget);
static void  gtk_frame_unmap         (GtkWidget       *widget);
static void  gtk_frame_realize       (GtkWidget       *widget);
static void  gtk_frame_draw          (GtkWidget       *widget,
				      GdkRectangle    *area,
				      gint             is_expose);
static void  gtk_frame_size_request  (GtkWidget       *widget,
				      GtkRequisition  *requisition);
static void  gtk_frame_size_allocate (GtkWidget       *widget,
				      GtkAllocation   *allocation);
static gint  gtk_frame_is_child      (GtkWidget       *widget,
				      GtkWidget       *child);
static gint  gtk_frame_locate        (GtkWidget       *widget,
				      GtkWidget      **child,
				      gint             x,
				      gint             y);
static void  gtk_frame_set_state     (GtkWidget       *widget,
				      GtkStateType     state);
static void  gtk_frame_add           (GtkContainer    *container,
				      GtkWidget       *widget);
static void  gtk_frame_remove        (GtkContainer    *container,
				      GtkWidget       *widget);
static void  gtk_frame_foreach       (GtkContainer   *container,
				      GtkCallback     callback,
				      gpointer        callback_data);


static GtkWidgetFunctions frame_widget_functions =
{
  gtk_frame_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_frame_map,
  gtk_frame_unmap,
  gtk_frame_realize,
  gtk_frame_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_frame_size_request,
  gtk_frame_size_allocate,
  gtk_frame_is_child,
  gtk_frame_locate,
  gtk_widget_default_activate,
  gtk_frame_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions frame_container_functions =
{
  gtk_frame_add,
  gtk_frame_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_frame_foreach,
};


GtkWidget*
gtk_frame_new (gchar *label)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_new");

  frame = g_new (GtkFrame, 1);

  frame->container.widget.type = gtk_get_frame_type ();
  frame->container.widget.function_table = &frame_widget_functions;
  frame->container.function_table = &frame_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) frame);
  gtk_container_set_defaults ((GtkWidget*) frame);

  frame->child = NULL;
  frame->label = g_strdup (label);
  frame->shadow_type = GTK_SHADOW_ETCHED_IN;
  frame->label_xalign = 0.0;
  frame->label_yalign = 0.5;

  if (frame->label)
    {
      frame->label_width = gdk_string_width (frame->container.widget.style->font, frame->label) + 6;
      frame->label_height = (frame->container.widget.style->font->ascent +
			     frame->container.widget.style->font->descent);
    }
  else
    {
      frame->label_width = 0;
      frame->label_height = 0;
    }

  g_function_leave ("gtk_frame_new");
  return ((GtkWidget*) frame);
}

void
gtk_frame_set_label (GtkWidget *widget,
		     gchar     *label)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_set_label");

  g_assert (widget != NULL);
  frame = (GtkFrame*) widget;

  if (frame->label)
    g_free (frame->label);
  frame->label = g_strdup (label);

  frame->label_width = gdk_string_width (frame->container.widget.style->font, frame->label) + 6;
  frame->label_height = (frame->container.widget.style->font->ascent +
			 frame->container.widget.style->font->descent);

  if (GTK_WIDGET_VISIBLE (frame))
    gtk_container_need_resize (frame->container.widget.parent, (GtkWidget*) frame);

  gtk_widget_draw ((GtkWidget*) frame, NULL, FALSE);

  g_function_leave ("gtk_frame_set_label");
}

void
gtk_frame_set_label_align (GtkWidget *widget,
			   gfloat     xalign,
			   gfloat     yalign)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_set_label_align");

  g_assert (widget != NULL);
  frame = (GtkFrame*) widget;

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);

  if ((xalign != frame->label_xalign) || (yalign != frame->label_yalign))
    {
      frame->label_xalign = xalign;
      frame->label_yalign = yalign;

      gtk_widget_size_allocate ((GtkWidget*) frame, &frame->container.widget.allocation);
      gtk_widget_draw ((GtkWidget*) frame, NULL, FALSE);
    }

  gtk_widget_draw ((GtkWidget*) frame, NULL, FALSE);

  g_function_leave ("gtk_frame_set_label_align");
}

void
gtk_frame_set_type (GtkWidget     *widget,
		    GtkShadowType  type)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_set_shadow_type");

  g_assert (widget != NULL);
  frame = (GtkFrame*) widget;

  frame->shadow_type = type;

  if (GTK_WIDGET_VISIBLE (frame))
    gtk_container_need_resize (frame->container.widget.parent, (GtkWidget*) frame);

  gtk_widget_draw ((GtkWidget*) frame, NULL, FALSE);

  g_function_leave ("gtk_frame_set_shadow_type");
}

guint16
gtk_get_frame_type ()
{
  static guint16 frame_type = 0;

  g_function_enter ("gtk_get_frame_type");

  if (!frame_type)
    gtk_widget_unique_type (&frame_type);

  g_function_leave ("gtk_get_frame_type");
  return frame_type;
}


static void
gtk_frame_destroy (GtkWidget *widget)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_destroy");

  g_assert (widget != NULL);
  frame = (GtkFrame*) widget;

  if (frame->child)
    if (!gtk_widget_destroy (frame->child))
      frame->child->parent = NULL;
  if (frame->container.widget.window)
    gdk_window_destroy (frame->container.widget.window);
  if (frame->label)
    g_free (frame->label);
  g_free (frame);

  g_function_leave ("gtk_frame_destroy");
}

static void
gtk_frame_map (GtkWidget *widget)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_map");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);

  frame = (GtkFrame*) widget;

  if (frame->child &&
      GTK_WIDGET_VISIBLE (frame->child) &&
      !GTK_WIDGET_MAPPED (frame->child))
    gtk_widget_map (frame->child);

  g_function_leave ("gtk_frame_map");
}

static void
gtk_frame_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_frame_unmap");

  g_assert (widget != NULL);

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);

  g_function_leave ("gtk_frame_unmap");
}

static void
gtk_frame_realize (GtkWidget *widget)
{
  GtkFrame *frame;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_function_enter ("gtk_frame_realize");

  g_assert (widget != NULL);

  frame = (GtkFrame*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.visual = gtk_peek_visual ();
  attributes.colormap = gtk_peek_colormap ();
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_EXPOSURE_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  frame->container.widget.window = gdk_window_new (widget->parent->widget.window,
						   &attributes, attributes_mask);
  gdk_window_set_user_data (frame->container.widget.window, frame);

  frame->container.widget.style = gtk_style_attach (frame->container.widget.style,
						    frame->container.widget.window);
  gdk_window_set_background (frame->container.widget.window,
			     &frame->container.widget.style->background[GTK_STATE_NORMAL]);

  g_function_leave ("gtk_frame_realize");
}

static void
gtk_frame_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint          is_expose)
{
  GtkFrame *frame;
  GdkRectangle child_area;
  gint shadow_thickness;
  gint height_extra;
  gint label_area_width;
  gint x, y;

  g_function_enter ("gtk_frame_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      frame = (GtkFrame*) widget;

      shadow_thickness = widget->style->shadow_thickness;
      height_extra = frame->label_height - shadow_thickness;
      height_extra = MAX (height_extra, 0);

      gtk_draw_shadow (widget->window,
		       frame->container.widget.style->highlight_gc[GTK_STATE_NORMAL],
		       frame->container.widget.style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       frame->shadow_type,
		       frame->container.border_width,
		       frame->container.border_width + height_extra / 2,
		       widget->allocation.width - frame->container.border_width * 2,
		       widget->allocation.height - frame->container.border_width * 2 - height_extra / 2,
		       frame->container.widget.style->shadow_thickness);

      if (frame->label)
	{
	  label_area_width = (frame->container.widget.allocation.width -
			      frame->container.border_width * 2 -
			      shadow_thickness * 2);

	  x = ((label_area_width - frame->label_width) * frame->label_xalign +
	       frame->container.border_width + shadow_thickness);
	  y = frame->container.border_width + frame->container.widget.style->font->ascent;

	  gdk_window_clear_area (widget->window,
				 x + 2, frame->container.border_width,
				 frame->label_width - 4, frame->label_height);

	  gdk_draw_string (widget->window,
			   frame->container.widget.style->foreground_gc[GTK_STATE_NORMAL],
			   x + 3, y, frame->label);
	}

      if (frame->child)
	if (!is_expose || GTK_WIDGET_NO_WINDOW (frame->child))
	  if (gtk_widget_intersect (frame->child, area, &child_area))
	    gtk_widget_draw (frame->child, &child_area, is_expose);
    }

  g_function_leave ("gtk_frame_draw");
}

static void
gtk_frame_size_request (GtkWidget      *widget,
			GtkRequisition *requisition)
{
  GtkFrame *frame;
  gint shadow_thickness;
  gint height_extra;

  g_function_enter ("gtk_frame_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  frame = (GtkFrame*) widget;
  shadow_thickness = widget->style->shadow_thickness;
  height_extra = frame->label_height - shadow_thickness;
  height_extra = MAX (height_extra, 0);

  if (frame->child)
    {
      frame->child->requisition.width = 0;
      frame->child->requisition.height = 0;

      gtk_widget_size_request (frame->child, &frame->child->requisition);

      requisition->width = MAX (frame->child->requisition.width, frame->label_width);
      requisition->width += frame->container.border_width * 2 + shadow_thickness * 2;

      requisition->height = (frame->child->requisition.height +
			     frame->container.border_width * 2 +
			     shadow_thickness * 2 + height_extra);
    }
  else
    {
      requisition->width = frame->container.border_width * 2 + shadow_thickness * 2 + frame->label_width;
      requisition->height = frame->container.border_width * 2 + shadow_thickness * 2 + height_extra;
    }

  g_function_leave ("gtk_frame_size_request");
}

static void
gtk_frame_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  GtkFrame *frame;
  GtkAllocation child_allocation;
  gint shadow_thickness;
  gint height_extra;

  g_function_enter ("gtk_frame_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  frame = (GtkFrame*) widget;

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

  if (frame->child)
    {
      shadow_thickness = widget->style->shadow_thickness;

      child_allocation.x = (frame->container.border_width + shadow_thickness);
      child_allocation.width = allocation->width - child_allocation.x * 2;

      height_extra = MAX (frame->label_height, shadow_thickness);
      child_allocation.y = (frame->container.border_width + height_extra);

      child_allocation.height = (allocation->height -
				 child_allocation.y -
				 frame->container.border_width -
				 shadow_thickness);

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate (frame->child, &child_allocation);
    }

  g_function_leave ("gtk_frame_size_allocate");
}

static gint
gtk_frame_is_child (GtkWidget *widget,
		    GtkWidget *child)
{
  GtkFrame *frame;
  gint return_val;

  g_function_enter ("gtk_frame_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  frame = (GtkFrame*) widget;

  return_val = FALSE;
  if (frame->child == child)
    return_val = TRUE;
  else if (frame->child)
    return_val = gtk_widget_is_child (frame->child, child);

  g_function_leave ("gtk_frame_is_child");
  return return_val;
}

static gint
gtk_frame_locate (GtkWidget  *widget,
		  GtkWidget **child,
		  gint        x,
		  gint        y)
{
  g_function_enter ("gtk_frame_locate");
  g_warning ("gtk_frame_locate: UNFINISHED");
  g_function_leave ("gtk_frame_locate");
  return FALSE;
}

static void
gtk_frame_set_state (GtkWidget    *widget,
		     GtkStateType  state)
{
  g_function_enter ("gtk_frame_set_state");
  g_warning ("gtk_frame_set_state: UNFINISHED");
  g_function_leave ("gtk_frame_set_state");
}

static void
gtk_frame_add (GtkContainer *container,
	       GtkWidget    *widget)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_add");

  g_assert (container != NULL);
  frame = (GtkFrame*) container;

  if (frame->child)
    g_error ("frame already has a child");
  else
    frame->child = widget;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_frame_add");
}

static void
gtk_frame_remove (GtkContainer *container,
		  GtkWidget    *widget)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  frame = (GtkFrame*) container;

  if (frame->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!frame->child)
    g_error ("frame has no child to remove");

  frame->child = NULL;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_frame_remove");
}

static void
gtk_frame_foreach (GtkContainer *container,
		   GtkCallback   callback,
		   gpointer      callback_data)
{
  GtkFrame *frame;

  g_function_enter ("gtk_frame_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  frame = (GtkFrame*) container;

  if (frame->child)
    (* callback) (frame->child, callback_data, NULL);

  g_function_leave ("gtk_frame_foreach");
}
