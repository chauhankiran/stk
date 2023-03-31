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
#include "gtkframe.h"
#include "gtklist.h"
#include "gtklistbox.h"
#include "gtkscrollbar.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


typedef struct _GtkListBox GtkListBox;

struct _GtkListBox
{
  GtkContainer container;

  GtkWidget *frame;
  GtkWidget *list;
  GtkWidget *hscrollbar;
  GtkWidget *vscrollbar;
};


static void   gtk_listbox_destroy       (GtkWidget       *widget);
static void   gtk_listbox_map           (GtkWidget       *widget);
static void   gtk_listbox_unmap         (GtkWidget       *widget);
static void   gtk_listbox_realize       (GtkWidget       *widget);
static void   gtk_listbox_draw          (GtkWidget       *widget,
					 GdkRectangle    *area,
					 gint             is_expose);
static gint   gtk_listbox_event         (GtkWidget       *widget,
					 GdkEvent        *event);
static void   gtk_listbox_size_request  (GtkWidget       *widget,
					 GtkRequisition  *requisition);
static void   gtk_listbox_size_allocate (GtkWidget       *widget,
					 GtkAllocation   *allocation);
static gint   gtk_listbox_is_child      (GtkWidget       *widget,
					 GtkWidget       *child);
static gint   gtk_listbox_locate        (GtkWidget       *widget,
					 GtkWidget      **child,
					 gint             x,
					 gint             y);
static void   gtk_listbox_add           (GtkContainer    *container,
					 GtkWidget       *widget);
static void   gtk_listbox_remove        (GtkContainer    *container,
					 GtkWidget       *widget);
static void   gtk_listbox_foreach       (GtkContainer    *container,
					 GtkCallback      callback,
					 gpointer         callback_data);


static guint16 listbox_type = 0;

static GtkWidgetFunctions listbox_widget_functions =
{
  gtk_listbox_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_listbox_map,
  gtk_listbox_unmap,
  gtk_listbox_realize,
  gtk_listbox_draw,
  gtk_widget_default_draw_focus,
  gtk_listbox_event,
  gtk_listbox_size_request,
  gtk_listbox_size_allocate,
  gtk_listbox_is_child,
  gtk_listbox_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions listbox_container_functions =
{
  gtk_listbox_add,
  gtk_listbox_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_listbox_foreach,
};


GtkWidget*
gtk_listbox_new ()
{
  GtkListBox *listbox;
  GtkDataAdjustment *hadjustment;
  GtkDataAdjustment *vadjustment;

  g_function_enter ("gtk_listbox_new");

  listbox = g_new (GtkListBox, 1);

  listbox->container.widget.type = gtk_get_listbox_type ();
  listbox->container.widget.function_table = &listbox_widget_functions;
  listbox->container.function_table = &listbox_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) listbox);
  gtk_container_set_defaults ((GtkWidget*) listbox);

  listbox->frame = NULL;
  listbox->list = NULL;
  listbox->hscrollbar = NULL;
  listbox->vscrollbar = NULL;

  listbox->frame = gtk_frame_new (NULL);
  listbox->frame->parent = (GtkContainer*) listbox;
  gtk_frame_set_type (listbox->frame, GTK_SHADOW_NONE);
  gtk_container_set_border_width (listbox->frame, 0);

  listbox->list = gtk_list_new (NULL, NULL);
  gtk_container_add (listbox->frame, listbox->list);

  hadjustment = (GtkDataAdjustment*) gtk_list_get_hadjustment (listbox->list);
  vadjustment = (GtkDataAdjustment*) gtk_list_get_vadjustment (listbox->list);

  listbox->hscrollbar = gtk_hscrollbar_new (hadjustment);
  listbox->vscrollbar = gtk_vscrollbar_new (vadjustment);

  listbox->hscrollbar->parent = (GtkContainer*) listbox;
  listbox->vscrollbar->parent = (GtkContainer*) listbox;

  gtk_widget_show (listbox->frame);
  gtk_widget_show (listbox->list);
  gtk_widget_show (listbox->hscrollbar);
  gtk_widget_show (listbox->vscrollbar);

  g_function_leave ("gtk_listbox_new");
  return ((GtkWidget*) listbox);
}


GtkWidget*
gtk_listbox_get_list (GtkWidget *listbox)
{
  GtkListBox *rlistbox;
  GtkWidget *list;

  g_function_enter ("gtk_listbox_get_list");

  g_assert (listbox != NULL);

  rlistbox = (GtkListBox*) listbox;
  list = (GtkWidget*) rlistbox->list;

  g_function_leave ("gtk_listbox_get_list");
  return list;
}

GtkWidget*
gtk_listbox_get_hscrollbar (GtkWidget *listbox)
{
  GtkListBox *rlistbox;
  GtkWidget *hscrollbar;

  g_function_enter ("gtk_listbox_get_hscrollbar");

  g_assert (listbox != NULL);

  rlistbox = (GtkListBox*) listbox;
  hscrollbar = rlistbox->hscrollbar;

  g_function_leave ("gtk_listbox_get_hscrollbar");
  return hscrollbar;
}

GtkWidget*
gtk_listbox_get_vscrollbar (GtkWidget *listbox)
{
  GtkListBox *rlistbox;
  GtkWidget *vscrollbar;

  g_function_enter ("gtk_listbox_get_vscrollbar");

  g_assert (listbox != NULL);

  rlistbox = (GtkListBox*) listbox;
  vscrollbar = rlistbox->vscrollbar;

  g_function_leave ("gtk_listbox_get_vscrollbar");
  return vscrollbar;
}

void
gtk_listbox_set_shadow_type (GtkWidget     *listbox,
			     GtkShadowType  type)
{
  GtkListBox *rlistbox;

  g_function_enter ("gtk_listbox_set_shadow_type");

  g_assert (listbox != NULL);
  rlistbox = (GtkListBox*) listbox;

  gtk_frame_set_type (rlistbox->frame, type);

  g_function_leave ("gtk_listbox_set_shadow_type");
}

guint16
gtk_get_listbox_type ()
{
  g_function_enter ("gtk_get_listbox_type");

  if (!listbox_type)
    gtk_widget_unique_type (&listbox_type);

  g_function_leave ("gtk_get_listbox_type");
  return listbox_type;
}

static void
gtk_listbox_destroy (GtkWidget *widget)
{
  GtkListBox *listbox;

  g_function_enter ("gtk_listbox_destroy");

  g_assert (widget != NULL);

  listbox = (GtkListBox*) widget;
  if (!gtk_widget_destroy (listbox->frame))
    listbox->frame->parent = NULL;
  if (!gtk_widget_destroy (listbox->hscrollbar))
    listbox->hscrollbar->parent = NULL;
  if (!gtk_widget_destroy (listbox->vscrollbar))
    listbox->vscrollbar->parent = NULL;
  g_free (listbox);

  g_function_leave ("gtk_listbox_destroy");
}

static void
gtk_listbox_map (GtkWidget *widget)
{
  GtkListBox *listbox;

  g_function_enter ("gtk_listbox_map");

  g_assert (widget != NULL);

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
      gdk_window_show (widget->window);

      listbox = (GtkListBox*) widget;

      if (listbox->frame &&
	  GTK_WIDGET_VISIBLE (listbox->frame) &&
	  !GTK_WIDGET_MAPPED (listbox->frame))
	gtk_widget_map ((GtkWidget*) listbox->frame);

      if (listbox->list &&
	  GTK_WIDGET_VISIBLE (listbox->list) &&
	  !GTK_WIDGET_MAPPED (listbox->list))
	gtk_widget_map ((GtkWidget*) listbox->list);

      if (listbox->hscrollbar &&
	  GTK_WIDGET_VISIBLE (listbox->hscrollbar) &&
	  !GTK_WIDGET_MAPPED (listbox->hscrollbar))
	gtk_widget_map (listbox->hscrollbar);

      if (listbox->vscrollbar &&
	  GTK_WIDGET_VISIBLE (listbox->vscrollbar) &&
	  !GTK_WIDGET_MAPPED (listbox->vscrollbar))
	gtk_widget_map (listbox->vscrollbar);
    }

  g_function_leave ("gtk_listbox_map");
}

static void
gtk_listbox_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_listbox_unmap");

  g_assert (widget != NULL);

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
      gdk_window_hide (widget->window);
    }

  g_function_leave ("gtk_listbox_unmap");
}

static void
gtk_listbox_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;

  g_function_enter ("gtk_listbox_realize");

  g_assert (widget != NULL);

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

  g_function_leave ("gtk_listbox_realize");
}

static void
gtk_listbox_draw (GtkWidget    *widget,
		  GdkRectangle *area,
		  gint        is_expose)
{
  g_function_enter ("gtk_listbox_draw");

  g_assert (widget != NULL);

  g_function_leave ("gtk_listbox_draw");
}

static gint
gtk_listbox_event (GtkWidget *widget,
		   GdkEvent  *event)
{
  g_function_enter ("gtk_listbox_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    default:
      break;
    }

  g_function_leave ("gtk_listbox_event");
  return FALSE;
}

static void
gtk_listbox_size_request (GtkWidget      *widget,
			  GtkRequisition *requisition)
{
  GtkListBox *listbox;

  g_function_enter ("gtk_listbox_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  listbox = (GtkListBox*) widget;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      listbox->frame->requisition.width = 0;
      listbox->frame->requisition.height = 0;

      gtk_widget_size_request (listbox->frame, &listbox->frame->requisition);

      requisition->width = listbox->frame->requisition.width;
      requisition->height = listbox->frame->requisition.height;

      listbox->hscrollbar->requisition.width = 0;
      listbox->hscrollbar->requisition.height = 0;

      if (GTK_WIDGET_VISIBLE (listbox->hscrollbar))
	{
	  gtk_widget_size_request (listbox->hscrollbar,
				   &listbox->hscrollbar->requisition);

	  requisition->height += listbox->hscrollbar->requisition.height + 5;
	}

      listbox->vscrollbar->requisition.width = 0;
      listbox->vscrollbar->requisition.height = 0;

      if (GTK_WIDGET_VISIBLE (listbox->vscrollbar))
	{
	  gtk_widget_size_request (listbox->vscrollbar,
				   &listbox->vscrollbar->requisition);

	  requisition->width += listbox->hscrollbar->requisition.width + 5;
	}
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  g_function_leave ("gtk_listbox_size_request");
}

static void
gtk_listbox_size_allocate (GtkWidget     *widget,
			   GtkAllocation *allocation)
{
  GtkListBox *listbox;
  GtkAllocation child_allocation;
  gint width_extra;
  gint height_extra;

  g_function_enter ("gtk_listbox_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  listbox = (GtkListBox*) widget;

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

  width_extra = GTK_WIDGET_VISIBLE (listbox->vscrollbar) ? 5 : 0;
  height_extra = GTK_WIDGET_VISIBLE (listbox->hscrollbar) ? 5 : 0;

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = (allocation->width -
			    listbox->vscrollbar->requisition.width -
			    width_extra);
  child_allocation.height = (allocation->height -
			     listbox->hscrollbar->requisition.height -
			     height_extra);

  if (child_allocation.width <= 0)
    child_allocation.width = 1;
  if (child_allocation.height <= 0)
    child_allocation.height = 1;

  gtk_widget_size_allocate ((GtkWidget*) listbox->frame, &child_allocation);

  if (GTK_WIDGET_VISIBLE (listbox->hscrollbar))
    {
      child_allocation.x = 0;
      child_allocation.y = listbox->frame->allocation.height + height_extra;
      child_allocation.width = listbox->frame->allocation.width;
      child_allocation.height = listbox->hscrollbar->requisition.height;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate ((GtkWidget*) listbox->hscrollbar, &child_allocation);
    }

  if (GTK_WIDGET_VISIBLE (listbox->vscrollbar))
    {
      child_allocation.x = listbox->frame->allocation.width + width_extra;
      child_allocation.y = 0;
      child_allocation.width = listbox->vscrollbar->requisition.width;
      child_allocation.height = listbox->frame->allocation.height;

      if (child_allocation.width <= 0)
	child_allocation.width = 1;
      if (child_allocation.height <= 0)
	child_allocation.height = 1;

      gtk_widget_size_allocate ((GtkWidget*) listbox->vscrollbar, &child_allocation);
    }

  g_function_leave ("gtk_listbox_size_allocate");
}

static gint
gtk_listbox_is_child (GtkWidget *widget,
		      GtkWidget *child)
{
  GtkListBox *listbox;
  gint return_val;

  g_function_enter ("gtk_listbox_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  listbox = (GtkListBox*) widget;

  return_val = FALSE;
  if ((listbox->frame == child) ||
      (listbox->list == child) ||
      (listbox->hscrollbar == child) ||
      (listbox->vscrollbar == child))
    return_val = TRUE;
  else if (listbox->list)
    return_val = gtk_widget_is_child (listbox->list, child);

  g_function_leave ("gtk_listbox_is_child");
  return return_val;
}

static gint
gtk_listbox_locate (GtkWidget  *widget,
		    GtkWidget **child,
		    gint        x,
		    gint        y)
{
  g_function_enter ("gtk_listbox_locate");
  g_warning ("gtk_listbox_locate: UNFINISHED");
  g_function_leave ("gtk_listbox_locate");
  return FALSE;
}

static void
gtk_listbox_add (GtkContainer *container,
		 GtkWidget    *widget)
{
  GtkListBox *listbox;

  g_function_enter ("gtk_listbox_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  listbox = (GtkListBox*) container;

  if (listbox->frame &&
      listbox->list &&
      listbox->hscrollbar &&
      listbox->vscrollbar)
    g_error ("trying to add child to scrolled window...add to scrolled area instead");

  g_function_leave ("gtk_listbox_add");
}

static void
gtk_listbox_remove (GtkContainer *container,
		    GtkWidget    *widget)
{
  g_function_enter ("gtk_listbox_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  g_error ("tring to remove child from listbox...remove from list instead");

  g_function_leave ("gtk_listbox_remove");
}

static void
gtk_listbox_foreach (GtkContainer *container,
		     GtkCallback   callback,
		     gpointer      callback_data)
{
  GtkListBox *listbox;

  g_function_enter ("gtk_listbox_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  listbox = (GtkListBox*) container;

  (* callback) (listbox->frame, callback_data, NULL);
  (* callback) (listbox->hscrollbar, callback_data, NULL);
  (* callback) (listbox->vscrollbar, callback_data, NULL);

  g_function_leave ("gtk_listbox_foreach");
}
